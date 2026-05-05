// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventScheduler.hpp"

#include "impl/EventClockAccess.hpp"

#include <vector>

namespace erbsland::event {

void EventScheduler::schedule(EventIdentifier eventType, EventDataUniquePtr &&data, EventTime eventTime) {
    if (eventType == cQuitEventId || eventType == cNoEventId) {
        return;
    }
    const auto now = impl::EventClockAccess::now();
    if (eventTime < now) {
        eventTime = now;
    }
    if (eventTime - now > cMaximumTime) {
        return; // discard event
    }
    std::scoped_lock lock{_mutex};
    if (_events.size() >= cMaximumEvents) {
        return;
    }
    _events.emplace(eventTime, Event{eventType, std::move(data)});
}

void EventScheduler::poll() {
    auto readyEvents = std::vector<Event>{};
    {
        std::scoped_lock lock{_mutex};
        if (_events.empty()) {
            return;
        }
        const auto now = impl::EventClockAccess::now();
        while (!_events.empty() && _events.begin()->first <= now) {
            readyEvents.emplace_back(std::move(_events.begin()->second));
            _events.erase(_events.begin());
        }
    }
    for (auto &event : readyEvents) {
        _driver.addEvent(std::move(event));
    }
}

void EventScheduler::clear() {
    std::scoped_lock lock{_mutex};
    _events.clear();
}

auto EventScheduler::nextEventTime() const -> std::optional<EventTime> {
    std::scoped_lock lock{_mutex};
    if (_events.empty()) {
        return {};
    }
    return _events.begin()->first;
}

}
