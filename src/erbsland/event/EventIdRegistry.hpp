// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventId.hpp"
#include "EventIdInfo.hpp"

#include "../core/impl/ApplicationData_fwd.hpp"
#include "../text/StringHashMap.hpp"
#include "../text/StringView.hpp"

#include <mutex>

namespace erbsland::event {

/// The registry to manage event ids.
class EventIdRegistry final {
    struct PrivateTag {};
    friend class core::impl::ApplicationData;
    constexpr static auto cCustomEventBase = 0x10000U;

public:
    /// Create the event id registry.
    explicit EventIdRegistry(PrivateTag);

    // defaults
    ~EventIdRegistry() = default;
    EventIdRegistry(const EventIdRegistry &) = delete;
    EventIdRegistry(EventIdRegistry &&) = delete;
    auto operator=(const EventIdRegistry &) -> EventIdRegistry & = delete;
    auto operator=(EventIdRegistry &&) -> EventIdRegistry & = delete;

public:
    /// Register a custom event type.
    /// @param name Must be a reverse domain name. E.g. `com.example.myapp.MyEvent`.
    ///    Valid characters are `-_.a-zA-Z0-9`. The maximum length is 200 code-points.
    /// @param description A description of the event type. The maximum length is 2000 code-points.
    [[nodiscard]] auto registerEvent(text::StringView name, text::StringView description = {}) -> EventId;

public:
    constexpr static auto noEvent() noexcept -> EventId { return EventId{}; }
    constexpr static auto quit() noexcept -> EventId { return EventId{1U}; }
    constexpr static auto invocation() noexcept -> EventId { return EventId{2U}; }
    constexpr static auto timer() noexcept -> EventId { return EventId{3U}; }

public:
    void registerInternalIds() noexcept;
    void addEvent(EventId eventId, text::StringView name, text::StringView description);

private:
    std::recursive_mutex _mutex;                            ///< The mutex to protect the registry.
    EventId::Value _nextCustomEventValue{cCustomEventBase}; ///< The next custom event value.
    text::StringHashMap<EventId> _eventIds;                 ///< The map of event ids.
    util::HashMap<EventId, EventIdInfo> _eventInfo;         ///< The map of event infos.
};

namespace id {

constexpr static auto NoEvent = EventIdRegistry::noEvent();
constexpr static auto QuitEvent = EventIdRegistry::quit();
constexpr static auto InvocationEvent = EventIdRegistry::invocation();
constexpr static auto TimerEvent = EventIdRegistry::timer();

}

}
