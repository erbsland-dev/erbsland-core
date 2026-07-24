// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventEditor_fwd.hpp"
#include "Events_fwd.hpp"

#include <atomic>

namespace erbsland::event {

/// A lifetime-controlled callback editor for an event source.
///
/// The source keeps only a weak reference to the editor. Retain the editor for as long as its callbacks shall remain
/// connected. Releasing the editor or calling `disconnect()` removes the complete callback group.
/// @tested{EventSourceTest}
class EventEditor {
public:
    virtual ~EventEditor();

    // defaults/deletions
    EventEditor(const EventEditor &) = delete;
    EventEditor(EventEditor &&) = delete;
    auto operator=(const EventEditor &) -> EventEditor & = delete;
    auto operator=(EventEditor &&) -> EventEditor & = delete;

public:
    /// Disconnect all callbacks represented by this editor.
    void disconnect() noexcept;
    /// Test if this editor is still connected.
    [[nodiscard]] auto isConnected() const noexcept -> bool;
    /// Access the target on which callbacks are executed.
    [[nodiscard]] auto targetEvents() const noexcept -> const EventsPtr &;

protected:
    /// Create an editor attached to an event target.
    explicit EventEditor(EventsPtr targetEvents);

private:
    EventsPtr _targetEvents;            ///< Target for callbacks.
    std::atomic<bool> _connected{true}; ///< Connection state.
};

}
