// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Name.hpp"

#include "../ReplacerError.hpp"

#include "../../../util/LoopStatus.hpp"
#include "../../Char.hpp"
#include "../../Literals.hpp"
#include "../../StringFormat.hpp"

namespace erbsland::text::placeholder::impl {

using namespace literals;

auto normalizeName(const String &name) -> String {
    if (name.isEmpty()) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, "Placeholder names must not be empty."_el};
    }
    if (name.characterLength().toSizeT() > 100U) {
        throw ReplacerError{ReplacerErrorCategory::LimitExceeded, "A placeholder name is too long."_el};
    }
    auto result = name.transformed(Char::toIdentifierNormalized);
    auto previous = Char{};
    result.forEach([&](const Char character, const unit::CpIndex index) -> util::LoopStatus {
        if (index.isZero() && (character == U'_' || character.isAsciiDigit())) {
            throw ReplacerError{ReplacerErrorCategory::Syntax, "A placeholder name must start with a letter."_el};
        }
        if (character != U'_' && !character.isAsciiAlphanumeric()) {
            throw ReplacerError{
                ReplacerErrorCategory::Syntax,
                StringFormat{"Invalid placeholder name character at position {}."_el}.build(index)};
        }
        if (character == U'_' && previous == U'_') {
            throw ReplacerError{ReplacerErrorCategory::Syntax, "Adjacent name separators are not allowed."_el};
        }
        previous = character;
        return util::LoopStatus::Continue;
    });
    if (previous == U'_') {
        throw ReplacerError{ReplacerErrorCategory::Syntax, "A placeholder name must not end with a separator."_el};
    }
    return result;
}

}
