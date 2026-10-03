// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsSubprocessBackend.hpp"

#include "WindowsSubprocessLaunch.hpp"

#include "../ProcessIdAccess.hpp"
#include "../WindowsErrorContext.hpp"

#include "../../../text/Literals.hpp"
#include "../../PlatformError.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace erbsland::system::impl {

using namespace text::literals;

WindowsSubprocessBackend::WindowsSubprocessBackend(
    const path::Path &executable,
    const text::StringList &arguments,
    const SubprocessOptions &options,
    const SubprocessOutputSink &sink) {
    auto launch = WindowsSubprocessLaunch{};
    const auto process = launch.start(executable, arguments, options);
    _process = process.hProcess;
    _processId = process.dwProcessId;
    try {
        startReader(launch.outputHandle(), false, sink);
        startReader(launch.errorHandle(), true, sink);
    } catch (...) {
        TerminateProcess(_process, 1U);
        WaitForSingleObject(_process, INFINITE);
        joinReaders();
        CloseHandle(_process);
        _process = nullptr;
        throw;
    }
}

WindowsSubprocessBackend::~WindowsSubprocessBackend() {
    joinReaders();
    if (_process != nullptr) {
        CloseHandle(_process);
    }
}

auto WindowsSubprocessBackend::processId() const noexcept -> ProcessId {
    return ProcessIdAccess::fromNative(static_cast<std::uint64_t>(_processId));
}

auto WindowsSubprocessBackend::isRunning() -> bool {
    const auto lock = std::scoped_lock{_waitMutex};
    if (_exitStatus.has_value()) {
        return false;
    }
    const auto result = WaitForSingleObject(_process, 0U);
    if (result == WAIT_TIMEOUT) {
        return true;
    }
    if (result != WAIT_OBJECT_0) {
        throw PlatformError{"Failed while polling the subprocess."_el, WindowsErrorContext::fromLastError()};
    }
    storeStatus();
    return false;
}

auto WindowsSubprocessBackend::wait() -> SubprocessExitStatus {
    {
        const auto lock = std::scoped_lock{_waitMutex};
        if (!_exitStatus.has_value()) {
            const auto result = WaitForSingleObject(_process, INFINITE);
            if (result != WAIT_OBJECT_0) {
                throw PlatformError{
                    "Failed while waiting for the subprocess."_el, WindowsErrorContext::fromLastError()};
            }
            storeStatus();
        }
    }
    joinReaders();
    return *_exitStatus;
}

auto WindowsSubprocessBackend::wait(const time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> {
    const auto milliseconds = timeout.toMilliseconds().toRawValue();
    const auto nativeTimeout = static_cast<DWORD>(std::min<std::int64_t>(milliseconds, MAXDWORD - 1U));
    {
        const auto lock = std::scoped_lock{_waitMutex};
        if (!_exitStatus.has_value()) {
            const auto result = WaitForSingleObject(_process, nativeTimeout);
            if (result == WAIT_TIMEOUT) {
                return std::nullopt;
            }
            if (result != WAIT_OBJECT_0) {
                throw PlatformError{
                    "Failed while waiting for the subprocess."_el, WindowsErrorContext::fromLastError()};
            }
            storeStatus();
        }
    }
    joinReaders();
    return _exitStatus;
}

void WindowsSubprocessBackend::terminate() {
    if (_exitStatus.has_value() || TerminateProcess(_process, 1U)) {
        return;
    }
    throw PlatformError{"Failed to terminate the subprocess."_el, WindowsErrorContext::fromLastError()};
}

void WindowsSubprocessBackend::kill() {
    terminate();
}

void WindowsSubprocessBackend::storeStatus() {
    auto exitCode = DWORD{};
    if (!GetExitCodeProcess(_process, &exitCode)) {
        throw PlatformError{"Failed to read the subprocess exit code."_el, WindowsErrorContext::fromLastError()};
    }
    _exitStatus = SubprocessExitStatus::exited(static_cast<std::int32_t>(exitCode));
}

void WindowsSubprocessBackend::joinReaders() {
    if (_standardOutputReader.joinable()) {
        _standardOutputReader.join();
    }
    if (_standardErrorReader.joinable()) {
        _standardErrorReader.join();
    }
}

void WindowsSubprocessBackend::readCaptured(const HANDLE handle, const std::shared_ptr<OutputState> &state) noexcept {
    auto buffer = std::array<char, 8192>{};
    while (true) {
        auto count = DWORD{};
        if (!ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) {
            if (GetLastError() != ERROR_BROKEN_PIPE && state->sink.error) {
                state->sink.error(WindowsErrorContext::fromLastError());
            }
            break;
        }
        if (count == 0U) {
            break;
        }
        if (state->sink.output) {
            state->sink.output(state->standardError, std::string_view{buffer.data(), static_cast<std::size_t>(count)});
            continue;
        }
    }
    CloseHandle(handle);
}

auto SubprocessBackend::create(
    const path::Path &executable,
    const text::StringList &arguments,
    const SubprocessOptions &options,
    const SubprocessOutputSink &sink) -> SubprocessBackendPtr {
    return std::make_unique<WindowsSubprocessBackend>(executable, arguments, options, sink);
}

void WindowsSubprocessBackend::startReader(HANDLE &handle, bool error, const SubprocessOutputSink &sink) {
    if (handle == nullptr) {
        return;
    }
    auto &state = error ? _standardError : _standardOutput;
    auto &reader = error ? _standardErrorReader : _standardOutputReader;
    state = std::make_shared<OutputState>(OutputState{sink, error});
    reader = std::thread{&WindowsSubprocessBackend::readCaptured, handle, state};
    handle = nullptr;
}

}
