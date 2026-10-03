// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CurrentEvents.hpp"
#include "EventEditor.hpp"
#include "Events_fwd.hpp"
#include "EventSource_fwd.hpp"

#include <memory>

namespace erbsland::event {

/// Base class for shared event sources owned by one event loop.
/// Each concrete source owns one stable event editor and exposes it through `events()`. Returning a reference makes
/// the editor's non-owning role explicit, while `EventEditor::source()` lets generic code retain the source if needed.
/// @tested{EventSourceTest SubprocessEventTest}
class EventSource : public std::enable_shared_from_this<EventSource> {
public:
    // defaults/deletions
    virtual ~EventSource() = default;
    EventSource(const EventSource &) = delete;
    EventSource(EventSource &&) = delete;
    auto operator=(const EventSource &) -> EventSource & = delete;
    auto operator=(EventSource &&) -> EventSource & = delete;

public: // accessors
    /// Access the event loop that owns this source.
    [[nodiscard]] auto ownerEvents() const noexcept -> const EventsPtr &;

public: // interface
    /// Access the stable editor owned by this source.
    /// Must be called on the owner event loop.
    /// @return The source-owned editor.
    [[nodiscard]] virtual auto events() -> EventEditor & = 0;

protected:
    /// Prepare a source whose optional event owner will be bound later.
    EventSource() = default;
    /// Bind an unowned source once to a non-null event target.
    /// @param ownerEvents The target to bind.
    void bindOwnerEvents(EventsPtr ownerEvents);
    /// Create an event source owned by an event target.
    explicit EventSource(EventsPtr ownerEvents);
    /// Get the current event target after verifying that it owns this source.
    /// Must be called on the owner event loop.
    [[nodiscard]] auto currentOwnerEvents() const -> EventsPtr;
    /// Verify that the current event loop owns this source.
    /// Must be called on the owner event loop.
    void verifyCurrentOwnerEvents() const;

private:
    EventsPtr _ownerEvents; ///< Owner event target.
};

}
