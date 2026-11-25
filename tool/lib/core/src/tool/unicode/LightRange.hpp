// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

#include <cstdint>

namespace tool::unicode {

/// One range-compressed Unicode Light table entry.
/// @tested{UnicodeDataToolTest}
struct LightRange {
    uint32_t start{};     ///< First code point in the range.
    el::String category;  ///< UCD general category.
    uint8_t deltaIndex{}; ///< Index of a simple mapping triple.
    uint8_t width{1};     ///< Terminal display width.
};

}
