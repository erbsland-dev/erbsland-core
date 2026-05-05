// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventThread.hpp"

#include "impl/InvocationEvent.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace erbsland::event {

auto EventThread::create() -> EventThreadPtr {
    auto eventThread = std::make_shared<EventThread>(ProtectedTag{});
    eventThread->start();
    return eventThread;
}

EventThread::EventThread(ProtectedTag protectedTag) noexcept : _eventDriver{}, _eventScheduler{_eventDriver} {
    static_cast<void>(protectedTag);
}

void EventThread::invoke(InvocationFn fn) {
    invokeImpl([fn = std::move(fn)](const StopToken) -> void {
        if (fn) {
            fn();
        }
    });
}

void EventThread::invoke(StoppableInvocationFn fn) {
    invokeImpl(std::move(fn));
}

void EventThread::invokeDelayed(InvocationFn fn, const milliseconds delay) {
    invokeDelayedImpl(
        [fn = std::move(fn)](const StopToken) -> void {
            if (fn) {
                fn();
            }
        },
        delay);
}

void EventThread::invokeDelayed(StoppableInvocationFn fn, const milliseconds delay) {
    invokeDelayedImpl(std::move(fn), delay);
}

void EventThread::quit() {
    {
        std::scoped_lock lock{_mutex};
        if (_quitRequested || _abortRequested || _finished) {
            return;
        }
        _quitRequested = true;
        _wakeupRequested = true;
    }
    _eventScheduler.clear();
    requestActiveStop();
    _eventDriver.quit();
    _wakeupCv.notify_all();
}

void EventThread::abort() {
    {
        std::scoped_lock lock{_mutex};
        if (_abortRequested || _finished) {
            return;
        }
        _abortRequested = true;
        _wakeupRequested = true;
    }
    _eventScheduler.clear();
    requestActiveStop();
    _wakeupCv.notify_all();
}

auto EventThread::waitForQuit(milliseconds timeout) -> bool {
    if (timeout < milliseconds::zero()) {
        timeout = milliseconds::zero();
    }
    auto lock = std::unique_lock{_mutex};
    if (!_finished && std::this_thread::get_id() == _workerThreadId) {
        throw std::logic_error{"EventThread::waitForQuit() must not be called from the event thread itself."};
    }
    if (!_finished && !_finishedCv.wait_for(lock, timeout, [this]() -> bool { return _finished; })) {
        return false;
    }
    lock.unlock();
    rethrowFailureIfNeeded();
    return true;
}

auto EventThread::quitAndWait(const milliseconds timeout) -> bool {
    quit();
    return waitForQuit(timeout);
}

auto EventThread::isRunning() const -> bool {
    std::scoped_lock lock{_mutex};
    return !_finished;
}

auto EventThread::isQuitRequested() const -> bool {
    std::scoped_lock lock{_mutex};
    return _quitRequested;
}

auto EventThread::isAbortRequested() const -> bool {
    std::scoped_lock lock{_mutex};
    return _abortRequested;
}

auto EventThread::isFinished() const -> bool {
    std::scoped_lock lock{_mutex};
    return _finished;
}

auto EventThread::hasFailed() const -> bool {
    std::scoped_lock lock{_mutex};
    return _failure != nullptr;
}

void EventThread::start() {
    auto self = shared_from_this();
    std::thread([self = std::move(self)]() mutable -> void { self->runWorker(); }).detach();
}

void EventThread::runWorker() {
    {
        std::scoped_lock lock{_mutex};
        _workerThreadId = std::this_thread::get_id();
    }
    _eventDriver.setEventHandler([this](const Event &event) -> void { handleEvent(event); });
    try {
        while (processLoopIteration()) {}
    } catch (...) {
        std::scoped_lock lock{_mutex};
        if (_failure == nullptr) {
            _failure = std::current_exception();
        }
        _abortRequested = true;
        _wakeupRequested = true;
    }
    finishWorker();
}

auto EventThread::processLoopIteration() -> bool {
    _eventScheduler.poll();
    while (true) {
        if (isAbortRequested()) {
            return false;
        }
        const auto result = _eventDriver.processOneEvent();
        switch (result.status()) {
        case EventDriver::ProcessResult::Processed:
            continue;
        case EventDriver::ProcessResult::Quit:
            return false;
        case EventDriver::ProcessResult::Idle:
        case EventDriver::ProcessResult::Timeout:
            break;
        }
        break;
    }
    if (isAbortRequested()) {
        return false;
    }
    const auto waitDuration = nextWaitDuration();
    auto lock = std::unique_lock{_mutex};
    if (_abortRequested) {
        return false;
    }
    if (_wakeupRequested) {
        _wakeupRequested = false;
        return true;
    }
    if (!waitDuration.has_value()) {
        _wakeupCv.wait(
            lock, [this]() -> bool { return _abortRequested || _wakeupRequested || _eventDriver.hasPendingEvents(); });
    } else if (*waitDuration > milliseconds::zero()) {
        _wakeupCv.wait_for(lock, *waitDuration, [this]() -> bool {
            return _abortRequested || _wakeupRequested || _eventDriver.hasPendingEvents();
        });
    }
    _wakeupRequested = false;
    return !_abortRequested;
}

