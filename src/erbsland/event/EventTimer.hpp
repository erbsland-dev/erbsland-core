// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventTimer_fwd.hpp"
#include "EventTimerMode.hpp"

#include "../time/TimeDelta.hpp"

namespace erbsland::event {

/// A thread-safe timer for scheduled event callbacks.
/// @tested{EventTimerTest}
class EventTimer {
public:
    virtual ~EventTimer() = default;

public:
    /// Start this timer once after `delay`.
    /// A zero or negative delay schedules the timer for immediate execution.
    /// @param delay The delay before executing the callback.
    virtual void startOnce(time::TimeDelta delay) = 0;
    /// Start this timer repeatedly with a fixed delay after each callback finishes.
    /// @param interval The positive interval between callback completion and the next execution.
    /// @throws err::ParameterError If `interval` is zero or negative.
    virtual void startFixedDelay(time::TimeDelta interval) = 0;
    /// Start this timer repeatedly with a fixed rate.
    /// Missed ticks are skipped and never queued as catch-up bursts.
    /// @param interval The positive interval between scheduled callback times.
    /// @throws err::ParameterError If `interval` is zero or negative.
    virtual void startFixedRate(time::TimeDelta interval) = 0;
    /// Stop this timer.
    virtual void stop() noexcept = 0;

public: // accessors
    /// Test if this timer is active or has a callback queued for execution.
    [[nodiscard]] virtual auto isActive() const noexcept -> bool = 0;
    /// Get the current timer mode.
    [[nodiscard]] virtual auto mode() const noexcept -> EventTimerMode = 0;
    /// Get the current delay or interval.
    [[nodiscard]] virtual auto interval() const noexcept -> time::TimeDelta = 0;
};

}
