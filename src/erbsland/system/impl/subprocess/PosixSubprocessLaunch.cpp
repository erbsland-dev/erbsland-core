// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixSubprocessLaunch.hpp"

#include "../environment/EnvironmentChangesProcessor.hpp"
#include "../PosixErrorContext.hpp"

#include "../../../text/impl/PlatformU8StringAccess.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../../text/StringJoins.hpp"
#include "../../PlatformError.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <string_view>

extern char **environ;

namespace erbsland::system::impl {

using namespace text;
using namespace text::literals;

PosixSubprocessLaunch::~PosixSubprocessLaunch() {
    for (const auto descriptor : {_outputPipe[0], _outputPipe[1], _errorPipe[0], _errorPipe[1], _nullDescriptor}) {
        if (descriptor >= 0) {
            ::close(descriptor);
        }
    }
    if (_actionsInitialized) {
        posix_spawn_file_actions_destroy(&_actions);
    }
}

void PosixSubprocessLaunch::checkAction(int result, const String &reason) {
    if (result != 0) {
        throw PlatformError{reason, PosixErrorContext::fromErrorCode(result)};
    }
}

void PosixSubprocessLaunch::createPipe(std::array<int, 2> &pipe) {
    if (::pipe(pipe.data()) != 0) {
        throw PlatformError{"Failed to create a subprocess output pipe."_el, PosixErrorContext::fromErrno()};
    }
}

void PosixSubprocessLaunch::prepareStreams(const SubprocessOptions &options) {
    if (options.standardOutputMode() == SubprocessOutputMode::Capture) {
        createPipe(_outputPipe);
    }
    if (!options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Capture) {
        createPipe(_errorPipe);
    }
    if (!options.inheritsStandardInput() || options.standardOutputMode() == SubprocessOutputMode::Discard ||
        (!options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Discard)) {
        _nullDescriptor = ::open("/dev/null", O_RDWR);
        if (_nullDescriptor < 0) {
            throw PlatformError{"Failed to open the subprocess null device."_el, PosixErrorContext::fromErrno()};
        }
    }
}

void PosixSubprocessLaunch::prepareActions(const SubprocessOptions &options) {
    checkAction(posix_spawn_file_actions_init(&_actions), "Failed to initialize subprocess file actions."_el);
    _actionsInitialized = true;
    if (!options.inheritsStandardInput()) {
        redirect(_nullDescriptor, STDIN_FILENO);
    }
    configureOutput(options.standardOutputMode(), _outputPipe, STDOUT_FILENO);
    if (options.mergesStandardError()) {
        redirect(STDOUT_FILENO, STDERR_FILENO);
    } else {
        configureOutput(options.standardErrorMode(), _errorPipe, STDERR_FILENO);
    }
    configureWorkingDirectory(options);
}

void PosixSubprocessLaunch::redirect(int source, int destination) {
    checkAction(
        posix_spawn_file_actions_adddup2(&_actions, source, destination),
        "Failed to configure subprocess standard streams."_el);
}

void PosixSubprocessLaunch::configureOutput(
    SubprocessOutputMode mode, const std::array<int, 2> &pipe, int destination) {
    if (mode == SubprocessOutputMode::Capture) {
        redirect(pipe[1], destination);
        checkAction(posix_spawn_file_actions_addclose(&_actions, pipe[0]), "Failed to close the child reader pipe."_el);
        checkAction(posix_spawn_file_actions_addclose(&_actions, pipe[1]), "Failed to close the child writer pipe."_el);
    } else if (mode == SubprocessOutputMode::Discard) {
        redirect(_nullDescriptor, destination);
    }
}

void PosixSubprocessLaunch::configureWorkingDirectory(const SubprocessOptions &options) {
    if (!options.workingDirectory().has_value()) {
        return;
    }
    const auto directory = options.workingDirectory()->toStdPath().string();
#if defined(__APPLE__)
    checkAction(
        posix_spawn_file_actions_addchdir(&_actions, directory.c_str()),
#else
    checkAction(
        posix_spawn_file_actions_addchdir_np(&_actions, directory.c_str()),
#endif
        "Failed to configure the subprocess working directory."_el);
}

void PosixSubprocessLaunch::prepareArguments(const path::Path &executable, const StringList &arguments) {

    _argumentStorage.reserve(arguments.count().toSizeT() + 1U);
    _argumentStorage.emplace_back(executable.toStdPath().string());
    for (const auto &argument : arguments) {
        _argumentStorage.emplace_back(StringConverter{argument}.toStdString());
    }

    _argumentPointers.reserve(_argumentStorage.size() + 1U);
    for (auto &argument : _argumentStorage) {
        _argumentPointers.push_back(argument.data());
    }
    _argumentPointers.push_back(nullptr);
}

void PosixSubprocessLaunch::prepareEnvironment(const SubprocessOptions &options) {
    auto base = EnvironmentChangesProcessor::Environment<String>{
        EnvironmentChangesProcessor::NameLess<String>{EnvironmentVariableBackend::native()}};
    for (auto cursor = environ; cursor != nullptr && *cursor != nullptr; ++cursor) {
        const auto entry = std::string_view{*cursor};
        const auto separator = entry.find('=');
        if (separator != std::string_view::npos) {
            base.insert_or_assign(String{entry.substr(0U, separator)}, String{entry.substr(separator + 1U)});
        }
    }
    const auto environment = EnvironmentChangesProcessor::apply(options.environmentChanges(), base);
    _environmentStorage.reserve(environment.size());
    for (const auto &[name, value] : environment) {
        const auto entry = text::join(name, "="_el, value);
        const auto access = text::impl::PlatformU8StringAccess{entry};
        _environmentStorage.emplace_back(access.nullTerminatedCharPtr(), access.sizeWithoutNull());
    }

    _environmentPointers.reserve(_environmentStorage.size() + 1U);
    for (auto &entry : _environmentStorage) {
        _environmentPointers.push_back(entry.data());
    }
    _environmentPointers.push_back(nullptr);
}

auto PosixSubprocessLaunch::start(
    const path::Path &executable, const StringList &arguments, const SubprocessOptions &options) -> int {
    prepareStreams(options);
    prepareActions(options);
    prepareArguments(executable, arguments);
    prepareEnvironment(options);
    auto processId = pid_t{};
    const auto executablePath = executable.toStdPath().string();
    checkAction(
        posix_spawn(
            &processId,
            executablePath.c_str(),
            &_actions,
            nullptr,
            _argumentPointers.data(),
            _environmentPointers.data()),
        "Failed to launch the subprocess."_el);
    // Parent writers must close before readers wait for EOF.
    closeWriter(_outputPipe[1]);
    closeWriter(_errorPipe[1]);
    return static_cast<int>(processId);
}

void PosixSubprocessLaunch::closeWriter(int &descriptor) noexcept {
    if (descriptor >= 0) {
        ::close(descriptor);
        descriptor = -1;
    }
}

auto PosixSubprocessLaunch::outputDescriptor() noexcept -> int & {
    return _outputPipe[0];
}

auto PosixSubprocessLaunch::errorDescriptor() noexcept -> int & {
    return _errorPipe[0];
}

}
