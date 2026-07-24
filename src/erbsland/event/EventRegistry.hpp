// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventBackendId.hpp"
#include "EventBackendIdInfo.hpp"
#include "EventId.hpp"
#include "EventIdInfo.hpp"
#include "EventRegistry_fwd.hpp"

#include "../core/impl/ApplicationData_fwd.hpp"
#include "../text/String.hpp"
#include "../text/StringHashMap.hpp"

#include <mutex>

namespace erbsland::event {

/// The registry to manage event and backend ids.
class EventRegistry final {
    struct PrivateTag {};
    friend class core::impl::ApplicationData;
    constexpr static auto cCustomEventBase = 0x10000U;
    constexpr static auto cCustomBackendBase = 0x10000U;

public:
    /// Create the event registry.
    explicit EventRegistry(PrivateTag);

    // defaults
    ~EventRegistry() = default;
    EventRegistry(const EventRegistry &) = delete;
    EventRegistry(EventRegistry &&) = delete;
    auto operator=(const EventRegistry &) -> EventRegistry & = delete;
    auto operator=(EventRegistry &&) -> EventRegistry & = delete;

public:
    /// Register a custom event type.
    /// @param name Must be a reverse domain name. E.g. `com.example.myapp.MyEvent`.
    ///    Valid characters are `-_.a-zA-Z0-9`. The maximum length is 200 code-points.
    /// @param description A description of the event type. The maximum length is 2000 code-points.
    [[nodiscard]] auto registerEvent(text::String name, text::String description = {}) -> EventId;

    /// Test if an event is registered.
    /// @param identifier The event identifier.
    [[nodiscard]] auto isRegistered(EventId identifier) const noexcept -> bool;

    /// Test if an event is registered.
    /// @param name The event name.
    [[nodiscard]] auto isRegistered(const text::String &name) const noexcept -> bool;

    /// Get the event identifier from a given ID
    /// @param name The event name.
    /// @return The event identifier or `noEvent()` if the event is not registered.
    [[nodiscard]] auto getEventId(const text::String &name) const noexcept -> EventId;

    /// Access event information for a given ID
    /// @param identifier The event identifier.
    /// @return The event information or empty event information if the event is not registered.
    [[nodiscard]] auto getEventInfo(EventId identifier) const noexcept -> EventIdInfo;

    /// Register a custom backend type.
    /// @param name Must be a reverse domain name. E.g. `com.example.myapp.MyBackend`.
    ///    Valid characters are `-_.a-zA-Z0-9`. The maximum length is 200 code-points.
    /// @param description A description of the backend type. The maximum length is 2000 code-points.
    [[nodiscard]] auto registerBackend(text::String name, text::String description = {}) -> EventBackendId;

    /// Test if a backend is registered.
    /// @param identifier The backend identifier.
    [[nodiscard]] auto isRegistered(EventBackendId identifier) const noexcept -> bool;

    /// Test if a backend is registered.
    /// @param name The backend name.
    [[nodiscard]] auto isBackendRegistered(const text::String &name) const noexcept -> bool;

    /// Get the backend identifier from a given ID.
    /// @param name The backend name.
    /// @return The backend identifier or `noBackend()` if the backend is not registered.
    [[nodiscard]] auto getBackendId(const text::String &name) const noexcept -> EventBackendId;

    /// Access backend information for a given ID.
    /// @param identifier The backend identifier.
    /// @return The backend information or empty backend information if the backend is not registered.
    [[nodiscard]] auto getBackendInfo(EventBackendId identifier) const noexcept -> EventBackendIdInfo;

public:
    /// Get the no-event identifier.
    /// This is an alias for `EventId{}` for making code more readable.
    constexpr static auto noEvent() noexcept -> EventId { return EventId{}; }
    /// Get the quit event identifier.
    constexpr static auto quit() noexcept -> EventId { return EventId{1U}; }
    /// Get the invocation event identifier.
    constexpr static auto invocation() noexcept -> EventId { return EventId{2U}; }
    /// Get the timer event identifier.
    constexpr static auto timer() noexcept -> EventId { return EventId{3U}; }
    /// Get the no-backend identifier.
    /// This is an alias for `EventBackendId{}` for making code more readable.
    constexpr static auto noBackend() noexcept -> EventBackendId { return EventBackendId{}; }
    /// Get the scheduler backend identifier.
    constexpr static auto schedulerBackend() noexcept -> EventBackendId { return EventBackendId{1U}; }
    /// Get the network backend identifier.
    constexpr static auto networkBackend() noexcept -> EventBackendId { return EventBackendId{2U}; }

private:
    /// Register all internal identifiers.
    void registerInternalIds() noexcept;
    /// Add an event to the registry (unchecked).
    void addEvent(EventId eventId, text::String name, text::String description);
    /// Add a backend to the registry (unchecked).
    void addBackend(EventBackendId backendId, text::String name, text::String description);

private:
    mutable std::recursive_mutex _mutex;                               ///< The mutex to protect the registry.
    EventId::Value _nextCustomEventValue{cCustomEventBase};            ///< The next custom event value.
    EventBackendId::Value _nextCustomBackendValue{cCustomBackendBase}; ///< The next custom backend value.
    text::StringHashMap<EventId> _eventIds;                            ///< The map of event ids.
    util::HashMap<EventId, EventIdInfo> _eventInfo;                    ///< The map of event infos.
    text::StringHashMap<EventBackendId> _backendIds;                   ///< The map of backend ids.
    util::HashMap<EventBackendId, EventBackendIdInfo> _backendInfo;    ///< The map of backend infos.
};

namespace id {

constexpr static auto NoEvent = EventRegistry::noEvent();
constexpr static auto QuitEvent = EventRegistry::quit();
constexpr static auto InvocationEvent = EventRegistry::invocation();
constexpr static auto TimerEvent = EventRegistry::timer();

constexpr static auto NoBackend = EventRegistry::noBackend();
constexpr static auto SchedulerBackend = EventRegistry::schedulerBackend();
constexpr static auto NetworkBackend = EventRegistry::networkBackend();

}

}
