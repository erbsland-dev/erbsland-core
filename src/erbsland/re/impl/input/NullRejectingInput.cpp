// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NullRejectingInput.hpp"

#include "../error/InternalError.hpp"

#include "../../../text/EncodingError.hpp"
#include "../../../text/Literals.hpp"
#include "../../CharAndPosition.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

auto NullRejectingInput::create(InputBasePtr input) -> InputBasePtr {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(input != nullptr, "Input must not be null"_el);
    return std::make_shared<NullRejectingInput>(std::move(input));
}

NullRejectingInput::NullRejectingInput(InputBasePtr input) : _input{std::move(input)} {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_input != nullptr, "Input must not be null"_el);
}

auto NullRejectingInput::read() -> CharAndPosition {
    return validate(_input->read());
}

auto NullRejectingInput::peek() -> CharAndPosition {
    return validate(_input->peek());
}

void NullRejectingInput::skip(const unit::CpLength characterCount) {
    for (auto index = unit::CpLength{}; index < characterCount; ++index) {
        if (read().character.isEndOfData()) {
            return;
        }
    }
}

auto NullRejectingInput::validate(const CharAndPosition result) -> CharAndPosition {
    if (result.character.isNull()) {
        throw text::EncodingError{"Null characters are disabled in regular expression input."_el};
    }
    return result;
}

}
