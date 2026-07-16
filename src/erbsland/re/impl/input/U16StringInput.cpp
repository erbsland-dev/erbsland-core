// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringInput.hpp"

#include "U16StringMatch.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

auto U16StringInput::read() -> CharAndPosition {
    const auto position = _position;
    return {_text.readCharAndAdvance(_position), position.toSizeT()};
}

auto U16StringInput::peek() -> CharAndPosition {
    return {_text.charAt(_position), _position.toSizeT()};
}

void U16StringInput::skip(const unit::CpLength characterCount) {
    for (auto i = unit::CpLength{}; i < characterCount; ++i) {
        if (_text.readCharAndAdvance(_position).isEndOfData()) {
            break;
        }
    }
}

auto U16StringInput::createMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList) -> Match16Ptr {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        !captureGroupList.empty(), "Capture group list must contain at least one element (the whole string)"_el);
    return std::make_shared<U16StringMatch>(std::move(regEx), std::move(captureGroupList), _text);
}

}
