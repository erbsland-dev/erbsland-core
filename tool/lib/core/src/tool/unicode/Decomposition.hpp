// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <vector>

namespace tool::unicode {

/// One direct UCD decomposition mapping.
/// @tested{UnicodeDataToolTest}
struct Decomposition {
    uint32_t codePoint{};          ///< Source scalar.
    std::vector<uint32_t> mapping; ///< Direct target sequence.
    bool compatibility{};          ///< Whether the mapping has a compatibility tag.
};

}
