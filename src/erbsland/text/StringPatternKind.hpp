// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// Select the matching and extraction semantics of a compiled string pattern.
/// @seedoc{/reference/text/matching_and_splitting}
enum class StringPatternKind : std::uint8_t {
    /// Match a prefix, or anchored prefix/suffix parts separated by one optional divider.
    /// Trimming removes these parts and preserves the text between them.
    Divider,
    /// Match the complete text with Core character-set syntax, escapes, and any number of glob wildcards.
    /// A successful match consumes the complete text for trimming and splitting.
    Glob,
    /// Match the complete text using the Python-style glob dialect with exclamation-mark set negation.
    /// A successful match consumes the complete text for trimming and splitting.
    PosixGlob,
};

}
