// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Bzip2Crc.hpp"
#include "Bzip2HuffmanTable.hpp"

#include "../CodecBitReader.hpp"
#include "../CodecOutput.hpp"

#include "../../../mem/BitReader.hpp"
#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../DecompressionOptions.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace erbsland::compression::impl {

/// Decodes one complete Bzip2 representation.
/// @tested{ByteCompressionTest}
class Bzip2Decoder final {
private:
    static constexpr auto cStreamMagic = uint32_t{0x425a68U};            ///< `BZh` stream signature.
    static constexpr auto cBlockMarker = uint64_t{0x314159265359ULL};    ///< Marker that starts a data block.
    static constexpr auto cEndMarker = uint64_t{0x177245385090ULL};      ///< Marker that ends a stream.
    static constexpr auto cBlockSizeStep = std::size_t{100'000U};        ///< Bytes represented by one level digit.
    static constexpr auto cSelectorGroupSize = std::size_t{50U};         ///< Symbols decoded by one selector.
    static constexpr auto cMaximumSelectorCount = uint64_t{18'002U};     ///< Maximum selectors in one block.
    static constexpr auto cMinimumGroupCount = uint64_t{2U};             ///< Minimum Huffman groups in a block.
    static constexpr auto cMaximumGroupCount = uint64_t{6U};             ///< Maximum Huffman groups in a block.
    static constexpr auto cMaximumCodeLength = 20;                       ///< Maximum Huffman code length.
    static constexpr auto cAlphabetGroupCount = std::size_t{16U};        ///< Number of alphabet-presence groups.
    static constexpr auto cAlphabetGroupSize = std::size_t{16U};         ///< Byte values represented per group.
    static constexpr auto cAlphabetValueCount = std::size_t{256U};       ///< Number of possible byte values.
    static constexpr auto cRleLiteralLimit = std::size_t{4U};            ///< Literals before an RLE count byte.
    static constexpr auto cWorkspaceFactor = std::size_t{10U};           ///< BWT workspace bytes per block byte.
    static constexpr auto cWorkspaceOverhead = std::size_t{64U * 1024U}; ///< Fixed decoder workspace allowance.

public:
    /// Create a bounded streaming decoder.
    Bzip2Decoder(CodecReader &input, const DecompressionOptions &options, CodecOutput::Write output);

    /// Decode and validate the complete payload.
    void decode();

private:
    /// Append a repeated output byte while enforcing the output limit.
    void append(uint8_t value, std::size_t count = 1U);
    /// Read the symbol alphabet used by the current block.
    [[nodiscard]] auto readAlphabet() -> std::vector<uint8_t>;
    /// Read the Huffman table selectors for the current block.
    [[nodiscard]] auto readSelectors(std::size_t groupCount) -> std::vector<uint8_t>;
    /// Read the Huffman decoding tables for the current block.
    [[nodiscard]] auto readTables(std::size_t groupCount, std::size_t alphaSize) -> std::vector<Bzip2HuffmanTable>;
    /// Decode the Huffman, move-to-front, and zero-run symbol stream.
    [[nodiscard]] auto decodeSymbols(
        const std::vector<uint8_t> &alphabet,
        const std::vector<uint8_t> &selectors,
        const std::vector<Bzip2HuffmanTable> &tables) -> mem::ByteBlock;
    /// Reverse the Burrows-Wheeler transform.
    [[nodiscard]] static auto inverseBwt(mem::ConstByteSpan lastColumn, std::size_t originalPointer) -> mem::ByteBlock;
    /// Reverse the first run-length transform and update the block CRC.
    void undoRle1(mem::ConstByteSpan data, Bzip2Crc &crc);
    /// Decode and validate one Bzip2 block.
    void decodeBlock();

private:
    CodecBitReader _reader;               ///< Compressed input bit stream.
    const DecompressionOptions &_options; ///< Output and workspace limits.
    std::size_t _blockSize{};             ///< Maximum decoded size of the current block.
    uint32_t _combinedCrc{};              ///< Running combined block checksum.
    CodecOutput _output;                  ///< Decoded output bytes.
};

}
