// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixSubprocessBackend.hpp"

#include "PosixSubprocessLaunch.hpp"

#include "../PosixErrorContext.hpp"
#include "../ProcessIdAccess.hpp"

#include "../../../text/Literals.hpp"
#include "../../PlatformError.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <string_view>
#include <thread>

namespace erbsland::system::impl {

using namespace text::literals;

PosixSubprocessBackend::PosixSubprocessBackend(
    const path::Path &executable,
    const text::StringList &arguments,
    const SubprocessOptions &options,
    const SubprocessOutputSink &sink) {
    auto launch = PosixSubprocessLaunch{};
    _processId = launch.start(executable, arguments, options);
    try {
        startReader(launch.outputDescriptor(), false, sink);
        startReader(launch.errorDescriptor(), true, sink);
    } catch (...) {
        ::kill(static_cast<pid_t>(_processId), SIGKILL);
        auto childStatus = int{};
        while (::waitpid(static_cast<pid_t>(_processId), &childStatus, 0) < 0 && errno == EINTR) {}
        joinReaders();
        throw;
    }
}

PosixSubprocessBackend::~PosixSubprocessBackend() {
    joinReaders();
}

auto PosixSubprocessBackend::processId() const noexcept -> ProcessId {
    return _processId > 0 ? ProcessIdAccess::fromNative(static_cast<std::uint64_t>(_processId)) : ProcessId{};
}

auto PosixSubprocessBackend::isRunning() -> bool {
    const auto lock = std::scoped_lock{_waitMutex};
    if (_exitStatus.has_value()) {
        return false;
    }
    return !pollStatus(WNOHANG);
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

void PosixSubprocessBackend::readCaptured(const int descriptor, const std::shared_ptr<OutputState> &state) noexcept {
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
            if (state->sink.error) {
                state->sink.error(PosixErrorContext::fromErrno());
            }
            break;
        }
        if (state->sink.output) {
            state->sink.output(state->standardError, std::string_view{buffer.data(), static_cast<std::size_t>(count)});
            continue;
        }
    }
    ::close(descriptor);
}

auto SubprocessBackend::create(
    const path::Path &executable,
    const text::StringList &arguments,
    const SubprocessOptions &options,
    const SubprocessOutputSink &sink) -> SubprocessBackendPtr {
    return std::make_unique<PosixSubprocessBackend>(executable, arguments, options, sink);
}

void PosixSubprocessBackend::startReader(int &descriptor, bool error, const SubprocessOutputSink &sink) {
    if (descriptor < 0) {
        return;
    }
    auto &state = error ? _standardError : _standardOutput;
    auto &reader = error ? _standardErrorReader : _standardOutputReader;
    state = std::make_shared<OutputState>(OutputState{sink, error});
    reader = std::thread{&PosixSubprocessBackend::readCaptured, descriptor, state};
    descriptor = -1;
}

}
