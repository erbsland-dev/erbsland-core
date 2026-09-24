// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// One canonical code used by the Zstandard Huffman encoder.
/// @tested{ZstandardInternalTest}
struct ZstandardHuffmanCode final {
    uint16_t value{};   ///< Canonical code value.
    uint8_t bitCount{}; ///< Code length.
};

}
