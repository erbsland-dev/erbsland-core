// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IntegerFormat.hpp"

#include "Char.hpp"

namespace erbsland::text {

auto IntegerFormat::setFromBasePrefix(const Char character) noexcept -> bool {
    const auto base = IntegerBase::fromPrefixChar(character);
    if (!base.has_value()) {
        return false;
    }
    setBase(base.value());
    setLetterCase(character.isAsciiUppercaseLetter() ? LetterCase::Uppercase : LetterCase::Lowercase);
    return true;
}

}
