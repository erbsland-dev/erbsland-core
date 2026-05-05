// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IntegerBase.hpp"

#include "Char.hpp"

namespace erbsland::text {

auto IntegerBase::fromPrefixChar(const Char character) noexcept -> std::optional<IntegerBase> {
    const auto value = character.toRawValue();
    if (value == U'x' || value == U'X') {
        return IntegerBase{Hexadecimal};
    }
    if (value == U'b' || value == U'B') {
        return IntegerBase{Binary};
    }
    if (value == U'o' || value == U'O') {
        return IntegerBase{Octal};
    }
    return {};
}

auto IntegerBase::prefixChar(const LetterCase letterCase) const noexcept -> Char {
    switch (_value) {
    case Hexadecimal:
        return letterCase == LetterCase::Uppercase ? U'X' : U'x';
    case Binary:
        return letterCase == LetterCase::Uppercase ? U'B' : U'b';
    case Octal:
        return letterCase == LetterCase::Uppercase ? U'O' : U'o';
    case Decimal:
    default:
        return {};
    }
}

}
