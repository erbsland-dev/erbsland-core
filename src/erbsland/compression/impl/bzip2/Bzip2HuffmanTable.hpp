// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CodecBitReader.hpp"

#include "../../../mem/BitReader_fwd.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace erbsland::compression::impl {

/// Decoding table for one Bzip2 Huffman group.
/// @tested{ByteCompressionTest}
class Bzip2HuffmanTable final {
private:
    static constexpr auto cMaximumCodeLength = uint8_t{20U}; ///< Maximum code length permitted by Bzip2.

public:
    /// Build a canonical table from symbol code lengths.
    explicit Bzip2HuffmanTable(const std::vector<uint8_t> &lengths);
    /// Decode one symbol without reading past its final bit.
    [[nodiscard]] auto decode(CodecBitReader &reader) const -> uint16_t;

private:
    std::array<uint32_t, cMaximumCodeLength + 1U> _firstCodes{};   ///< First canonical code by length.
    std::array<uint16_t, cMaximumCodeLength + 1U> _firstSymbols{}; ///< First symbol-table index by length.
    std::array<uint16_t, cMaximumCodeLength + 1U> _symbolCounts{}; ///< Symbol count by length.
    std::vector<uint16_t> _symbols;                                ///< Symbols ordered by length and value.
    uint8_t _maximumLength{};                                      ///< Longest code length present in this table.
};

}
