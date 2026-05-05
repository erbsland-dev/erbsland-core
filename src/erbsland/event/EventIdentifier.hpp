// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::event {

/// The event type
class EventIdentifier {
public:
    /// Create a "no-event" event type.
    constexpr EventIdentifier() = default;
    /// Create an internal event with a specific identifier.
    /// @param componentId The identifier for the component.
    /// @param eventId The identifier for the event.
    constexpr explicit EventIdentifier(const uint16_t componentId, const uint16_t eventId) :
        _componentId{componentId}, _eventId{eventId} {}

    // defaults
    EventIdentifier(const EventIdentifier &) = default;
    auto operator=(const EventIdentifier &) -> EventIdentifier & = default;
    auto operator==(const EventIdentifier &) const -> bool = default;
    auto operator!=(const EventIdentifier &) const -> bool = default;

public:
    /// Test if this is a "no-event" event type.
    [[nodiscard]] constexpr auto isNoEvent() const -> bool { return _componentId == 0 && _eventId == 0; }

private:
    uint16_t _componentId{0};
    uint16_t _eventId{0};
};

constexpr static uint16_t cCoreComponentId = 0;

/// The identifier for no event.
constexpr static auto cNoEventId = EventIdentifier{};
/// The identifier for a quit event.
constexpr static auto cQuitEventId = EventIdentifier{cCoreComponentId, 1};

}
