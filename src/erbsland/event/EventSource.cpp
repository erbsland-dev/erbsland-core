// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EventSource.hpp"

#include "../err/LogicFailure.hpp"
#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"

namespace erbsland::event {

using namespace err;
using namespace text::literals;

EventSource::EventSource(EventTargetPtr ownerLoop) : _ownerLoop{std::move(ownerLoop)} {
    if (_ownerLoop == nullptr) {
        throw ParameterFailure{"The owner event target must not be null."_el, "ownerLoop"_el};
    }
}

void EventSource::bindOwnerLoop(EventTargetPtr ownerLoop) {
    if (ownerLoop == nullptr || _ownerLoop != nullptr) {
        throw LogicFailure{"An event source must bind exactly once to a non-null owner."_el};
    }
    _ownerLoop = std::move(ownerLoop);
}

auto EventSource::ownerLoop() const noexcept -> const EventTargetPtr & {
    return _ownerLoop;
}

auto EventSource::currentOwnerLoop() const -> EventTargetPtr {
    const auto current = currentLoop();
    if (current != _ownerLoop) {
        throw LogicFailure{"This event source operation must run on its owner loop."_el};
    }
    return current;
}

void EventSource::verifyCurrentOwnerLoop() const {
    if (currentLoop() != _ownerLoop) {
        throw LogicFailure{"This event source operation must run on its owner loop."_el};
    }
}

}
