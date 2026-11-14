// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/event/impl/KqueueEventLoopNativeCalls.hpp>

#include <cerrno>

/// Scripts native errors while forwarding successful operations to the OS.
class KqueueEventLoopTestCalls final : public el::event::impl::KqueueEventLoopNativeCalls {
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

    auto change(int queue, const struct kevent *changes, int count) noexcept -> int override {
        const auto wake = count == 1 && changes[0].filter == EVFILT_USER && (changes[0].fflags & NOTE_TRIGGER) != 0;
        const auto removal = count == 1 && (changes[0].flags & EV_DELETE) != 0;
        if (wake) {
            ++wakeCalls;
            if (wakeInterrupts > 0) {
                --wakeInterrupts;
                errno = EINTR;
                return -1;
            }
            if (wakeError != 0) {
                errno = wakeError;
                return -1;
            }
        }
        if (removal) {
            ++removalCalls;
            if (removalInterrupts > 0) {
                --removalInterrupts;
                errno = EINTR;
                return -1;
            }
            if (removalError != 0 && removalError != ENOENT) {
                errno = removalError;
                return -1;
            }
        }
        const auto result = KqueueEventLoopNativeCalls::change(queue, changes, count);
        if (removal && removalError == ENOENT && result == 0) {
            errno = ENOENT;
            return -1;
        }
        return result;
    }
};
