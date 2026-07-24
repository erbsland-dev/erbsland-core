// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoop.hpp"

#include "CallbackEventData.hpp"
#include "CurrentEventsScope.hpp"
#include "SchedulerBackend.hpp"

#include "../EventRegistry.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::event::impl {

using namespace text::literals;
using time::TimeDelta;
using time::TimePoint;

EventLoop::EventLoop() : EventLoop{EventLoopDriver::createDefault()} {
}

EventLoop::EventLoop(EventLoopDriverPtr driver) : _driver{std::move(driver)} {
    if (_driver == nullptr) {
        throw err::ParameterError{"The event-loop driver must not be null."_el, "driver"_el};
    }
}

void EventLoop::post(Event event) {
    postInternal(std::move(event), false);
}

void EventLoop::postInternal(Event event, const bool allowAfterQuit) {
    ensureBackendsAttached();
    {
        std::scoped_lock lock{_mutex};
        if (_quitRequested && !allowAfterQuit) {
            return;
        }
        _queue.emplace_back(std::move(event));
    }
    _driver->wake();
}

void EventLoop::invoke(EventCallback callback) {
    post(Event{id::InvocationEvent, std::make_unique<CallbackEventData>(std::move(callback))});
}

void EventLoop::invokeAfter(const TimeDelta delay, EventCallback callback) {
    if (!delay.isPositive()) {
        invoke(std::move(callback));
        return;
    }
    get<EventScheduler>().invokeAfter(delay, std::move(callback));
}

void EventLoop::postFromBackend(Event event) {
    post(std::move(event));
}

void EventLoop::wakeFromBackend() noexcept {
    _driver->wake();
}

void EventLoop::run() {
    ensureBackendsAttached();
    auto currentEventsScope = CurrentEventsScope{shared_from_this()};
    {
        std::scoped_lock lock{_mutex};
        if (_running) {
            throw err::LogicError{"The event loop is already running."_el};
        }
        _running = true;
        _stopRequested = false;
    }
    try {
        while (!isStopRequested()) {
            if (!runOnceImpl(std::nullopt) && isQuitRequested()) {
                break;
            }
        }
    } catch (...) {
        std::scoped_lock lock{_mutex};
        _running = false;
        throw;
    }
    std::scoped_lock lock{_mutex};
    _running = false;
}

auto EventLoop::runOnce() -> bool {
    ensureBackendsAttached();
    auto currentEventsScope = CurrentEventsScope{shared_from_this()};
    {
        std::scoped_lock lock{_mutex};
        if (_running) {
            throw err::LogicError{"The event loop is already running."_el};
        }
        _running = true;
        _stopRequested = false;
    }
    auto result = false;
    try {
        result = runOnceImpl(std::nullopt);
    } catch (...) {
        std::scoped_lock lock{_mutex};
        _running = false;
        throw;
    }
    std::scoped_lock lock{_mutex};
    _running = false;
    return result;
}

auto EventLoop::runOnce(const TimeDelta maximumWait) -> bool {
    ensureBackendsAttached();
    auto currentEventsScope = CurrentEventsScope{shared_from_this()};
    {
        std::scoped_lock lock{_mutex};
        if (_running) {
            throw err::LogicError{"The event loop is already running."_el};
        }
        _running = true;
        _stopRequested = false;
    }
    auto result = false;
    try {
        result = runOnceImpl(maximumWait);
    } catch (...) {
        std::scoped_lock lock{_mutex};
        _running = false;
        throw;
    }
    std::scoped_lock lock{_mutex};
    _running = false;
    return result;
}

auto EventLoop::runUntilIdle() -> std::size_t {
    ensureBackendsAttached();
    auto currentEventsScope = CurrentEventsScope{shared_from_this()};
    {
        std::scoped_lock lock{_mutex};
        if (_running) {
            throw err::LogicError{"The event loop is already running."_el};
        }
        _running = true;
        _stopRequested = false;
    }
    auto result = std::size_t{0};
    try {
        while (runOnceImpl(TimeDelta::zero())) {
            result += 1U;
        }
    } catch (...) {
        std::scoped_lock lock{_mutex};
        _running = false;
        throw;
    }
    std::scoped_lock lock{_mutex};
    _running = false;
    return result;
}

