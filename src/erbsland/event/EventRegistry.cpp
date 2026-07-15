// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventRegistry.hpp"

#include "../err/ParameterError.hpp"

#include <thread>
#include <utility>

namespace erbsland::event {

using namespace text::literals;

EventRegistry::EventRegistry(PrivateTag) {
    registerInternalIds();
}

auto EventRegistry::registerEvent(text::StringView name, text::StringView description) -> EventId {
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

auto EventRegistry::isRegistered(const EventId identifier) const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _eventInfo.contains(identifier);
}

auto EventRegistry::isRegistered(const text::StringView &name) const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _eventIds.contains(name);
}

auto EventRegistry::getEventId(const text::StringView &name) const noexcept -> EventId {
    std::scoped_lock lock{_mutex};
    if (const auto eventId = _eventIds.get(name); eventId.has_value()) {
        return eventId.value();
    }
    return noEvent();
}

auto EventRegistry::getEventInfo(const EventId identifier) const noexcept -> EventIdInfo {
    static auto noEventInfo = EventIdInfo{};
    std::scoped_lock lock{_mutex};
    return _eventInfo.get(identifier, noEventInfo);
}

auto EventRegistry::registerBackend(text::StringView name, text::StringView description) -> EventBackendId {
    std::scoped_lock lock{_mutex};
    static const auto validNameCharSet = text::CharSet::fromPattern("-_.a-zA-Z0-9"_el);
    if (!name.isValidUtf8() || !name.containsOnly(validNameCharSet)) {
        throw err::ParameterError{"The backend name contains invalid characters."_el, "name"_el};
    }
    if (name.length() > unit::ByteLength{800} || name.characterLength() > unit::CpLength{200}) {
        throw err::ParameterError{"The backend name exceeds the maximum length."_el, "name"_el};
    }
    if (!description.isValidUtf8()) {
        throw err::ParameterError{"The backend description contains invalid characters."_el, "description"_el};
    }
    if (description.length() > unit::ByteLength{8000} || description.characterLength() > unit::CpLength{2000}) {
        throw err::ParameterError{"The backend description exceeds the maximum length."_el, "description"_el};
    }
    if (_backendIds.contains(name)) {
        throw err::ParameterError{"The backend name is already registered."_el, "name"_el};
    }
    const auto newBackendValue = _nextCustomBackendValue;
    _nextCustomBackendValue += 1;
    const auto backendId = EventBackendId{newBackendValue};
    addBackend(backendId, std::move(name), std::move(description));
    return backendId;
}

auto EventRegistry::isRegistered(const EventBackendId identifier) const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _backendInfo.contains(identifier);
}

auto EventRegistry::isBackendRegistered(const text::StringView &name) const noexcept -> bool {
    std::scoped_lock lock{_mutex};
    return _backendIds.contains(name);
}

auto EventRegistry::getBackendId(const text::StringView &name) const noexcept -> EventBackendId {
    std::scoped_lock lock{_mutex};
    if (const auto backendId = _backendIds.get(name); backendId.has_value()) {
        return backendId.value();
    }
    return noBackend();
}

auto EventRegistry::getBackendInfo(const EventBackendId identifier) const noexcept -> EventBackendIdInfo {
    static auto noBackendInfo = EventBackendIdInfo{};
    std::scoped_lock lock{_mutex};
    return _backendInfo.get(identifier, noBackendInfo);
}

void EventRegistry::registerInternalIds() noexcept {
    addEvent(noEvent(), "dev.erbsland.core.NoEvent"_el, "No event"_el);
    addEvent(quit(), "dev.erbsland.core.Quit"_el, "Quits the event loop"_el);
    addEvent(invocation(), "dev.erbsland.core.Invocation"_el, "Invocation of a method from the event loop"_el);
    addEvent(timer(), "dev.erbsland.core.Timer"_el, "A timer event"_el);
    addBackend(noBackend(), "dev.erbsland.core.NoBackend"_el, "No backend"_el);
    addBackend(schedulerBackend(), "dev.erbsland.core.SchedulerBackend"_el, "Scheduler backend"_el);
}

void EventRegistry::addEvent(EventId eventId, text::StringView name, text::StringView description) {
    _eventIds.set(name, eventId);
    // created after eventId, to as `name` is moved into event info.
    auto eventInfo = EventIdInfo{std::move(name), std::move(description)};
    _eventInfo.set(eventId, std::move(eventInfo));
}

void EventRegistry::addBackend(EventBackendId backendId, text::StringView name, text::StringView description) {
    _backendIds.set(name, backendId);
    auto backendInfo = EventBackendIdInfo{std::move(name), std::move(description)};
    _backendInfo.set(backendId, std::move(backendInfo));
}

}
