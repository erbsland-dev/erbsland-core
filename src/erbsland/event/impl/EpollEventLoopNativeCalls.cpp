// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EpollEventLoopNativeCalls.hpp"

#include <sys/epoll.h>
#include <unistd.h>

namespace erbsland::event::impl {

auto EpollEventLoopNativeCalls::writeWake(const int descriptor, const uint64_t &value) noexcept -> ssize_t {
    return ::write(descriptor, &value, sizeof(value));
}

auto EpollEventLoopNativeCalls::readWake(const int descriptor, uint64_t &value) noexcept -> ssize_t {
    return ::read(descriptor, &value, sizeof(value));
}

auto EpollEventLoopNativeCalls::control(
    const int queue, const int operation, const int descriptor, epoll_event *event) noexcept -> int {
    return ::epoll_ctl(queue, operation, descriptor, event);
}

}
