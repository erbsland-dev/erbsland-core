// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::placeholder {

/// How literal placeholder delimiters are represented.
enum class EscapeMode : uint8_t {
    Backslash, ///< A backslash escapes the next delimiter-start code point.
    Double,    ///< Repeating a delimiter-start code point makes it literal.
};

}
