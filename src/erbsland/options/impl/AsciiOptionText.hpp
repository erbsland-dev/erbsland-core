// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionLiterals.hpp"

#include "../../text/Char.hpp"
#include "../../text/String.hpp"
#include "../../text/StringSide.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"
#include "../../unit/ByteCount.hpp"

namespace erbsland::options::impl {

/// Test whether a name body is bounded ASCII text with non-empty word segments.
/// @param token The body without an option prefix; underscores are accepted as separators.
/// @return Whether the body starts with a letter and has valid word segments.
/// @tested{OptionsFrameworkTest OptionsParserTest OptionDocumentTest}
[[nodiscard]] inline auto isAsciiNameToken(const text::String &token) noexcept -> bool {
    if (token.isEmpty() || token.length() > unit::ByteCount{100U}) {
        return false;
    }
    const auto [first, rest] = token.slice(text::StringSide::Front);
    if (!first.isAsciiLetter()) {
        return false;
    }
    auto afterSeparator = false;
    for (const auto character : rest) {
        if (character == U'-' || character == U'_') {
            if (afterSeparator) {
                return false;
            }
            afterSeparator = true;
        } else if (character.isAsciiAlphanumeric()) {
            afterSeparator = false;
        } else {
            return false;
        }
    }
    return !afterSeparator;
}

/// Canonicalize a valid name body without changing its literal option prefix or short-name case.
/// @param name The complete name; malformed spellings that could change form are left intact.
/// @return The spelling with lowercase ASCII letters and dash word separators.
/// @tested{OptionsFrameworkTest OptionsParserTest OptionDocumentTest}
[[nodiscard]] inline auto normalizeOptionName(const text::String &name) -> text::String {
    if (name.length() > unit::ByteCount{102U}) {
        return name;
    }
    if (name.startsWith(option_literal::cShortNamePrefix) && !name.startsWith(option_literal::cLongNamePrefix)) {
        return name;
    }
    if (!name.startsWith(option_literal::cLongNamePrefix) && !name.charAt(text::StringSide::Front).isAsciiLetter()) {
        return name;
    }
    return name.transformed([](const text::Char character) noexcept -> text::Char {
        if (character == U'_') {
            return text::Char{U'-'};
        }
        return character.toAsciiLowercase();
    });
}

}
