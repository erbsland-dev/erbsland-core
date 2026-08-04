// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Events_fwd.hpp"
#include "EventSource_fwd.hpp"

namespace erbsland::event {

/// Common interface for editing handlers owned by an event source.
/// Sources own their editors and return them by reference. The source and target accessors exist primarily for
/// generic code that works with heterogeneous editors; regular code uses the typed `on...()` methods.
/// @tested{EventSourceTest}
class EventEditor {
public:
    /// Destroy the event editor.
    virtual ~EventEditor();

    // defaults/deletions
    EventEditor(const EventEditor &) = delete;
    EventEditor(EventEditor &&) = delete;
    auto operator=(const EventEditor &) -> EventEditor & = delete;
    auto operator=(EventEditor &&) -> EventEditor & = delete;

public: // interface
    /// Retain and access the source that owns this editor.
    /// @return The owning event source.
    /// @throws err::LogicError If the editor cannot provide its source because of an internal lifetime error.
    [[nodiscard]] virtual auto source() const -> EventSourcePtr = 0;
    /// Retain and access the target on which callbacks are executed.
    /// @return The callback target.
    [[nodiscard]] virtual auto target() const noexcept -> EventsPtr = 0;

protected:
    // defaults
    EventEditor() = default;
};

}
