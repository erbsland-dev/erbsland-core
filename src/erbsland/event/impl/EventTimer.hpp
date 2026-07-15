// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventTimer_fwd.hpp"
#include "TimerEventData_fwd.hpp"

#include "../EventCallback.hpp"
#include "../EventTimer.hpp"
#include "../EventTimerMode.hpp"

#include "../../time/TimeDelta.hpp"
#include "../../time/TimePoint.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>

namespace erbsland::event::impl {

/// The implementation of a scheduled event timer.
/// @tested{EventTimerTest}
class EventTimer final : public event::EventTimer, public std::enable_shared_from_this<EventTimer> {
    friend class TimerEventData;

public:
    struct CallbackExecution {
        EventCallback callback;
        EventTimerMode mode{EventTimerMode::Inactive};
        time::TimePoint dueTime;
        uint64_t generation{0};
    };
    using ScheduleCallback = std::function<void(const EventTimerPtr &)>;

    struct PrivateTag {};

public:
    /// Create a timer with a private construction tag.
    EventTimer(PrivateTag, EventCallback callback, ScheduleCallback scheduleCallback);
    /// Create a timer with a fixed callback.
    /// @param callback The callback to execute when the timer fires.
    /// @param scheduleCallback The callback used to schedule this timer in a backend.
    [[nodiscard]] static auto create(EventCallback callback, ScheduleCallback scheduleCallback) -> EventTimerPtr;

    // defaults
    ~EventTimer() override = default;
    EventTimer(const EventTimer &) = delete;
    EventTimer(EventTimer &&) = delete;
    auto operator=(const EventTimer &) -> EventTimer & = delete;
    auto operator=(EventTimer &&) -> EventTimer & = delete;

public: // implement event::EventTimer
    void startOnce(time::TimeDelta delay) override;
    void startFixedDelay(time::TimeDelta interval) override;
    void startFixedRate(time::TimeDelta interval) override;
    void stop() noexcept override;
    [[nodiscard]] auto isActive() const noexcept -> bool override;
    [[nodiscard]] auto mode() const noexcept -> EventTimerMode override;
    [[nodiscard]] auto interval() const noexcept -> time::TimeDelta override;

public:
    /// Get the next wake time requested by the backend.
    [[nodiscard]] auto backendNextWakeTime() const noexcept -> std::optional<time::TimePoint>;
    /// Prepare a callback if this timer is due.
    [[nodiscard]] auto prepareCallback(time::TimePoint now) noexcept -> std::optional<CallbackExecution>;
    /// Test if a prepared callback is still pending.
    [[nodiscard]] auto isCallbackPending(const CallbackExecution &execution) const noexcept -> bool;
    /// Finish a callback and reschedule if required.
    void finishCallback(const CallbackExecution &execution, time::TimePoint now) noexcept;

private:
    void start(EventTimerMode mode, time::TimeDelta interval);
    void scheduleSelf();

private:
    mutable std::mutex _mutex;                      ///< Protects all timer state.
    EventCallback _callback;                        ///< The fixed callback.
    ScheduleCallback _scheduleCallback;             ///< Schedules this timer in the loop backend.
    EventTimerMode _mode{EventTimerMode::Inactive}; ///< Current timer mode.
    time::TimeDelta _interval;                      ///< Current delay or interval.
    time::TimePoint _nextWakeTime;                  ///< The next scheduled wake time.
    time::TimePoint _lastDueTime;                   ///< The scheduled time of the current callback.
    uint64_t _generation{0};                        ///< Incremented for every external state change.
    bool _active{false};                            ///< True while waiting in a backend.
    bool _pending{false};                           ///< True while a due callback is queued in the loop.
};

}
