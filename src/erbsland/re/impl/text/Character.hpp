// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Char.hpp"
#include "../../../text/StringEditor.hpp"

#include <compare>

namespace erbsland::re::impl {

/// Append a character as a regular-expression Unicode hex escape.
/// Values up to U+FFFF use `\\uxxxx`; larger values use `\\u{xxxxxxxx}`.
/// @tested{ReCharacterTest}
void appendAsHexEscape(text::StringEditor &str, text::Char character);

/// Append a character using an unambiguous representation suitable for diagnostics.
/// @tested{ReCharacterTest}
void appendToSafeString(text::StringEditor &str, text::Char character);

/// Append a character using a stable representation suitable for a character class.
/// @tested{ReCharacterTest}
void appendToCharRangeString(text::StringEditor &str, text::Char character);

/// Compare two characters, optionally applying Unicode simple case folding.
[[nodiscard]] auto compareCharacters(text::Char left, text::Char right, bool caseInsensitive) noexcept
    -> std::strong_ordering;

/// Test if this character starts a repetition expression.
[[nodiscard]] constexpr auto isRepetitionStart(const text::Char character) noexcept -> bool {
    return character == U'?' || character == U'*' || character == U'+' || character == U'{';
}

/// Test if this character is a regular-expression group flag.
[[nodiscard]] constexpr auto isGroupFlag(const text::Char character) noexcept -> bool {
    return character == U'i' || character == U'm' || character == U's' || character == U'x' || character == U'a' ||
        character == U'u' || character == U'-';
}

}
