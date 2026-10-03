// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsEventLoopDriver.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../system/impl/WindowsErrorContext.hpp"
#include "../../system/PlatformError.hpp"
#include "../../text/Literals.hpp"

#include <chrono>
#include <limits>

namespace erbsland::event::impl {

using namespace text::literals;

WindowsEventLoopDriver::WindowsEventLoopDriver() :
    _completionPort{CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1)} {
    if (_completionPort == nullptr) {
        throw system::PlatformError{
            "Failed to create the IOCP event-loop driver."_el, system::impl::WindowsErrorContext::fromLastError()};
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
    // Waking is best-effort because the driver contract is noexcept and a failed port cannot be recovered here.
    PostQueuedCompletionStatus(_completionPort, 0, cWakeKey, nullptr);
}

auto WindowsEventLoopDriver::registerHandle(HANDLE handle, NativeCompletionCallback callback) -> RegistrationPtr {
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        throw err::ParameterFailure{"The native handle must be valid."_el, "handle"_el};
    }
    if (!callback) {
        throw err::ParameterFailure{"The native completion callback must not be empty."_el, "callback"_el};
    }
    const auto lock = std::scoped_lock{_registrationMutex};
    for (const auto &entry : _registrations) {
        if (entry.second.handle == handle) {
            throw err::ParameterFailure{"The native handle is already registered."_el, "handle"_el};
        }
    }
    const auto generation = _nextGeneration++;
    if (CreateIoCompletionPort(handle, _completionPort, generation, 0) == nullptr) {
        throw system::PlatformError{
            "Failed to associate a native handle with the event-loop completion port."_el,
            system::impl::WindowsErrorContext::fromLastError()};
    }
    _registrations.emplace(generation, Registration{handle, std::move(callback)});
    return std::make_unique<EventLoopDriverRegistration>(
        [this, generation]() noexcept -> void { unregisterHandle(generation); });
}

void WindowsEventLoopDriver::unregisterHandle(const ULONG_PTR generation) noexcept {
    const auto lock = std::scoped_lock{_registrationMutex};
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
        throw system::PlatformError{
            "The IOCP event-loop wait failed."_el, system::impl::WindowsErrorContext::fromErrorCode(error)};
    }
    auto callback = NativeCompletionCallback{};
    {
        const auto lock = std::scoped_lock{_registrationMutex};
        const auto iterator = _registrations.find(key);
        if (iterator == _registrations.end()) {
            return;
        }
        callback = iterator->second.callback;
    }
    callback(transferred, overlapped, error);
}

}
