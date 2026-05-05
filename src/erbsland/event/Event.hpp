// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventData.hpp"
#include "EventIdentifier.hpp"

#include <chrono>
#include <memory>

namespace erbsland::event {

using EventClock = std::chrono::steady_clock;
using EventTime = EventClock::time_point;

/// A single event.
class Event {
public:
    /// Create an event of a given type.
    /// @param identifier The event identifier.
    explicit Event(const EventIdentifier identifier) noexcept : Event{identifier, {}} {}
    /// Create an event of a given type with optional data.
    /// @param identifier The event identifier.
    /// @param data Optional event data.
    Event(const EventIdentifier identifier, EventDataUniquePtr data) noexcept :
        _identifier{identifier}, _data{std::move(data)} {}
    /// Move constructor.
    Event(Event &&other) noexcept : _time{other._time}, _identifier{other._identifier}, _data{std::move(other._data)} {
        if (this == &other) {
            return;
        }
        other._identifier = cNoEventId;
    }
    /// Move assignment operator.
    auto operator=(Event &&other) noexcept -> Event & {
        if (this == &other) {
            return *this;
        }
        _time = other._time;
        _identifier = other._identifier;
        _data = std::move(other._data);
        other._identifier = cNoEventId;
        return *this;
    }

    // defaults
    ~Event() = default;
    Event(const Event &) = delete;
    auto operator=(const Event &) -> Event & = delete;

public:
    /// The event creation time.
    [[nodiscard]] auto time() const noexcept -> EventTime { return _time; }
    /// The event type.
    [[nodiscard]] auto type() const noexcept -> EventIdentifier { return _identifier; }
    /// Optional event data.
    [[nodiscard]] auto data() const noexcept -> const EventDataUniquePtr & { return _data; }

private:
    EventTime _time{EventClock::now()};      ///< Creation time for the event.
    EventIdentifier _identifier{cNoEventId}; ///< The event type.
    EventDataUniquePtr _data{nullptr};       ///< Optional event data.
};

}
