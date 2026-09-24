// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <vector>

namespace erbsland::compression::impl {

/// Move-to-front and run-length transform result for one Bzip2 block.
/// @tested{ByteCompressionTest}
struct Bzip2MtfResult final {
    std::vector<uint16_t> symbols; ///< Huffman symbols after move-to-front and zero-run encoding.
    std::vector<uint8_t> alphabet; ///< Numeric byte values used by the block, in symbol order.
};

}
