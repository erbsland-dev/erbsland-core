// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/Char.hpp"

namespace erbsland::cterm::impl {

/// Test if one code point is safe for `Block` construction.
/// This accepts any valid Unicode code point except control characters.
/// New-line and tabs are allowed control-characters.
[[nodiscard]] constexpr auto isSafeCodePoint(const text::Char codePoint) noexcept -> bool {
    return codePoint.isValidUnicode() &&
        (codePoint == U'\t' || codePoint == U'\n' || (codePoint >= U' ' && codePoint <= U'~') ||
            codePoint >= text::Char{0xA0U});
}

/// Return a safe replacement for invalid code-points.
/// @return Space for all valid Unicode code points, otherwise the Unicode replacement character.
[[nodiscard]] constexpr auto safeCodePointReplacement(const text::Char codePoint) noexcept -> text::Char {
    return codePoint.isValidUnicode() ? text::Char{U' '} : text::Char::replacement();
}

/// Return a safe code point for `Block`.
/// Valid Unicode code points are preserved, control codes are replaced with a space - to represent an
/// "invisible" character. Invalid Unicode code points are replaced with the Unicode replacement character.
/// New-line and tabs are allowed control-characters.
/// @return The safe code point for use in terminal strings.
[[nodiscard]] constexpr auto safeCodePoint(const text::Char codePoint) noexcept -> text::Char {
    return isSafeCodePoint(codePoint) ? codePoint : safeCodePointReplacement(codePoint);
}

/// Test whether one code point is accepted as a visible string character.
/// @param codePoint The code point to test.
/// @return `true` if the code point is preserved in a terminal string.
[[nodiscard]] inline auto isStringCharacter(const text::Char codePoint) noexcept -> bool {
    return codePoint == U'\t' || codePoint == U'\n' ||
        (!codePoint.isControl() && codePoint >= U' ' && codePoint.displayWidth() > 0);
}

}