void EventThread::handleEvent(const Event &event) {
    if (event.type() != EventIdentifier::Invocation) {
        return;
    }
    auto *invocationEvent = dynamic_cast<impl::InvocationEvent *>(event.data().get());
    if (invocationEvent == nullptr) {
        return;
    }
    auto stopSource = StopSource{};
    {
        std::scoped_lock lock{_mutex};
        _activeStopSource = stopSource;
        if (_quitRequested || _abortRequested) {
            _activeStopSource->requestStop();
        }
    }
    try {
        invocationEvent->invoke(stopSource.getToken());
    } catch (...) {
        std::scoped_lock lock{_mutex};
        _activeStopSource.reset();
        throw;
    }
    std::scoped_lock lock{_mutex};
    _activeStopSource.reset();
}

void EventThread::invokeImpl(StoppableInvocationFn fn) {
    if (!fn) {
        return;
    }
    {
        std::scoped_lock lock{_mutex};
        if (_quitRequested || _abortRequested || _finished || _failure != nullptr) {
            return;
        }
        _eventDriver.addEvent(EventIdentifier::Invocation, createInvocationEvent(std::move(fn)));
        _wakeupRequested = true;
    }
    _wakeupCv.notify_all();
}

void EventThread::invokeDelayedImpl(StoppableInvocationFn fn, milliseconds delay) {
    if (!fn) {
        return;
    }
    {
        std::scoped_lock lock{_mutex};
        if (_quitRequested || _abortRequested || _finished || _failure != nullptr) {
            return;
        }
        auto eventTime = impl::EventClockAccess::now();
        if (delay > milliseconds::zero()) {
            eventTime += delay;
        }
        _eventScheduler.schedule(EventIdentifier::Invocation, createInvocationEvent(std::move(fn)), eventTime);
        _wakeupRequested = true;
    }
    _wakeupCv.notify_all();
}

auto EventThread::createInvocationEvent(StoppableInvocationFn fn) -> EventDataUniquePtr {
    return std::make_unique<impl::InvocationEvent>(std::move(fn));
}

void EventThread::wakeWorker() {
    {
        std::scoped_lock lock{_mutex};
        _wakeupRequested = true;
    }
    _wakeupCv.notify_all();
}

auto EventThread::nextWaitDuration() const -> std::optional<milliseconds> {
    if (_eventDriver.hasPendingEvents()) {
        return milliseconds::zero();
    }
    const auto nextEventTime = _eventScheduler.nextEventTime();
    if (!nextEventTime.has_value()) {
        return std::nullopt;
    }
    const auto now = impl::EventClockAccess::now();
    if (*nextEventTime <= now) {
        return milliseconds::zero();
    }
    const auto duration = std::chrono::duration_cast<milliseconds>(*nextEventTime - now);
    if (duration <= milliseconds::zero()) {
        return milliseconds::zero();
    }
    return std::min(duration, cMaximumWaitSlice);
}

void EventThread::requestActiveStop() {
    auto stopSource = std::optional<StopSource>{};
    {
        std::scoped_lock lock{_mutex};
        if (_activeStopSource.has_value()) {
            stopSource = *_activeStopSource;
        }
    }
    if (stopSource.has_value()) {
        stopSource->requestStop();
    }
}

void EventThread::finishWorker() noexcept {
    auto callback = FinishedCallback{};
    {
        std::scoped_lock lock{_mutex};
        _activeStopSource.reset();
        _finished = true;
        _wakeupRequested = true;
        callback = _finishedCallback;
    }
    _finishedCv.notify_all();
    if (callback) {
        try {
            callback();
        } catch (...) {}
    }
}

void EventThread::rethrowFailureIfNeeded() const {
    auto failure = std::exception_ptr{};
    {
        std::scoped_lock lock{_mutex};
        failure = _failure;
    }
    if (failure != nullptr) {
        std::rethrow_exception(failure);
    }
}

void EventThread::setFinishedCallback(FinishedCallback callback) {
    auto runImmediately = false;
    {
        std::scoped_lock lock{_mutex};
        if (_finished) {
            runImmediately = true;
        } else {
            _finishedCallback = std::move(callback);
        }
    }
    if (runImmediately && callback) {
        callback();
    }
}

}
