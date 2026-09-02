// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// The major Unicode general category group.
/// @seedoc{/reference/text/characters}
enum class UnicodeCategoryGroup : uint8_t {
    Letter = 0x0,      ///< Letter (`L*`) categories.
    Mark = 0x1,        ///< Mark (`M*`) categories.
    Number = 0x2,      ///< Number (`N*`) categories.
    Punctuation = 0x3, ///< Punctuation (`P*`) categories.
    Symbol = 0x4,      ///< Symbol (`S*`) categories.
    Separator = 0x5,   ///< Separator (`Z*`) categories.
    Other = 0x6,       ///< Other (`C*`) categories.
};

}
