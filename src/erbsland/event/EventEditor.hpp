// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventSource_fwd.hpp"
#include "EventTarget_fwd.hpp"

namespace erbsland::event {

/// Common interface for editing handlers owned by an event source.
/// Sources own their editors and return them by reference. The source and target accessors exist primarily for
/// generic code that works with heterogeneous editors; regular code uses the typed `on...()` or `add...()` methods.
/// Access and editing require the source's owner loop unless the concrete editor documents thread-safe operations.
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
    /// Source ownership must remain valid for the editor's lifetime.
    /// @return The owning event source.
    [[nodiscard]] virtual auto source() const -> EventSourcePtr = 0;
    /// Retain and access the target on which callbacks are executed.
    /// @return The callback target.
    [[nodiscard]] virtual auto target() const noexcept -> EventTargetPtr = 0;

protected:
    // defaults
    EventEditor() = default;
};

}
