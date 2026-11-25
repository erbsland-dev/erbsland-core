// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::idna {

/// Source-derived IDNA script values.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
enum class IdnaScript : uint8_t {
    None,     ///< No contextual script is stored.
    Greek,    ///< Greek.
    Han,      ///< Han.
    Hebrew,   ///< Hebrew.
    Hiragana, ///< Hiragana.
    Katakana, ///< Katakana.
};

}
