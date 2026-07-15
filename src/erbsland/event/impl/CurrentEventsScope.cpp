// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CurrentEventsScope.hpp"

#include <utility>

namespace erbsland::event::impl {

namespace {
thread_local auto currentEvents = EventsWeakPtr{};
}

CurrentEventsScope::CurrentEventsScope(EventsPtr events) noexcept : _previousEvents{currentEvents} {
    currentEvents = std::move(events);
}

CurrentEventsScope::~CurrentEventsScope() noexcept {
    currentEvents = std::move(_previousEvents);
}

auto currentEventsWeakPtr() noexcept -> EventsWeakPtr {
    return currentEvents;
}

}
