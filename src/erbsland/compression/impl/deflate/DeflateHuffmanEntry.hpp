// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// Canonical Huffman code entry used by a Deflate decoding table.
/// @tested{ByteCompressionTest}
struct DeflateHuffmanEntry final {
    uint16_t symbol{}; ///< Decoded symbol.
    uint8_t length{};  ///< Code length in bits.
};

}
