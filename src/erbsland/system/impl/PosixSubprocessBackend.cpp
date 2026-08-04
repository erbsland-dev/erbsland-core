// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixSubprocessBackend.hpp"

#include "../PlatformError.hpp"
#include "../PosixErrorContext.hpp"
#include "../SubprocessOptions.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"

#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <map>
#include <string_view>
#include <thread>
#include <vector>

extern char **environ;

namespace erbsland::system::impl {
using namespace text::literals;

PosixSubprocessBackend::PosixSubprocessBackend(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options) {
    auto outputPipe = std::array<int, 2>{-1, -1};
    auto errorPipe = std::array<int, 2>{-1, -1};
    const auto captureOutput = options.standardOutputMode() == SubprocessOutputMode::Capture;
    const auto captureError =
        !options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Capture;
    if (captureOutput && ::pipe(outputPipe.data()) != 0) {
        throw PlatformError{"Failed to create the subprocess standard output pipe."_el, PosixErrorContext::fromErrno()};
    }
    if (captureError && ::pipe(errorPipe.data()) != 0) {
        const auto error = PosixErrorContext::fromErrno();
        ::close(outputPipe[0]);
        ::close(outputPipe[1]);
        throw PlatformError{"Failed to create the subprocess standard error pipe."_el, error};
    }

    auto actions = posix_spawn_file_actions_t{};
    auto actionsInitialized = false;
    auto nullDescriptor = -1;
    const auto closeDescriptors = [&]() noexcept -> void {
        for (const auto descriptor : outputPipe) {
            if (descriptor >= 0) {
                ::close(descriptor);
            }
        }
        for (const auto descriptor : errorPipe) {
            if (descriptor >= 0) {
                ::close(descriptor);
            }
        }
        if (nullDescriptor >= 0) {
            ::close(nullDescriptor);
        }
        if (actionsInitialized) {
            posix_spawn_file_actions_destroy(&actions);
        }
    };

    auto status = posix_spawn_file_actions_init(&actions);
    if (status != 0) {
        closeDescriptors();
        throw PlatformError{
            "Failed to initialize subprocess file actions."_el, PosixErrorContext::fromErrorCode(status)};
    }
    actionsInitialized = true;

    const auto needsNull = !options.inheritsStandardInput() ||
        options.standardOutputMode() == SubprocessOutputMode::Discard ||
        (!options.mergesStandardError() && options.standardErrorMode() == SubprocessOutputMode::Discard);
    if (needsNull) {
        nullDescriptor = ::open("/dev/null", O_RDWR);
        if (nullDescriptor < 0) {
            const auto error = PosixErrorContext::fromErrno();
            closeDescriptors();
            throw PlatformError{"Failed to open the null device for a subprocess."_el, error};
        }
    }
    const auto addAction = [&](const int result, const text::String &reason) -> void {
        if (result != 0) {
            closeDescriptors();
            throw PlatformError{reason, PosixErrorContext::fromErrorCode(result)};
        }
    };
    if (!options.inheritsStandardInput()) {
        addAction(
            posix_spawn_file_actions_adddup2(&actions, nullDescriptor, STDIN_FILENO),
            "Failed to configure subprocess standard input."_el);
    }
    if (captureOutput) {
        addAction(
            posix_spawn_file_actions_adddup2(&actions, outputPipe[1], STDOUT_FILENO),
            "Failed to configure subprocess standard output capture."_el);
        addAction(
            posix_spawn_file_actions_addclose(&actions, outputPipe[0]),
            "Failed to configure subprocess standard output capture."_el);
        addAction(
            posix_spawn_file_actions_addclose(&actions, outputPipe[1]),
            "Failed to configure subprocess standard output capture."_el);
    } else if (options.standardOutputMode() == SubprocessOutputMode::Discard) {
        addAction(
            posix_spawn_file_actions_adddup2(&actions, nullDescriptor, STDOUT_FILENO),
            "Failed to discard subprocess standard output."_el);
    }
    if (options.mergesStandardError()) {
        addAction(
            posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO),
            "Failed to merge subprocess standard error."_el);
    } else if (captureError) {
        addAction(
            posix_spawn_file_actions_adddup2(&actions, errorPipe[1], STDERR_FILENO),
            "Failed to configure subprocess standard error capture."_el);
        addAction(
            posix_spawn_file_actions_addclose(&actions, errorPipe[0]),
            "Failed to configure subprocess standard error capture."_el);
        addAction(
            posix_spawn_file_actions_addclose(&actions, errorPipe[1]),
            "Failed to configure subprocess standard error capture."_el);
    } else if (options.standardErrorMode() == SubprocessOutputMode::Discard) {
        addAction(
            posix_spawn_file_actions_adddup2(&actions, nullDescriptor, STDERR_FILENO),
            "Failed to discard subprocess standard error."_el);
    }
    if (options.workingDirectory().has_value()) {
        const auto directory = options.workingDirectory()->toStdPath().string();
#if defined(__APPLE__)
        addAction(
            posix_spawn_file_actions_addchdir(&actions, directory.c_str()),
#else
        addAction(
            posix_spawn_file_actions_addchdir_np(&actions, directory.c_str()),
#endif
            "Failed to configure the subprocess working directory."_el);
    }

    auto argumentStorage = std::vector<std::string>{};
    argumentStorage.reserve(arguments.count().toSizeT() + 1U);
    argumentStorage.emplace_back(executable.toStdPath().string());
    for (const auto &argument : arguments) {
        argumentStorage.emplace_back(text::StringConverter{argument}.toStdString());
    }
    auto argumentPointers = std::vector<char *>{};
    argumentPointers.reserve(argumentStorage.size() + 1U);
    for (auto &argument : argumentStorage) {
        argumentPointers.push_back(argument.data());
    }
    argumentPointers.push_back(nullptr);

    auto environment = std::map<std::string, std::string>{};
    if (options.inheritsEnvironment()) {
        for (auto cursor = environ; cursor != nullptr && *cursor != nullptr; ++cursor) {
            const auto entry = std::string_view{*cursor};
            const auto separator = entry.find('=');
            if (separator != std::string_view::npos) {
                environment.insert_or_assign(
                    std::string{entry.substr(0U, separator)}, std::string{entry.substr(separator + 1U)});
            }
        }
    }
    for (const auto &[name, value] : options.environmentChanges()) {
        const auto nativeName = text::StringConverter{name}.toStdString();
        if (value.has_value()) {
            environment.insert_or_assign(nativeName, text::StringConverter{*value}.toStdString());
        } else {
            environment.erase(nativeName);
        }
    }
    auto environmentStorage = std::vector<std::string>{};
    environmentStorage.reserve(environment.size());
    for (const auto &[name, value] : environment) {
        environmentStorage.emplace_back(name + "=" + value);
    }
    auto environmentPointers = std::vector<char *>{};
    environmentPointers.reserve(environmentStorage.size() + 1U);
    for (auto &entry : environmentStorage) {
        environmentPointers.push_back(entry.data());
    }
    environmentPointers.push_back(nullptr);

    auto processId = pid_t{};
    const auto executablePath = executable.toStdPath().string();
    status = posix_spawn(
        &processId, executablePath.c_str(), &actions, nullptr, argumentPointers.data(), environmentPointers.data());
    posix_spawn_file_actions_destroy(&actions);
    actionsInitialized = false;
    if (nullDescriptor >= 0) {
        ::close(nullDescriptor);
        nullDescriptor = -1;
    }
    if (status != 0) {
        closeDescriptors();
        throw PlatformError{"Failed to launch the subprocess."_el, PosixErrorContext::fromErrorCode(status)};
    }
    _processId = static_cast<int>(processId);

    if (outputPipe[1] >= 0) {
        ::close(outputPipe[1]);
        outputPipe[1] = -1;
    }
    if (errorPipe[1] >= 0) {
        ::close(errorPipe[1]);
        errorPipe[1] = -1;
    }
    if (captureOutput) {
        _standardOutput = std::make_shared<CaptureState>();
        _standardOutput->limit = options.captureLimit().toSizeT();
        _standardOutputReader = std::thread{&PosixSubprocessBackend::readCaptured, outputPipe[0], _standardOutput};
        outputPipe[0] = -1;
    }
    if (captureError) {
        _standardError = std::make_shared<CaptureState>();
        _standardError->limit = options.captureLimit().toSizeT();
        _standardErrorReader = std::thread{&PosixSubprocessBackend::readCaptured, errorPipe[0], _standardError};
        errorPipe[0] = -1;
    }
    closeDescriptors();
}

PosixSubprocessBackend::~PosixSubprocessBackend() {
    joinReaders();
}

auto PosixSubprocessBackend::isRunning() -> bool {
    const auto lock = std::scoped_lock{_waitMutex};
    if (_exitStatus.has_value()) {
        return false;
    }
    return !pollStatus(WNOHANG);
}

auto PosixSubprocessBackend::exitStatus() const noexcept -> const std::optional<SubprocessExitStatus> & {
    return _exitStatus;
}

auto PosixSubprocessBackend::wait() -> SubprocessExitStatus {
    {
        const auto lock = std::scoped_lock{_waitMutex};
        if (!_exitStatus.has_value()) {
            pollStatus(0);
        }
    }
    joinReaders();
    return *_exitStatus;
}

auto PosixSubprocessBackend::wait(const time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> {
    const auto deadline = std::chrono::steady_clock::now() + timeout.toStdNanoseconds();
    while (true) {
        {
            const auto lock = std::scoped_lock{_waitMutex};
            if (_exitStatus.has_value() || pollStatus(WNOHANG)) {
                break;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return std::nullopt;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    joinReaders();
    return _exitStatus;
}

void PosixSubprocessBackend::terminate() {
    if (_exitStatus.has_value() || ::kill(static_cast<pid_t>(_processId), SIGTERM) == 0 || errno == ESRCH) {
        return;
    }
    throw PlatformError{"Failed to terminate the subprocess."_el, PosixErrorContext::fromErrno()};
}

void PosixSubprocessBackend::kill() {
    if (_exitStatus.has_value() || ::kill(static_cast<pid_t>(_processId), SIGKILL) == 0 || errno == ESRCH) {
        return;
    }
    throw PlatformError{"Failed to kill the subprocess."_el, PosixErrorContext::fromErrno()};
}

auto PosixSubprocessBackend::standardOutput() const -> text::String {
    return capturedText(_standardOutput);
}

auto PosixSubprocessBackend::standardError() const -> text::String {
    return capturedText(_standardError);
}

auto PosixSubprocessBackend::wasStandardOutputTruncated() const noexcept -> bool {
    return wasTruncated(_standardOutput);
}

auto PosixSubprocessBackend::wasStandardErrorTruncated() const noexcept -> bool {
    return wasTruncated(_standardError);
}

auto PosixSubprocessBackend::pollStatus(const int options) -> bool {
    auto nativeStatus = 0;
    const auto result = ::waitpid(static_cast<pid_t>(_processId), &nativeStatus, options);
    if (result == 0) {
        return false;
    }
    if (result < 0) {
        if (errno == EINTR) {
            return pollStatus(options);
        }
        throw PlatformError{"Failed while waiting for the subprocess."_el, PosixErrorContext::fromErrno()};
    }
    storeStatus(nativeStatus);
    return true;
}

void PosixSubprocessBackend::storeStatus(const int status) {
    if (WIFEXITED(status)) {
        _exitStatus = SubprocessExitStatus::exited(WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        _exitStatus = SubprocessExitStatus::signaled(WTERMSIG(status));
    } else {
        _exitStatus = SubprocessExitStatus::exited(-1);
    }
}

void PosixSubprocessBackend::joinReaders() {
    if (_standardOutputReader.joinable()) {
        _standardOutputReader.join();
    }
    if (_standardErrorReader.joinable()) {
        _standardErrorReader.join();
    }
}

void PosixSubprocessBackend::readCaptured(const int descriptor, const std::shared_ptr<CaptureState> &state) noexcept {
    auto buffer = std::array<char, 8192>{};
    while (true) {
        const auto count = ::read(descriptor, buffer.data(), buffer.size());
        if (count == 0) {
            break;
        }
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        const auto lock = std::scoped_lock{state->mutex};
        const auto available = state->limit > state->bytes.size() ? state->limit - state->bytes.size() : 0U;
        const auto retain = std::min<std::size_t>(available, static_cast<std::size_t>(count));
        state->bytes.append(buffer.data(), retain);
        state->truncated = state->truncated || retain < static_cast<std::size_t>(count);
    }
    ::close(descriptor);
}

auto PosixSubprocessBackend::capturedText(const std::shared_ptr<CaptureState> &state) -> text::String {
    if (state == nullptr) {
        return {};
    }
    const auto lock = std::scoped_lock{state->mutex};
    return text::String{std::string_view{state->bytes}};
}

auto PosixSubprocessBackend::wasTruncated(const std::shared_ptr<CaptureState> &state) noexcept -> bool {
    if (state == nullptr) {
        return false;
    }
    const auto lock = std::scoped_lock{state->mutex};
    return state->truncated;
}

auto createSubprocessBackend(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
    -> SubprocessBackendPtr {
    return std::make_unique<PosixSubprocessBackend>(executable, arguments, options);
}

}
