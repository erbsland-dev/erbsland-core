// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventSource.hpp"

#include "../err/LogicError.hpp"
#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::event {

using namespace text::literals;

EventSource::EventSource(EventsPtr ownerEvents) : _ownerEvents{std::move(ownerEvents)} {
    if (_ownerEvents == nullptr) {
        throw err::ParameterError{"The owner event target must not be null."_el, "ownerEvents"_el};
    }
}

EventSource::~EventSource() {
    std::scoped_lock lock{_editorMutex};
    for (const auto &weakEditor : _editors) {
        if (const auto editor = weakEditor.lock(); editor != nullptr) {
            editor->disconnect();
        }
    }
}

auto EventSource::ownerEvents() const noexcept -> const EventsPtr & {
    return _ownerEvents;
}

auto EventSource::currentOwnerEvents() const -> EventsPtr {
    const auto current = currentEvents();
    if (current != _ownerEvents) {
        throw err::LogicError{"Event editors must be created on their source owner loop."_el};
    }
    return current;
}

void EventSource::registerEditor(const EventEditorPtr &editor) {
    if (editor == nullptr || editor->targetEvents() != _ownerEvents) {
        throw err::LogicError{"The event editor target does not own this source."_el};
    }
    std::scoped_lock lock{_editorMutex};
    auto nextEditors = std::vector<EventEditorWeakPtr>{};
    nextEditors.reserve(_editors.size() + 1U);
    for (const auto &weakEditor : _editors) {
        if (!weakEditor.expired()) {
            nextEditors.emplace_back(weakEditor);
        }
    }
    nextEditors.emplace_back(editor);
    _editors = std::move(nextEditors);
}

}
