// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <sys/epoll.h>
#include <sys/types.h>

#include <cstdint>

namespace erbsland::event::impl {

/// Injectable native operations for the Epoll event-loop driver.
/// @tested{EpollEventLoopDriverTest}
class EpollEventLoopNativeCalls {
public:
    virtual ~EpollEventLoopNativeCalls() = default;

public: // native operations
    /// Write one wake increment; follows write(2) return and errno conventions.
    /// @param descriptor The nonblocking wake descriptor.
    /// @param value The wake increment.
    /// @return Bytes written, or -1 with errno set.
    virtual auto writeWake(int descriptor, const uint64_t &value) noexcept -> ssize_t;
    /// Read wake increments; follows read(2) return and errno conventions.
    /// @param descriptor The nonblocking wake descriptor.
    /// @param value Receives the accumulated wake count.
    /// @return Bytes read, or -1 with errno set.
    virtual auto readWake(int descriptor, uint64_t &value) noexcept -> ssize_t;
    /// Apply an epoll registration change; follows epoll_ctl(2) conventions.
    /// @param queue The epoll descriptor.
    /// @param operation The registration operation.
    /// @param descriptor The monitored descriptor.
    /// @param event The registration details, or null for deletion.
    /// @return Zero on success, or -1 with errno set.
    virtual auto control(int queue, int operation, int descriptor, epoll_event *event) noexcept -> int;
};

}
