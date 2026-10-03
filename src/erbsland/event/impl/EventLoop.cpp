// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoop.hpp"

#include "CallbackEventData.hpp"
#include "EventLoopRunScope.hpp"
#include "SchedulerBackend.hpp"

#include "../EventRegistry.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../network/impl/NetworkBackend.hpp"
#include "../../system/impl/SystemBackend.hpp"
#include "../../text/Literals.hpp"
#include "../../util/impl/UtilitiesBackend.hpp"

#include <utility>

namespace erbsland::event::impl {

using namespace err;
using namespace text::literals;
using time::TimeDelta;
using time::TimePoint;

EventLoop::EventLoop(const EventLoopOptions &options, FailureHandler failureHandler) :
    EventLoop{EventLoopDriver::createDefault(), options, std::move(failureHandler)} {
}

EventLoop::EventLoop(EventLoopDriverPtr driver, const EventLoopOptions &options, FailureHandler failureHandler) :
    _options{options}, _failureHandler{std::move(failureHandler)}, _driver{std::move(driver)} {
    if (!_failureHandler &&
        (_options.errorPolicy() == EventLoopErrorPolicy::FailApplication ||
            _options.queueOverflowAction() == EventQueueOverflowAction::FailApplication)) {
        throw ParameterFailure{"FailApplication requires an application-managed event thread."_el, "options"_el};
    }
    switch (_options.queueOverflowAction()) {
    case EventQueueOverflowAction::Terminate:
    case EventQueueOverflowAction::FailApplication:
    case EventQueueOverflowAction::Quit:
    case EventQueueOverflowAction::Block:
    case EventQueueOverflowAction::Ignore:
        break;
    default:
        throw ParameterFailure{"Invalid event queue overflow action."_el, "options"_el};
    }
    switch (_options.errorPolicy()) {
    case EventLoopErrorPolicy::Ignore:
    case EventLoopErrorPolicy::Store:
    case EventLoopErrorPolicy::Quit:
    case EventLoopErrorPolicy::FailApplication:
        break;
    default:
        throw ParameterFailure{"Invalid event loop error policy."_el, "options"_el};
    }
    if (_driver == nullptr) {
        throw ParameterFailure{"The event-loop driver must not be null."_el, "driver"_el};
    }
}

void EventLoop::run() {
    auto runScope = EventLoopRunScope{*this};
    while (!isStopRequested()) {
        if (!runOnceImpl(std::nullopt) && isQuitRequested()) {
            break;
        }
    }
}

auto EventLoop::runOnce() -> bool {
    auto runScope = EventLoopRunScope{*this};
    return runOnceImpl(std::nullopt);
}

auto EventLoop::runOnce(const TimeDelta maximumWait) -> bool {
    auto runScope = EventLoopRunScope{*this};
    return runOnceImpl(maximumWait);
}

auto EventLoop::runUntilIdle() -> std::size_t {
    auto runScope = EventLoopRunScope{*this};
    auto result = std::size_t{0};
    while (runOnceImpl(TimeDelta::zero())) {
        ++result;
    }
    return result;
}

auto EventLoop::beginDispatch() -> CurrentEventsScope {
    ensureBackendsAttached();
    auto events = shared_from_this();
    std::scoped_lock lock{_mutex};
    if (_running) {
        throw LogicFailure{"The event loop is already running."_el};
    }
    _running = true;
    _consumerThread = std::this_thread::get_id();
    _stopRequested = false;
    return CurrentEventsScope{std::move(events)};
}

void EventLoop::stop() noexcept {
    {
        std::scoped_lock lock{_mutex};
        _stopRequested = true;
        ++_stopGeneration;
    }
    _queueSpace.notify_all();
    _driver->wake();
}

void EventLoop::finishDispatch() noexcept {
    std::scoped_lock lock{_mutex};
    _running = false;
    _consumerThread = {};
    _failureStopRequested = false;
}

void EventLoop::stopForFailure() noexcept {
    {
        std::scoped_lock lock{_mutex};
        _failureStopRequested = true;
        ++_stopGeneration;
    }
    _queueSpace.notify_all();
    _driver->wake();
}

void EventLoop::quit() noexcept {
    {
        std::scoped_lock lock{_mutex};
        if (!_quitRequested) {
            _quitRequested = true;
            _quitEventQueued = true;
        }
    }
    _queueSpace.notify_all();
    _driver->wake();
}

auto EventLoop::isRunning() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _running;
}

