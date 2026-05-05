// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventDriver.hpp"

#include "QuitEvent.hpp"

#include <atomic>
#include <stdexcept>
#include <thread>

namespace erbsland::event {

using namespace std::chrono_literals;

void EventDriver::addEvent(EventIdentifier eventType, EventDataUniquePtr &&eventData) {
    std::scoped_lock lock(_mutex);
    _eventQueue.emplace_back(eventType, std::move(eventData));
}

void EventDriver::addEvent(Event &&event) {
    std::scoped_lock lock(_mutex);
    _eventQueue.emplace_back(std::move(event));
}

void EventDriver::setEventHandler(std::function<void(const Event &)> eventHandler) {
    std::scoped_lock lock(_mutex);
    _eventHandler = std::move(eventHandler);
}

auto EventDriver::hasPendingEvents() const -> bool {
    std::scoped_lock lock(_mutex);
    return !_eventQueue.empty() || _state.load(std::memory_order_acquire) == State::Quit;
}

auto EventDriver::run() -> ProcessResult {
    for (;;) {
        const auto result = processEvents(10ms);
        if (result.status() == ProcessResult::Quit) {
            return result;
        }
        std::this_thread::sleep_for(10ms);
    }
}

auto EventDriver::processEvents(const std::chrono::milliseconds timeout) -> ProcessResult {
    const auto endTime = EventClock::now() + timeout;
    while (timeout == 0ms || EventClock::now() < endTime) {
        const auto result = processOneEvent();
        switch (result.status()) {
        case ProcessResult::Processed:
            std::this_thread::yield();
            continue;
        case ProcessResult::Quit:
            return result;
        case ProcessResult::Idle:
            return ProcessResult{ProcessResult::Idle};
        case ProcessResult::Timeout:
            break;
        }
    }
    return ProcessResult{ProcessResult::Timeout};
}

auto EventDriver::processOneEvent() -> ProcessResult {
    Event nextEvent{EventIdentifier::NoEvent};
    {
        std::scoped_lock lock(_mutex);
        if (_state.load(std::memory_order_acquire) == State::Quit) {
            return ProcessResult{ProcessResult::Quit, _exitCode};
        }
        if (_eventQueue.empty()) {
            return ProcessResult{ProcessResult::Idle};
        }
        nextEvent = std::move(_eventQueue.front());
        _eventQueue.pop_front();
    }
    if (nextEvent.type() != EventIdentifier::NoEvent) {
        processEvent(nextEvent);
    }
    if (_state.load(std::memory_order_acquire) == State::Quit) {
        return ProcessResult{ProcessResult::Quit, _exitCode};
    }
    return ProcessResult{ProcessResult::Processed};
}

void EventDriver::processEvent(const Event &event) {
    switch (event.type()) {
    case EventIdentifier::Quit:
        _state.store(State::Quit, std::memory_order_release);
        if (auto quitEvent = dynamic_cast<QuitEvent *>(event.data().get()); quitEvent != nullptr) {
            _exitCode = quitEvent->exitCode();
        } else {
            _exitCode = 0;
        }
        break;
    default:
        if (_eventHandler) {
            _eventHandler(event);
        }
        break;
    }
}

void EventDriver::quit(int exitCode) {
    addEvent(EventIdentifier::Quit, std::make_unique<QuitEvent>(exitCode));
}

}
