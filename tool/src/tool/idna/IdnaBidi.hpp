// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::idna {

/// Source-derived IDNA bidi values.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
enum class IdnaBidi : uint8_t {
    Unknown, ///< No bidi class is stored for this code point.
    L,       ///< Left-to-right.
    R,       ///< Right-to-left.
    AL,      ///< Arabic letter.
    EN,      ///< European number.
    AN,      ///< Arabic number.
    ES,      ///< European separator.
    CS,      ///< Common separator.
    ET,      ///< European terminator.
    ON,      ///< Other neutral.
    BN,      ///< Boundary neutral.
    NSM,     ///< Nonspacing mark.
};

}
