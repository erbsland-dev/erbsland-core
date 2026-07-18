// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringInput.hpp"

#include "StringMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../CharAndPosition.hpp"

namespace erbsland::re::impl {

auto StringInput::read() -> CharAndPosition {
    const auto position = _position;
    const auto character = _text.readCharAndAdvance(_position);
    return {character, position.toSizeT()};
}

auto StringInput::peek() -> CharAndPosition {
    return {_text.charAt(_position), _position.toSizeT()};
}

void StringInput::skip(const unit::CpLength characterCount) {
    for (auto i = unit::CpLength{}; i < characterCount; ++i) {
        if (_text.readCharAndAdvance(_position).isEndOfData()) {
            break;
        }
    }
}

auto StringInput::createMatch(CaptureGroupList captureGroupList) -> MatchPtr {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        !captureGroupList.empty(), "Capture group list must contain at least one element (the whole string)"_el);
    return std::make_shared<StringMatch>(std::move(captureGroupList), _text);
}

}
