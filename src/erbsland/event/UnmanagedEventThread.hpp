// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventLoopOptions.hpp"
#include "EventThread.hpp"

namespace erbsland::event {

/// A standalone thread that runs an event loop.
/// Unmanaged event threads are not registered with `core::Application` and are not quit by application shutdown.
/// @tested{EventThreadTest ApplicationEventTest}
class UnmanagedEventThread : public EventThread {
public:
    // defaults
    ~UnmanagedEventThread() override = default;

public:
    /// Create an unmanaged event thread.
    /// @param options Capacity and default error policies, copied into the loop. Must be valid and must not require
    /// managed-application delivery.
    /// @return The new, unstarted event thread.
    [[nodiscard]] static auto create(const EventLoopOptions &options = {}) -> UnmanagedEventThreadPtr;
};

}
