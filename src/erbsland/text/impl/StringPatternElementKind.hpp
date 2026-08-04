// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// Identify the matching rule represented by a string-pattern element.
enum class StringPatternElementKind : std::uint8_t {
    Character, ///< Match one literal character.
    OneChar,   ///< Match one arbitrary character.
    Set,       ///< Match a character from a set of ranges.
};

}
