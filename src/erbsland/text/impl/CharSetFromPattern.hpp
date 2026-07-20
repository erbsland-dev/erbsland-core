// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"

#include "../CharSet.hpp"
#include "../StringCharReader.hpp"

namespace erbsland::text::impl {

/// Create a character set from a simple range pattern read character-by-character.
[[nodiscard]] inline auto charSetFromPatternCharacters(StringCharReader &reader) -> CharSet {
    auto result = CharSet{};
    auto isFirstCharacter = true;

    while (!reader.isAtEnd()) {
        const auto character = reader.read();

        if (character == Char{U'-'}) {
            if (isFirstCharacter || reader.isAtEnd()) {
                result.add(character);
                isFirstCharacter = false;
                continue;
            }
            throwParseError("Unexpected hyphen in character set pattern");
        }

        if (reader.advanceIf(Char{U'-'})) {
            if (reader.isAtEnd()) {
                result.add(character);
                result.add(Char{U'-'});
                isFirstCharacter = false;
                continue;
            }

            const auto last = reader.read();
            if (character >= last) {
                throwParseError("Invalid character range in character set pattern");
            }
            result.add(CharRange{character, last});
            isFirstCharacter = false;
            continue;
        }

        result.add(character);
        isFirstCharacter = false;
    }

    return result;
}

}
