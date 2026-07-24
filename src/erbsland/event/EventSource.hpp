// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CurrentEvents.hpp"
#include "EventEditor.hpp"
#include "Events_fwd.hpp"
#include "EventSource_fwd.hpp"

#include <mutex>
#include <vector>

namespace erbsland::event {

/// Base class for event sources owned by one event loop.
/// @tested{EventSourceTest}
class EventSource {
public:
    virtual ~EventSource();
    EventSource(const EventSource &) = delete;
    EventSource(EventSource &&) = delete;
    auto operator=(const EventSource &) -> EventSource & = delete;
    auto operator=(EventSource &&) -> EventSource & = delete;

public: // accessors
    /// Access the event loop that owns this source.
    [[nodiscard]] auto ownerEvents() const noexcept -> const EventsPtr &;

protected:
    /// Create an event source owned by an event target.
    explicit EventSource(EventsPtr ownerEvents);
    /// Get the current event target after verifying that it owns this source.
    /// @throws err::LogicError If called outside the owner event loop.
    [[nodiscard]] auto currentOwnerEvents() const -> EventsPtr;
    /// Register an editor for weak lifetime tracking.
    void registerEditor(const EventEditorPtr &editor);
    /// Get connected editors of one type.
    template <typename T>
    [[nodiscard]] auto connectedEditors() const -> std::vector<std::shared_ptr<T>> {
        auto result = std::vector<std::shared_ptr<T>>{};
        std::scoped_lock lock{_editorMutex};
        for (const auto &weakEditor : _editors) {
            const auto editor = weakEditor.lock();
            if (editor == nullptr || !editor->isConnected()) {
                continue;
            }
            if (const auto typedEditor = std::dynamic_pointer_cast<T>(editor); typedEditor != nullptr) {
                result.emplace_back(std::move(typedEditor));
            }
        }
        return result;
    }

private:
    EventsPtr _ownerEvents;                   ///< Owner event target.
    mutable std::mutex _editorMutex;          ///< Protects editor tracking.
    std::vector<EventEditorWeakPtr> _editors; ///< Weakly held event editors.
};

}
