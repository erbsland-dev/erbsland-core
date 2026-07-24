// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventEditor.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::event {

using namespace text::literals;

EventEditor::EventEditor(EventsPtr targetEvents) : _targetEvents{std::move(targetEvents)} {
    if (_targetEvents == nullptr) {
        throw err::ParameterError{"The event target must not be null."_el, "targetEvents"_el};
    }
}

EventEditor::~EventEditor() {
    disconnect();
}

void EventEditor::disconnect() noexcept {
    _connected = false;
}

auto EventEditor::isConnected() const noexcept -> bool {
    return _connected;
}

auto EventEditor::targetEvents() const noexcept -> const EventsPtr & {
    return _targetEvents;
}

}
