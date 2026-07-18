// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringInput.hpp"

#include "U32StringMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../CharAndPosition.hpp"

namespace erbsland::re::impl {

auto U32StringInput::read() -> CharAndPosition {
    const auto position = _position;
    return {_text.readCharAndAdvance(_position), position.toSizeT()};
}

auto U32StringInput::peek() -> CharAndPosition {
    return {_text.charAt(_position), _position.toSizeT()};
}

void U32StringInput::skip(const unit::CpLength characterCount) {
    for (auto i = unit::CpLength{}; i < characterCount; ++i) {
        if (_text.readCharAndAdvance(_position).isEndOfData()) {
            break;
        }
    }
}

auto U32StringInput::createMatch(CaptureGroupList captureGroupList) -> Match32Ptr {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        !captureGroupList.empty(), "Capture group list must contain at least one element (the whole string)"_el);
    return std::make_shared<U32StringMatch>(std::move(captureGroupList), _text);
}

}
