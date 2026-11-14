// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/event/impl/WindowsEventLoopNativeCalls.hpp>

#include <cerrno>

/// Scripts native errors while forwarding successful operations to the OS.
class WindowsEventLoopTestCalls final : public el::event::impl::WindowsEventLoopNativeCalls {
public:
    int wakeError{};
    int wakeInterrupts{};
    int wakeCalls{};
    int removalError{};
    int removalInterrupts{};
    int removalCalls{};
    int readError{};
    int readInterrupts{};
    bool shortWrite{};

    auto postWake(HANDLE port, ULONG_PTR key) noexcept -> BOOL override {
        ++wakeCalls;
        if (wakeError != 0) {
            SetLastError(static_cast<DWORD>(wakeError));
            return 0;
        }
        return WindowsEventLoopNativeCalls::postWake(port, key);
    }
};
