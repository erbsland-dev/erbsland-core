// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventDriver.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace erbsland::event {

class EventScheduler;
using EventSchedulerPtr = std::unique_ptr<EventScheduler>;

/// A scheduler for events.
/// Stores events until a given time and add them to the event driver.
/// - thread-safe.
/// - limited to a maximum of 10'000 scheduled events. If more events are added, they are discarded.
/// - limited to a scheduling time of 0-12h.
class EventScheduler {
    /// The maximum number of scheduled events.
    static constexpr std::size_t cMaximumEvents = 10'000;
    /// The maximum scheduling time.
    static constexpr auto cMaximumTime = std::chrono::hours{12};
    /// The map type for storing events with their scheduled time.
    using SchedulingMap = std::multimap<EventTime, Event>;

public:
    /// Create a new scheduler using the given driver.
    /// @param driver The event driver to add events to.
    explicit EventScheduler(EventDriver &driver) : _driver{driver} {};

    // defaults
    ~EventScheduler() = default;
    EventScheduler(const EventScheduler &) = delete;
    EventScheduler(EventScheduler &&) = delete;
    auto operator=(const EventScheduler &) -> EventScheduler & = delete;
    auto operator=(EventScheduler &&) -> EventScheduler & = delete;

public:
    /// Schedule an event.
    /// @param eventType The event type.
    /// @param data The event data.
    /// @param eventTime The time when the event should be scheduled.
    void schedule(EventIdentifier eventType, EventDataUniquePtr &&data, EventTime eventTime);

    /// Poll the scheduler.
    /// If an event is ready, it is removed from the scheduler and passed to the event driver.
    void poll();
    /// Remove all scheduled events.
    void clear();
    /// Get the time of the next scheduled event.
    /// @return The next scheduled event time, or an empty optional if no events are pending.
    [[nodiscard]] auto nextEventTime() const -> std::optional<EventTime>;

private:
    EventDriver &_driver;      ///< The event driver to add events to.
    mutable std::mutex _mutex; ///< The mutex to protect the scheduler.
    SchedulingMap _events;     ///< The map with all scheduled events.
};

}
