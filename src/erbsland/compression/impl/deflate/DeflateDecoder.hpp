// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DeflateHuffmanTable.hpp"

#include "../CodecBitReader.hpp"
#include "../CodecOutput.hpp"

#include "../../../mem/BitReader.hpp"
#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../DecompressionOptions.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace erbsland::compression::impl {

/// Decodes one complete Deflate representation.
/// @tested{ByteCompressionTest}
class DeflateDecoder final {
private:
    static constexpr auto cFixedLiteralCount = std::size_t{288U};       ///< Symbols in the fixed literal table.
    static constexpr auto cFixedDistanceCount = std::size_t{32U};       ///< Symbols in the fixed distance table.
    static constexpr auto cCodeLengthCount = std::size_t{19U};          ///< Symbols in the code-length alphabet.
    static constexpr auto cEndOfBlockSymbol = uint16_t{256U};           ///< Huffman symbol ending a block.
    static constexpr auto cFirstLengthSymbol = uint16_t{257U};          ///< First length symbol.
    static constexpr auto cLastLengthSymbol = uint16_t{285U};           ///< Last valid length symbol.
    static constexpr auto cMaximumDistanceSymbol = uint16_t{29U};       ///< Last valid distance symbol.
    static constexpr auto cMaximumCodeLength = uint8_t{15U};            ///< Maximum Deflate code length.
    static constexpr auto cMaximumBackReference = std::size_t{32'768U}; ///< Maximum Deflate window distance.

public:
    /// Create a bounded streaming decoder.
    DeflateDecoder(CodecReader &input, const DecompressionOptions &options, CodecOutput::Write output);

    /// Decode and validate the complete payload.
    void decode();

private:
    /// Access the immutable fixed literal/length decoding table.
    [[nodiscard]] static auto fixedLiteralTable() -> const DeflateHuffmanTable &;
    /// Access the immutable fixed distance decoding table.
    [[nodiscard]] static auto fixedDistanceTable() -> const DeflateHuffmanTable &;
    /// Create the code lengths for the fixed literal table.
    [[nodiscard]] static auto fixedLiteralLengths() -> std::vector<uint8_t>;
    /// Decode an uncompressed Deflate block.
    void decodeStored();
    /// Read tables and decode a dynamic-Huffman block.
    void decodeDynamic();
    /// Decode a block with the supplied literal and distance tables.
    void decodeCompressed(const DeflateHuffmanTable &literalTable, const DeflateHuffmanTable &distanceTable);
    /// Validate the optional exact output length.
    void validateExpectedLength() const;

private:
    CodecBitReader _reader;               ///< Compressed input bit stream.
    const DecompressionOptions &_options; ///< Output and workspace limits.
    CodecOutput _output;                  ///< Decoded output bytes.
};

}
