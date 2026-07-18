// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Character.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"

#include <cstdint>

namespace erbsland::re::impl {

void appendAsHexEscape(text::StringEditor &str, const text::Char character) {
    if (!character.isValidUnicode()) {
        return;
    }
    const auto value = character.toRawValue();
    if (value <= 0xFFFFU) {
        str.append(text::StringFormat{"\\u{:04X}"}.build(static_cast<std::uint32_t>(value)));
    } else {
        str.append(text::StringFormat{"\\u{{{:X}}}"}.build(static_cast<std::uint32_t>(value)));
    }
}

void appendToSafeString(text::StringEditor &str, const text::Char character) {
    using namespace text::literals;
    if (character == U'"' || character == U'\\') {
        str.append("\\"_el);
        str.append(character);
    } else if (!character.isSafeUnicode()) {
        appendAsHexEscape(str, character);
    } else {
        str.append(character);
    }
}

void appendToCharRangeString(text::StringEditor &str, const text::Char character) {
    using namespace text::literals;
    if (!character.isValidUnicode()) {
        return;
    }
    if (character == U'\\') {
        // Always escape a backslash
        str.append("\\\\"_el);
        return;
    }
    // Characters that can confuse the class syntax are always hex-escaped.
    if (character == U'-' || character == U'^' || character == U'[' || character == U']') {
        appendAsHexEscape(str, character);
        return;
    }
    // ASCII control and space should be escaped in class output.
    if (character.toRawValue() <= 0x20U) {
        appendAsHexEscape(str, character);
        return;
    }
    // Keep non-ASCII characters hex-escaped to make class output stable and unambiguous.
    if (character.toRawValue() >= 0x7FU) {
        appendAsHexEscape(str, character);
        return;
    }
    str.append(character);
}

auto compareCharacters(const text::Char left, const text::Char right, const bool caseInsensitive) noexcept
    -> std::strong_ordering {
    if (!caseInsensitive) {
        return left <=> right;
    }
    return left.compareCaseFolded(right);
}

}
