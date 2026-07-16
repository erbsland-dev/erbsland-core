// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventTimer.hpp"
#include "TimerEventData_fwd.hpp"

#include "../EventData.hpp"

#include <utility>

namespace erbsland::event::impl {

/// Event data for due timers.
/// @tested{EventTimerTest}
class TimerEventData final : public EventData {
public:
    /// Create timer event data.
    TimerEventData(impl::EventTimerPtr timer, impl::EventTimer::CallbackExecution execution) :
        _timer{std::move(timer)}, _execution{std::move(execution)} {}

public: // accessors
    /// Get the timer.
    [[nodiscard]] auto timer() const noexcept -> const impl::EventTimerPtr & { return _timer; }
    /// Get the prepared callback execution.
    [[nodiscard]] auto execution() const noexcept -> const impl::EventTimer::CallbackExecution & { return _execution; }

private:
    impl::EventTimerPtr _timer;                     ///< The timer that became due.
    impl::EventTimer::CallbackExecution _execution; ///< The prepared callback execution.
};

}