auto EventLoop::isQuitRequested() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _quitRequested;
}

void EventLoop::registerBackend(EventBackendPtr backend) {
    if (backend == nullptr) {
        throw ParameterFailure{"The backend must not be null."_el, "backend"_el};
    }
    if (!backend->backendId().isValid()) {
        throw ParameterFailure{"The backend identifier must not be `NoBackend`."_el, "backend"_el};
    }
    ensureBackendsAttached();
    {
        std::scoped_lock lock{_mutex};
        if (_running) {
            throw LogicFailure{"Can not register a backend while the event loop is running."_el};
        }
        for (const auto &registeredBackend : _backends) {
            if (registeredBackend->backendId() == backend->backendId()) {
                throw ParameterFailure{"A backend with this identifier is already registered."_el, "backend"_el};
            }
        }
        registerBackendInternal(std::move(backend));
    }
}

auto EventLoop::getBackend(const EventBackendId backendId) -> EventBackend & {
    if (!backendId.isValid()) {
        throw ParameterFailure{"The backend identifier must not be `NoBackend`."_el, "backendId"_el};
    }
    ensureBackendsAttached();
    std::scoped_lock lock{_mutex};
    for (const auto &backend : _backends) {
        if (backend->backendId() == backendId) {
            return *backend;
        }
    }
    auto backend = createFundamentalBackend(backendId);
    if (backend == nullptr) {
        throw ParameterFailure{"The requested backend is not available."_el, "backendId"_el};
    }
    // Returning the reference is safe, as the backend lifetime is bound to the event loop.
    return registerBackendInternal(std::move(backend));
}

auto EventLoop::registerBackendInternal(EventBackendPtr backend) -> EventBackend & {
    auto &result = *backend;
    if (_backendsAttached) {
        backend->attach(EventBackendTargetWeakPtr{weak_from_this()}, EventLoopDriverWeakPtr{_driver});
    }
    _backends.emplace_back(std::move(backend));
    // Returning the reference is safe, as the backend lifetime is bound to the event loop.
    return result;
}

auto EventLoop::createFundamentalBackend(const EventBackendId backendId) -> EventBackendPtr {
    if (backendId == id::SchedulerBackend) {
        return std::make_unique<SchedulerBackend>();
    }
    if (backendId == id::UtilitiesBackend) {
        return std::make_unique<util::impl::UtilitiesBackend>();
    }
    if (backendId == id::SystemBackend) {
        return std::make_unique<system::impl::SystemBackend>();
    }
    if (backendId == id::NetworkBackend) {
        return std::make_unique<network::impl::NetworkBackend>();
    }
    return {};
}

void EventLoop::ensureBackendsAttached() {
    std::scoped_lock lock{_mutex};
    if (_backendsAttached) {
        return;
    }
    const auto target = EventBackendTargetWeakPtr{weak_from_this()};
    for (const auto &backend : _backends) {
        backend->attach(target, EventLoopDriverWeakPtr{_driver});
    }
    _backendsAttached = true;
}

auto EventLoop::runOnceImpl(const std::optional<TimeDelta> maximumWait) -> bool {
    auto maximumWaitEnd = std::optional<TimePoint>{};
    const auto pollNativeOnce = maximumWait.has_value() && !maximumWait->isPositive();
    auto nativePollCompleted = false;
    if (maximumWait.has_value()) {
        if (maximumWait->isPositive()) {
            maximumWaitEnd = TimePoint::inFuture(*maximumWait);
        } else {
            maximumWaitEnd = TimePoint::now();
        }
    }

    while (true) {
        const auto now = TimePoint::now();
        if (isStopRequested()) {
            return false;
        }
        if (!isQuitRequested()) {
            pollBackends(now);
        }
        if (isStopRequested()) {
            return false;
        }
        if (auto event = takeNextEvent(); event.has_value()) {
            processEvent(*event);
            return true;
        }
        if (isQuitRequested()) {
            return false;
        }
        if (isStopRequested()) {
            return false;
        }
        if (maximumWaitEnd.has_value() && *maximumWaitEnd <= now) {
            if (pollNativeOnce && !nativePollCompleted) {
                waitForWake(TimeDelta::zero());
                nativePollCompleted = true;
                continue;
            }
            return false;
        }

        auto waitEnd = std::optional<TimePoint>{};
        const auto backendWakeTime = nextBackendWakeTime();
        if (backendWakeTime.has_value()) {
            waitEnd = *backendWakeTime;
        }
        if (maximumWaitEnd.has_value() && (!waitEnd.has_value() || *maximumWaitEnd < *waitEnd)) {
            waitEnd = *maximumWaitEnd;
        }

        if (!waitEnd.has_value()) {
            waitForWake();
            continue;
        }

        const auto waitStart = TimePoint::now();
        const auto waitTime = waitStart.timeDeltaTo(*waitEnd);
        if (waitTime.isPositive()) {
            waitForWake(waitTime);
            continue;
        }
        if (maximumWaitEnd.has_value() && *maximumWaitEnd <= waitStart) {
            return false;
        }
    }
}

