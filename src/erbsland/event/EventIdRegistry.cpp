// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventIdRegistry.hpp"

#include "../err/ParameterError.hpp"

#include <thread>
#include <utility>

namespace erbsland::event {

using namespace text::literals;

EventIdRegistry::EventIdRegistry(PrivateTag) {
    registerInternalIds();
}

auto EventIdRegistry::registerEvent(text::StringView name, text::StringView description) -> EventId {
    std::scoped_lock lock{_mutex};
    static const auto validNameCharSet = text::CharSet::fromPattern("-_.a-zA-Z0-9"_el);
    if (!name.isValidUtf8() || !name.containsOnly(validNameCharSet)) {
        throw err::ParameterError{"The event name contains invalid characters."_el, "name"_el};
    }
    if (name.length() > unit::ByteLength{800} || name.characterLength() > unit::CpLength{200}) {
        throw err::ParameterError{"The event name exceeds the maximum length."_el, "name"_el};
    }
    if (!description.isValidUtf8()) {
        throw err::ParameterError{"The event description contains invalid characters."_el, "description"_el};
    }
    if (description.length() > unit::ByteLength{8000} || description.characterLength() > unit::CpLength{2000}) {
        throw err::ParameterError{"The event description exceeds the maximum length."_el, "description"_el};
    }
    if (_eventIds.contains(name)) {
        throw err::ParameterError{"The event name is already registered."_el, "name"_el};
    }
    const auto newEventValue = _nextCustomEventValue;
    _nextCustomEventValue += 1;
    const auto eventId = EventId{newEventValue};
    addEvent(eventId, std::move(name), std::move(description));
    return eventId;
}

void EventIdRegistry::registerInternalIds() noexcept {
    addEvent(noEvent(), "NoEvent"_el, "No event"_el);
    addEvent(quit(), "Quit"_el, "Quits the event loop"_el);
    addEvent(invocation(), "Invocation"_el, "Invocation of a method from the event loop"_el);
    addEvent(timer(), "Timer"_el, "A timer event"_el);
}

void EventIdRegistry::addEvent(EventId eventId, text::StringView name, text::StringView description) {
    auto eventInfo = EventIdInfo{std::move(name), std::move(description)};
    _eventIds.set(name, eventId);
    _eventInfo.set(eventId, std::move(eventInfo));
}

}
