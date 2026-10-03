// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventLoop.hpp"

#include "CallbackEventData.hpp"

#include "../EventQueueFullError.hpp"
#include "../EventRegistry.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::event::impl {

using namespace text::literals;
using time::TimeDelta;

void EventLoop::post(Event event) {
    postInternal(std::move(event));
}

auto EventLoop::postInternal(Event event) -> bool {
    if (event.identifier() == id::QuitEvent) {
        quit();
        return true;
    }
    ensureBackendsAttached();
    auto lock = std::unique_lock{_mutex};
    if (_quitRequested) {
        return false;
    }
    const auto maximum = _options.maximumQueuedEvents();
    if (maximum != 0 && _queue.size() >= maximum) {
        switch (_options.queueOverflowAction()) {
        case EventQueueOverflowAction::Terminate:
            std::terminate();
        case EventQueueOverflowAction::Ignore:
            return false;
        case EventQueueOverflowAction::Quit:
            _quitRequested = true;
            _quitEventQueued = true;
            lock.unlock();
            _queueSpace.notify_all();
            _driver->wake();
            return false;
        case EventQueueOverflowAction::FailApplication:
            _quitRequested = true;
            _quitEventQueued = true;
            lock.unlock();
            _queueSpace.notify_all();
            _driver->wake();
            failApplication(std::make_exception_ptr(EventQueueFullError{"The event queue is full."_el}));
            return false;
        case EventQueueOverflowAction::Block: {
            if (_consumerThread == std::this_thread::get_id()) {
                std::terminate();
            }
            const auto generation = _stopGeneration;
            _queueSpace.wait(lock, [this, maximum, generation]() -> bool {
                return _quitRequested || _stopRequested || _failureStopRequested || _stopGeneration != generation ||
                    _queue.size() < maximum;
            });
            if (_quitRequested || _stopRequested || _failureStopRequested || _stopGeneration != generation) {
                return false;
            }
            break;
        }
        }
    }
    _queue.emplace_back(std::move(event));
    lock.unlock();
    _driver->wake();
    return true;
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

auto EventLoop::postFromBackend(Event event) -> bool {
    return postInternal(std::move(event));
}

void EventLoop::wakeFromBackend() noexcept {
    _driver->wake();
}

auto EventLoop::queuedEventCount() const noexcept -> std::size_t {
    std::scoped_lock lock{_mutex};
    return _queue.size() + _continuations.size();
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

void EventLoop::captureError(std::exception_ptr error) noexcept {
    if (error == nullptr) {
        return;
    }
    std::scoped_lock lock{_mutex};
    _errors.push(std::move(error));
}

void EventLoop::handleError(std::exception_ptr error) noexcept {
    auto handler = EventLoopErrorHandler{};
    {
        std::scoped_lock lock{_mutex};
        handler = _errorHandler;
    }
    if (!handler) {
        switch (_options.errorPolicy()) {
        case EventLoopErrorPolicy::Ignore:
            return;
        case EventLoopErrorPolicy::Store:
            captureError(std::move(error));
            return;
        case EventLoopErrorPolicy::Quit:
            captureError(std::move(error));
            quit();
            return;
        case EventLoopErrorPolicy::FailApplication:
            failApplication(std::move(error));
            return;
        }
    }
    captureError(error);
    try {
        if (handler(std::move(error)) == EventLoopErrorAction::Stop) {
            stop();
        }
    } catch (const err::RuntimeError &) {
        captureError(std::current_exception());
        stop();
    }
}

void EventLoop::failApplication(std::exception_ptr error) noexcept {
    captureError(error);
    if (_failureHandler) {
        try {
            _failureHandler(std::move(error));
        } catch (const err::RuntimeError &) {
            captureError(std::current_exception());
        }
    }
    quit();
}

}
