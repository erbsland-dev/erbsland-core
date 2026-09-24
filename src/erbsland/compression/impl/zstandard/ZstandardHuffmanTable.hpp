// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// One direct-lookup entry in a Zstandard Huffman decoding table.
struct ZstandardHuffmanEntry final {
    uint8_t symbol{};   ///< Decoded literal byte.
    uint8_t bitCount{}; ///< Number of bits consumed for this symbol.
};

/// A validated canonical Huffman table for Zstandard literal streams.
/// @tested{ZstandardInternalTest}
class ZstandardHuffmanTable final {
public:
    /// Read a direct or FSE-compressed weight description.
    void read(mem::ConstByteSpan data, std::size_t &position);
    /// Decode one reverse bitstream into `output`.
    void decodeStream(
        mem::ConstByteSpan data,
        std::size_t begin,
        std::size_t end,
        std::size_t regeneratedSize,
        mem::ByteBuffer &output) const;
    /// Test whether a table is available for a treeless literals section.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _tableBits != 0U; }

private:
    /// Decode weights encoded by an FSE stream.
    [[nodiscard]] static auto readCompressedWeights(
        mem::ConstByteSpan data, std::size_t begin, std::size_t end, std::array<uint8_t, 256U> &weights) -> std::size_t;
    /// Validate weights and construct the direct lookup table.
    void build(const std::array<uint8_t, 256U> &weights, std::size_t count);

private:
    std::array<ZstandardHuffmanEntry, 2048U> _entries{}; ///< Direct lookup table.
    uint8_t _tableBits{};                                ///< Lookup width.
};

}
