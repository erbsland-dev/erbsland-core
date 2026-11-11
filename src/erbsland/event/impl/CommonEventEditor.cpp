// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CommonEventEditor.hpp"

#include "../EventSource.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::event::impl {

using namespace err;
using namespace text::literals;

CommonEventEditor::CommonEventEditor(EventSourcePtr source, EventTargetPtr target) :
    _source{source}, _target{std::move(target)} {
    if (source == nullptr) {
        throw ParameterFailure{"The event source must not be null."_el, "source"_el};
    }
    if (_target == nullptr) {
        throw ParameterFailure{"The event target must not be null."_el, "target"_el};
    }
    if (source->ownerLoop() != _target) {
        throw LogicFailure{"The event editor target does not own its source."_el};
    }
}

auto CommonEventEditor::source() const -> EventSourcePtr {
    auto result = _source.lock();
    if (result == nullptr) {
        throw LogicFailure{"The event editor source no longer exists."_el};
    }
    return result;
}

auto CommonEventEditor::target() const noexcept -> EventTargetPtr {
    return _target;
}

}
