// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/event/impl/EpollEventLoopNativeCalls.hpp>
#include <sys/epoll.h>

#include <cerrno>

/// Scripts native errors while forwarding successful operations to the OS.
class EpollEventLoopTestCalls final : public el::event::impl::EpollEventLoopNativeCalls {
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

    auto writeWake(int descriptor, const uint64_t &value) noexcept -> ssize_t override {
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
        if (shortWrite) {
            return 0;
        }
        return EpollEventLoopNativeCalls::writeWake(descriptor, value);
    }
    auto readWake(int descriptor, uint64_t &value) noexcept -> ssize_t override {
        if (readInterrupts > 0) {
            --readInterrupts;
            errno = EINTR;
            return -1;
        }
        if (readError != 0) {
            errno = readError;
            return -1;
        }
        return EpollEventLoopNativeCalls::readWake(descriptor, value);
    }
    auto control(int queue, int operation, int descriptor, epoll_event *event) noexcept -> int override {
        if (operation == EPOLL_CTL_DEL) {
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
        const auto result = EpollEventLoopNativeCalls::control(queue, operation, descriptor, event);
        if (operation == EPOLL_CTL_DEL && removalError == ENOENT && result == 0) {
            errno = ENOENT;
            return -1;
        }
        return result;
    }
};
