// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventSource.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::event {

using namespace text::literals;

EventSource::EventSource(EventsPtr ownerEvents) : _ownerEvents{std::move(ownerEvents)} {
    if (_ownerEvents == nullptr) {
        throw err::ParameterError{"The owner event target must not be null."_el, "ownerEvents"_el};
    }
}

auto EventSource::ownerEvents() const noexcept -> const EventsPtr & {
    return _ownerEvents;
}

auto EventSource::currentOwnerEvents() const -> EventsPtr {
    const auto current = currentEvents();
    if (current != _ownerEvents) {
        throw err::LogicError{"This event source operation must run on its owner loop."_el};
    }
    return current;
}

void EventSource::verifyCurrentOwnerEvents() const {
    if (currentEvents() != _ownerEvents) {
        throw err::LogicError{"This event source operation must run on its owner loop."_el};
    }
}

}
