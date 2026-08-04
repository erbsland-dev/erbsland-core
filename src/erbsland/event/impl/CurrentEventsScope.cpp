// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CurrentEventsScope.hpp"

#include <utility>

namespace erbsland::event::impl {

thread_local EventsWeakPtr CurrentEventsScope::_currentEvents{};

CurrentEventsScope::CurrentEventsScope(EventsPtr events) noexcept : _previousEvents{_currentEvents} {
    _currentEvents = std::move(events);
}

CurrentEventsScope::~CurrentEventsScope() noexcept {
    _currentEvents = std::move(_previousEvents);
}

auto CurrentEventsScope::currentEventsWeakPtr() noexcept -> EventsWeakPtr {
    return _currentEvents;
}

auto currentEventsWeakPtr() noexcept -> EventsWeakPtr {
    return CurrentEventsScope::currentEventsWeakPtr();
}

}
