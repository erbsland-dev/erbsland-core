// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DeflateHuffmanEntry.hpp"

#include "../CodecBitReader.hpp"

#include "../../../mem/BitReader_fwd.hpp"

#include <cstdint>
#include <vector>

namespace erbsland::compression::impl {

/// Canonical Huffman decoding table for a Deflate block.
/// @tested{ByteCompressionTest}
class DeflateHuffmanTable final {
private:
    static constexpr auto cMaximumCodeLength = uint8_t{15U}; ///< Maximum code length permitted by Deflate.

public:
    /// Build a canonical table from symbol code lengths.
    explicit DeflateHuffmanTable(const std::vector<uint8_t> &lengths);

public:
    /// Decode one symbol without reading past its final bit.
    [[nodiscard]] auto decode(CodecBitReader &reader) const -> uint16_t;
    /// Reverse the requested low-order bits.
    [[nodiscard]] static auto reverseBits(uint32_t value, unsigned count) noexcept -> uint16_t;

private:
    std::vector<DeflateHuffmanEntry> _entries; ///< Direct canonical-code lookup table.
    uint8_t _maximumLength{};                  ///< Longest code length present in this table.
};

}
