// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SchedulerBackend.hpp"

#include "CallbackEventData.hpp"
#include "EventTimer.hpp"
#include "TimerEventData.hpp"

#include "../Event.hpp"
#include "../EventBackendTarget.hpp"
#include "../EventRegistry.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::event::impl {

SchedulerBackend::SchedulerBackend() : _state{std::make_shared<State>()} {
}

auto SchedulerBackend::backendId() const noexcept -> EventBackendId {
    return EventScheduler::backendId();
}

void SchedulerBackend::attach(EventBackendTargetWeakPtr target, [[maybe_unused]] EventLoopDriverWeakPtr driver) {
    std::scoped_lock lock{_state->mutex};
    _state->target = std::move(target);
}

void SchedulerBackend::poll(const time::TimePoint now) {
    auto dueTimers = std::vector<std::pair<impl::EventTimerPtr, impl::EventTimer::CallbackExecution>>{};
    auto dueInvocations = std::vector<EventCallback>{};
    auto target = EventBackendTargetPtr{};
    {
        std::scoped_lock lock{_state->mutex};
        target = _state->target.lock();

        auto delayedInvocations = std::vector<DelayedInvocation>{};
        delayedInvocations.reserve(_state->delayedInvocations.size());
        for (auto &delayedInvocation : _state->delayedInvocations) {
            if (delayedInvocation.dueTime <= now) {
                dueInvocations.emplace_back(std::move(delayedInvocation.callback));
                continue;
            }
            delayedInvocations.emplace_back(std::move(delayedInvocation));
        }
        _state->delayedInvocations = std::move(delayedInvocations);

        auto nextTimers = std::vector<impl::EventTimerWeakPtr>{};
        nextTimers.reserve(_state->timers.size());
        for (const auto &weakTimer : _state->timers) {
            const auto timer = weakTimer.lock();
            if (timer == nullptr) {
                continue;
            }
            if (auto execution = timer->prepareCallback(now); execution.has_value()) {
                dueTimers.emplace_back(timer, std::move(*execution));
                continue;
            }
            if (timer->backendNextWakeTime().has_value()) {
                nextTimers.emplace_back(timer);
            }
        }
        _state->timers = std::move(nextTimers);
    }
    if (target == nullptr) {
        return;
    }
    for (auto &callback : dueInvocations) {
        target->postFromBackend(Event{id::InvocationEvent, std::make_unique<CallbackEventData>(std::move(callback))});
    }
    for (auto &[timer, execution] : dueTimers) {
        target->postFromBackend(
            Event{id::TimerEvent, std::make_unique<TimerEventData>(std::move(timer), std::move(execution))});
    }
}

auto SchedulerBackend::handleEvent(const Event &event) -> bool {
    if (event.identifier() != id::TimerEvent) {
        return false;
    }
    if (event.data() == nullptr) {
        return true;
    }
    const auto timerData = dynamic_cast<const TimerEventData *>(event.data().get());
    if (timerData == nullptr || timerData->timer() == nullptr) {
        return true;
    }
    const auto &timer = timerData->timer();
    const auto &execution = timerData->execution();
    if (!timer->isCallbackPending(execution)) {
        return true;
    }
    auto callbackError = std::exception_ptr{};
    if (execution.callback) {
        try {
            execution.callback();
        } catch (...) {
            callbackError = std::current_exception();
        }
    }
    timer->finishCallback(execution, time::TimePoint::now());
    if (callbackError != nullptr) {
        std::rethrow_exception(callbackError);
    }
    return true;
}

auto SchedulerBackend::nextWakeTime() const -> std::optional<time::TimePoint> {
    auto result = std::optional<time::TimePoint>{};
    std::scoped_lock lock{_state->mutex};
    for (const auto &delayedInvocation : _state->delayedInvocations) {
        if (!result.has_value() || delayedInvocation.dueTime < *result) {
            result = delayedInvocation.dueTime;
        }
    }
    for (const auto &weakTimer : _state->timers) {
        const auto timer = weakTimer.lock();
        if (timer == nullptr) {
            continue;
        }
        const auto nextWakeTime = timer->backendNextWakeTime();
        if (!nextWakeTime.has_value()) {
            continue;
        }
        if (!result.has_value() || *nextWakeTime < *result) {
            result = *nextWakeTime;
        }
    }
    return result;
}

void SchedulerBackend::invokeAfter(const time::TimeDelta delay, EventCallback callback) {
    auto target = EventBackendTargetPtr{};
    if (!delay.isPositive()) {
        {
            std::scoped_lock lock{_state->mutex};
            target = _state->target.lock();
        }
        if (target != nullptr) {
            target->postFromBackend(
                Event{id::InvocationEvent, std::make_unique<CallbackEventData>(std::move(callback))});
        }
        return;
    }
    {
        std::scoped_lock lock{_state->mutex};
        _state->delayedInvocations.emplace_back(
            DelayedInvocation{
                .dueTime = time::TimePoint::inFuture(delay),
                .callback = std::move(callback),
            });
    }
    wakeAttachedTarget(_state);
}

auto SchedulerBackend::createTimer(EventCallback callback) -> event::EventTimerPtr {
    const auto weakState = std::weak_ptr<State>{_state};
    return impl::EventTimer::create(std::move(callback), [weakState](const impl::EventTimerPtr &timer) -> void {
        const auto state = weakState.lock();
        if (state == nullptr) {
            return;
        }
        schedule(state, timer);
        wakeAttachedTarget(state);
    });
}

auto SchedulerBackend::hasTarget() const noexcept -> bool {
    std::scoped_lock lock{_state->mutex};
    return !_state->target.expired();
}

void SchedulerBackend::schedule(const std::shared_ptr<State> &state, const impl::EventTimerPtr &timer) {
    std::scoped_lock lock{state->mutex};
    auto nextTimers = std::vector<impl::EventTimerWeakPtr>{};
    nextTimers.reserve(state->timers.size() + 1U);
    for (const auto &weakTimer : state->timers) {
        const auto existingTimer = weakTimer.lock();
        if (existingTimer == nullptr || existingTimer == timer) {
            continue;
        }
        if (existingTimer->backendNextWakeTime().has_value()) {
            nextTimers.emplace_back(existingTimer);
        }
    }
    if (timer != nullptr && timer->backendNextWakeTime().has_value()) {
        nextTimers.emplace_back(timer);
    }
    state->timers = std::move(nextTimers);
}

void SchedulerBackend::wakeAttachedTarget(const std::shared_ptr<State> &state) noexcept {
    auto target = EventBackendTargetPtr{};
    {
        std::scoped_lock lock{state->mutex};
        target = state->target.lock();
    }
    if (target != nullptr) {
        target->wakeFromBackend();
    }
}

}