void EventLoop::stop() noexcept {
    {
        std::scoped_lock lock{_mutex};
        _stopRequested = true;
    }
    _driver->wake();
}

void EventLoop::quit() noexcept {
    auto queueQuitEvent = false;
    {
        std::scoped_lock lock{_mutex};
        if (!_quitEventQueued) {
            _quitRequested = true;
            _quitEventQueued = true;
            queueQuitEvent = true;
        }
    }
    if (queueQuitEvent) {
        try {
            postInternal(Event{id::QuitEvent}, true);
        } catch (...) {
            stop();
        }
        return;
    }
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

auto EventLoop::hasError() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return !_errors.empty();
}

auto EventLoop::takeError() noexcept -> std::exception_ptr {
    std::scoped_lock lock{_mutex};
    if (_errors.empty()) {
        return {};
    }
    auto result = _errors.front();
    _errors.pop();
    return result;
}

void EventLoop::setErrorHandler(EventLoopErrorHandler handler) {
    std::scoped_lock lock{_mutex};
    _errorHandler = std::move(handler);
}

void EventLoop::registerBackend(EventBackendPtr backend) {
    if (backend == nullptr) {
        throw err::ParameterError{"The backend must not be null."_el, "backend"_el};
    }
    if (!backend->backendId().isValid()) {
        throw err::ParameterError{"The backend identifier must not be `NoBackend`."_el, "backend"_el};
    }
    ensureBackendsAttached();
    {
        std::scoped_lock lock{_mutex};
        if (_running) {
            throw err::LogicError{"Can not register a backend while the event loop is running."_el};
        }
        for (const auto &registeredBackend : _backends) {
            if (registeredBackend->backendId() == backend->backendId()) {
                throw err::ParameterError{"A backend with this identifier is already registered."_el, "backend"_el};
            }
        }
        static_cast<void>(registerBackendInternal(std::move(backend)));
    }
}

auto EventLoop::getBackend(const EventBackendId backendId) -> EventBackend & {
    if (!backendId.isValid()) {
        throw err::ParameterError{"The backend identifier must not be `NoBackend`."_el, "backendId"_el};
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
        throw err::ParameterError{"The requested backend is not available."_el, "backendId"_el};
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
        pollBackends(now);
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
    std::scoped_lock lock{_mutex};
    if (_queue.empty()) {
        return std::nullopt;
    }
    auto result = std::move(_queue.front());
    _queue.pop_front();
    return result;
}

void EventLoop::clearQueuedEvents() noexcept {
    std::scoped_lock lock{_mutex};
    _queue.clear();
}

void EventLoop::processEvent(const Event &event) {
    if (event.identifier() == id::QuitEvent) {
        {
            std::scoped_lock lock{_mutex};
            _quitRequested = true;
            _quitEventQueued = true;
        }
        clearQueuedEvents();
        stop();
        return;
    }
    if (event.identifier() == id::InvocationEvent) {
        processInvocationEvent(event);
        return;
    }
    static_cast<void>(dispatchBackendEvent(event));
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
    } catch (...) {
        handleError(std::current_exception());
    }
}

auto EventLoop::dispatchBackendEvent(const Event &event) -> bool {
    for (const auto backend : backendSnapshot()) {
        try {
            if (backend->handleEvent(event)) {
                return true;
            }
        } catch (...) {
            handleError(std::current_exception());
            return true;
        }
    }
    return false;
}

void EventLoop::captureError(std::exception_ptr error) noexcept {
    if (error == nullptr) {
        return;
    }
    std::scoped_lock lock{_mutex};
    _errors.push(std::move(error));
}

void EventLoop::handleError(std::exception_ptr error) noexcept {
    captureError(error);
    auto handler = EventLoopErrorHandler{};
    {
        std::scoped_lock lock{_mutex};
        handler = _errorHandler;
    }
    if (!handler) {
        return;
    }
    try {
        if (handler(std::move(error)) == EventLoopErrorAction::Stop) {
            stop();
        }
    } catch (...) {
        captureError(std::current_exception());
        stop();
    }
}

auto EventLoop::isStopRequested() const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _stopRequested;
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
