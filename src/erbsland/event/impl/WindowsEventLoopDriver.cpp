// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEventLoopDriver.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

#include <chrono>
#include <limits>

namespace erbsland::event::impl {

using namespace text::literals;

namespace {
constexpr auto cWakeKey = ULONG_PTR{1U};
}

WindowsEventLoopDriver::WindowsEventLoopDriver() :
    _completionPort{CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1)} {
    if (_completionPort == nullptr) {
        throw err::RuntimeError{"Failed to create the IOCP event-loop driver."};
    }
}

WindowsEventLoopDriver::~WindowsEventLoopDriver() {
    if (_completionPort != nullptr) {
        CloseHandle(_completionPort);
    }
}

void WindowsEventLoopDriver::wait() {
    waitInternal(INFINITE);
}

void WindowsEventLoopDriver::wait(const time::TimeDelta maximumWait) {
    if (!maximumWait.isPositive()) {
        waitInternal(0);
        return;
    }
    const auto nanoseconds = maximumWait.toStdNanoseconds();
    const auto roundedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        nanoseconds + std::chrono::milliseconds{1} - std::chrono::nanoseconds{1});
    const auto maximum = static_cast<int64_t>(std::numeric_limits<DWORD>::max() - 1U);
    waitInternal(static_cast<DWORD>(std::min<int64_t>(roundedMilliseconds.count(), maximum)));
}

void WindowsEventLoopDriver::wake() noexcept {
    static_cast<void>(PostQueuedCompletionStatus(_completionPort, 0, cWakeKey, nullptr));
}

auto WindowsEventLoopDriver::registerHandle(HANDLE handle, NativeCompletionCallback callback) -> RegistrationPtr {
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        throw err::ParameterError{"The native handle must be valid."_el, "handle"_el};
    }
    if (!callback) {
        throw err::ParameterError{"The native completion callback must not be empty."_el, "callback"_el};
    }
    for (const auto &[generation, registration] : _registrations) {
        static_cast<void>(generation);
        if (registration.handle == handle) {
            throw err::ParameterError{"The native handle is already registered."_el, "handle"_el};
        }
    }
    const auto generation = _nextGeneration++;
    if (CreateIoCompletionPort(handle, _completionPort, generation, 0) == nullptr) {
        throw err::RuntimeError{"Failed to associate a native handle with the event-loop completion port."_el};
    }
    _registrations.emplace(generation, Registration{handle, std::move(callback)});
    return std::make_unique<EventLoopDriverRegistration>(
        [this, generation]() noexcept -> void { unregisterHandle(generation); });
}

void WindowsEventLoopDriver::unregisterHandle(const ULONG_PTR generation) noexcept {
    _registrations.erase(generation);
}

void WindowsEventLoopDriver::waitInternal(const DWORD timeoutMilliseconds) {
    auto transferred = DWORD{};
    auto key = ULONG_PTR{};
    auto overlapped = LPOVERLAPPED{};
    const auto result =
        GetQueuedCompletionStatus(_completionPort, &transferred, &key, &overlapped, timeoutMilliseconds);
    const auto error = result != 0 ? DWORD{ERROR_SUCCESS} : GetLastError();
    if (result == 0 && overlapped == nullptr && error == WAIT_TIMEOUT) {
        return;
    }
    if (key == cWakeKey && overlapped == nullptr) {
        return;
    }
    if (result == 0 && overlapped == nullptr) {
        throw err::RuntimeError{"The IOCP event-loop wait failed."};
    }
    const auto iterator = _registrations.find(key);
    if (iterator == _registrations.end()) {
        return;
    }
    const auto callback = iterator->second.callback;
    callback(transferred, overlapped, error);
}

}
