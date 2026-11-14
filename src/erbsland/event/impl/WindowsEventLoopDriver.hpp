// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsEventLoopDriver.hpp is only available on Windows."
#endif

#include "EventLoopDriverRegistration.hpp"
#include "WindowsEventLoopNativeCalls.hpp"

#include "../EventLoopDriver.hpp"

#include "../../core/impl/WindowsApi.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace erbsland::event::impl {

/// The Windows IOCP event-loop driver.
/// @tested{EventLoopDriverTest WindowsEventLoopDriverTest}
class WindowsEventLoopDriver final : public EventLoopDriver {
public:
    /// Create a Windows event-loop driver.
    /// @throws system::PlatformError If native event-loop initialization fails.
    WindowsEventLoopDriver();
    /// Create a driver using the given non-null native operations.
    /// @param nativeCalls The native operations retained by this driver.
    /// @throws system::PlatformError If native event-loop initialization fails.
    explicit WindowsEventLoopDriver(std::shared_ptr<WindowsEventLoopNativeCalls> nativeCalls);

    /// Release the native completion port and its registrations.
    ~WindowsEventLoopDriver() override;

    // defaults/deletions
    WindowsEventLoopDriver(const WindowsEventLoopDriver &) = delete;
    auto operator=(const WindowsEventLoopDriver &) -> WindowsEventLoopDriver & = delete;

public: // implement EventLoopDriver
    void wait() override;
    void wait(time::TimeDelta maximumWait) override;
    void wake() override;

public: // native source interface
    using NativeCompletionCallback = std::function<void(DWORD transferred, OVERLAPPED *overlapped, DWORD error)>;
    using RegistrationPtr = std::unique_ptr<EventLoopDriverRegistration>;

    /// Associate a native handle and its completions with this driver.
    /// @throws system::PlatformError If native event registration fails.
    [[nodiscard]] auto registerHandle(HANDLE handle, NativeCompletionCallback callback) -> RegistrationPtr;

private:
    static constexpr auto cWakeKey = ULONG_PTR{1U}; ///< Completion key reserved for wake events.

    /// Wait for native completions up to the given timeout.
    void waitInternal(DWORD timeoutMilliseconds);
    /// Remove a handle registration by generation.
    void unregisterHandle(ULONG_PTR generation) noexcept;

    /// A native-handle completion registration.
    struct Registration {
        HANDLE handle;
        NativeCompletionCallback callback;
    };

private:
    std::shared_ptr<WindowsEventLoopNativeCalls> _nativeCalls;  ///< Retained native call adapter.
    HANDLE _completionPort{nullptr};                            ///< Native completion port.
    mutable std::mutex _registrationMutex;                      ///< Protects cross-thread unregistration.
    ULONG_PTR _nextGeneration{2U};                              ///< Next unique registration generation.
    std::unordered_map<ULONG_PTR, Registration> _registrations; ///< Active completion registrations.
};

}