auto EventLoop::takeNextEvent() -> std::optional<Event> {
    auto lock = std::unique_lock{_mutex};
    if (!_continuations.empty() && (_preferContinuation || _queue.empty())) {
        auto result = std::move(_continuations.front());
        _continuations.pop_front();
        _preferContinuation = false;
        return result;
    }
    if (_queue.empty()) {
        if (_quitEventQueued) {
            _quitEventQueued = false;
            return Event{id::QuitEvent};
        }
        return std::nullopt;
    }
    auto result = std::move(_queue.front());
    _queue.pop_front();
    _preferContinuation = true;
    lock.unlock();
    _queueSpace.notify_one();
    return result;
}

void EventLoop::clearQueuedEvents() noexcept {
    auto discarded = std::deque<Event>{};
    auto discardedContinuations = std::deque<Event>{};
    {
        std::scoped_lock lock{_mutex};
        discarded.swap(_queue);
        discardedContinuations.swap(_continuations);
    }
    _queueSpace.notify_all();
}

void EventLoop::processEvent(const Event &event) {
    if (event.identifier() == id::QuitEvent) {
        {
            std::scoped_lock lock{_mutex};
            _quitRequested = true;
            _quitEventQueued = false;
        }
        clearQueuedEvents();
        stop();
        return;
    }
    if (event.identifier() == id::InvocationEvent) {
        processInvocationEvent(event);
        return;
    }
    dispatchBackendEvent(event);
}

void EventLoop::processInvocationEvent(const Event &event) {
    if (event.data() == nullptr) {
        return;
    }
    const auto callbackData = dynamic_cast<const CallbackEventData *>(event.data().get());
    if (callbackData == nullptr || !callbackData->callback()) {
        return;
    }
    try {
        callbackData->callback()();
    } catch (const RuntimeError &) {
        handleError(std::current_exception());
    }
}

auto EventLoop::dispatchBackendEvent(const Event &event) -> bool {
    for (const auto backend : backendSnapshot()) {
        try {
            if (backend->handleEvent(event)) {
                return true;
            }
        } catch (const RuntimeError &) {
            handleError(std::current_exception());
            return true;
        }
    }
    return false;
}

auto EventLoop::isStopRequested() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _stopRequested || _failureStopRequested;
}

auto EventLoop::backendSnapshot() const -> std::vector<EventBackend *> {
    auto result = std::vector<EventBackend *>{};
    std::scoped_lock lock{_mutex};
    result.reserve(_backends.size());
    for (const auto &backend : _backends) {
        result.emplace_back(backend.get());
    }
    return result;
}

void EventLoop::pollBackends(const TimePoint now) {
    for (const auto backend : backendSnapshot()) {
        backend->poll(now);
    }
}

auto EventLoop::nextBackendWakeTime() const -> std::optional<TimePoint> {
    auto result = std::optional<TimePoint>{};
    for (const auto backend : backendSnapshot()) {
        const auto nextWakeTime = backend->nextWakeTime();
        if (!nextWakeTime.has_value()) {
            continue;
        }
        if (!result.has_value() || *nextWakeTime < *result) {
            result = *nextWakeTime;
        }
    }
    return result;
}

void EventLoop::waitForWake() {
    _driver->wait();
}

void EventLoop::waitForWake(const TimeDelta waitTime) {
    _driver->wait(waitTime);
}

}
