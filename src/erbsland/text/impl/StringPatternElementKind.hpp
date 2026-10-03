// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// Identify the matching rule represented by a string-pattern element.
enum class StringPatternElementKind : std::uint8_t {
    Character,     ///< Match one literal character.
    OneChar,       ///< Match one arbitrary character.
    AnyText,       ///< Match any number of characters in a full-string glob.
    RecursiveStar, ///< Match complete path components; consumed only by the path matcher.
    NegatedSet,    ///< Match a character outside a set of ranges.
    Set,           ///< Match a character from a set of ranges.
};

}
