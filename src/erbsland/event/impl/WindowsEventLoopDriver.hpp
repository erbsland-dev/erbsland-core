// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsEventLoopDriver.hpp is only available on Windows."
#endif

#include "EventLoopDriverRegistration.hpp"

#include "../EventLoopDriver.hpp"

#include "../../core/impl/WindowsApi.hpp"

#include <functional>
#include <memory>
#include <unordered_map>

namespace erbsland::event::impl {

/// The Windows IOCP event-loop driver.
/// @tested{EventLoopDriverTest}
class WindowsEventLoopDriver final : public EventLoopDriver {
public:
    WindowsEventLoopDriver();
    ~WindowsEventLoopDriver() override;

    // defaults/deletions
    WindowsEventLoopDriver(const WindowsEventLoopDriver &) = delete;
    auto operator=(const WindowsEventLoopDriver &) -> WindowsEventLoopDriver & = delete;

public: // implement EventLoopDriver
    void wait() override;
    void wait(time::TimeDelta maximumWait) override;
    void wake() noexcept override;

public: // native source interface
    using NativeCompletionCallback = std::function<void(DWORD transferred, OVERLAPPED *overlapped, DWORD error)>;
    using RegistrationPtr = std::unique_ptr<EventLoopDriverRegistration>;

    /// Associate a native handle and its completions with this driver.
    [[nodiscard]] auto registerHandle(HANDLE handle, NativeCompletionCallback callback) -> RegistrationPtr;

private:
    void waitInternal(DWORD timeoutMilliseconds);
    void unregisterHandle(ULONG_PTR generation) noexcept;

    struct Registration {
        HANDLE handle;
        NativeCompletionCallback callback;
    };

private:
    HANDLE _completionPort{nullptr};                            ///< Native completion port.
    ULONG_PTR _nextGeneration{2U};                              ///< Next unique registration generation.
    std::unordered_map<ULONG_PTR, Registration> _registrations; ///< Active completion registrations.
};

}
