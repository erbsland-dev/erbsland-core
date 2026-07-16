// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventCallback.hpp"
#include "EventRegistry.hpp"
#include "EventScheduler_fwd.hpp"
#include "EventTimer_fwd.hpp"

#include "../time/TimeDelta.hpp"

namespace erbsland::event {

/// The public scheduler frontend for an events interface.
/// @tested{EventTimerTest EventBackendTest}
class EventScheduler {
public:
    /// The backend identifier for scheduler implementations.
    [[nodiscard]] constexpr static auto backendId() noexcept -> EventBackendId { return id::SchedulerBackend; }

public:
    virtual ~EventScheduler() = default;

public: // interface
    /// Invoke a callback after a delay.
    /// A zero or negative delay queues the callback immediately.
    /// @param delay The delay before queuing the callback.
    /// @param callback The callback to execute on the target event loop.
    virtual void invokeAfter(time::TimeDelta delay, EventCallback callback) = 0;
    /// Create an inactive timer.
    /// @param callback The callback to execute when the timer fires.
    [[nodiscard]] virtual auto createTimer(EventCallback callback) -> EventTimerPtr = 0;
};

}
