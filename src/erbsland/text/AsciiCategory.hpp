// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// ASCII-only categories for fast classification without the Unicode database.
enum class AsciiCategory : uint8_t {
    Letter,
    LowercaseLetter,
    UppercaseLetter,
    Digit,
    HexDigit,
    Alphanumeric,
    Whitespace,
    Blank,
    Control,
    Punctuation,
};

}
