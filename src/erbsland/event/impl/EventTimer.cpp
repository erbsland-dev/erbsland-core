// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventTimer.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::event::impl {

using namespace text::literals;

EventTimer::EventTimer(PrivateTag, EventCallback callback, ScheduleCallback scheduleCallback) :
    _callback{std::move(callback)}, _scheduleCallback{std::move(scheduleCallback)} {
}

auto EventTimer::create(EventCallback callback, ScheduleCallback scheduleCallback) -> EventTimerPtr {
    return std::make_shared<EventTimer>(PrivateTag{}, std::move(callback), std::move(scheduleCallback));
}

void EventTimer::startOnce(const time::TimeDelta delay) {
    start(EventTimerMode::Once, delay);
}

void EventTimer::startFixedDelay(const time::TimeDelta interval) {
    if (!interval.isPositive()) {
        throw err::ParameterError{"The timer interval must be positive."_el, "interval"_el};
    }
    start(EventTimerMode::FixedDelay, interval);
}

void EventTimer::startFixedRate(const time::TimeDelta interval) {
    if (!interval.isPositive()) {
        throw err::ParameterError{"The timer interval must be positive."_el, "interval"_el};
    }
    start(EventTimerMode::FixedRate, interval);
}

void EventTimer::stop() noexcept {
    std::scoped_lock lock{_mutex};
    _active = false;
    _pending = false;
    _mode = EventTimerMode::Inactive;
    _generation += 1;
}

auto EventTimer::isActive() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _active || _pending;
}

auto EventTimer::mode() const noexcept -> EventTimerMode {
    std::scoped_lock lock{_mutex};
    return _mode;
}

auto EventTimer::interval() const noexcept -> time::TimeDelta {
    std::scoped_lock lock{_mutex};
    return _interval;
}

void EventTimer::start(const EventTimerMode mode, const time::TimeDelta interval) {
    {
        std::scoped_lock lock{_mutex};
        _mode = mode;
        _interval = interval;
        _nextWakeTime = interval.isPositive() ? time::TimePoint::inFuture(interval) : time::TimePoint::now();
        _active = true;
        _pending = false;
        _generation += 1;
    }
    scheduleSelf();
}

auto EventTimer::backendNextWakeTime() const noexcept -> std::optional<time::TimePoint> {
    std::scoped_lock lock{_mutex};
    if (!_active) {
        return std::nullopt;
    }
    return _nextWakeTime;
}

auto EventTimer::prepareCallback(const time::TimePoint now) noexcept -> std::optional<CallbackExecution> {
    std::scoped_lock lock{_mutex};
    if (!_active || _nextWakeTime > now) {
        return std::nullopt;
    }
    _active = false;
    _pending = true;
    _lastDueTime = _nextWakeTime;
    return CallbackExecution{
        .callback = _callback,
        .mode = _mode,
        .dueTime = _lastDueTime,
        .generation = _generation,
    };
}

auto EventTimer::isCallbackPending(const CallbackExecution &execution) const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _pending && _generation == execution.generation && _mode == execution.mode;
}

void EventTimer::finishCallback(const CallbackExecution &execution, const time::TimePoint now) noexcept {
    auto scheduleAgain = false;
    {
        std::scoped_lock lock{_mutex};
        if (!_pending || _generation != execution.generation || _mode != execution.mode) {
            _pending = false;
            return;
        }
        _pending = false;
        switch (_mode) {
        case EventTimerMode::Inactive:
        case EventTimerMode::Once:
            _mode = EventTimerMode::Inactive;
            break;
        case EventTimerMode::FixedDelay:
            _nextWakeTime = now + _interval;
            _active = true;
            scheduleAgain = true;
            break;
        case EventTimerMode::FixedRate:
            _nextWakeTime = execution.dueTime + _interval;
            while (_nextWakeTime <= now) {
                _nextWakeTime += _interval;
            }
            _active = true;
            scheduleAgain = true;
            break;
        }
    }
    if (scheduleAgain) {
        scheduleSelf();
    }
}

void EventTimer::scheduleSelf() {
    if (_scheduleCallback) {
        _scheduleCallback(shared_from_this());
    }
}

}
