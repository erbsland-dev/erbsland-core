// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::re::impl {

/// Flags active while parsing or matching a pattern group.
enum class GroupFlag : uint8_t {
    None = 0,
    IgnoreCase = 1U << 0, ///< Ignore case when matching text.
    Multiline = 1U << 1,  ///< Match at the beginning and end of each line.
    DotAll = 1U << 2,     ///< The dot operator also matches newlines.
    Ascii = 1U << 3,      ///< Restrict `\\w`, `\\d`, `\\s` to ASCII only matching.
    Verbose = 1U << 4,    ///< Ignore spacing in the regular expression.
    Atomic = 1U << 5,     ///< Atomic match. Used for atomic groups.
};

}
