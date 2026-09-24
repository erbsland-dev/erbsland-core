// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Bzip2BwtResult.hpp"
#include "Bzip2MtfResult.hpp"

#include "../CodecOutput.hpp"
#include "../CodecReader.hpp"

#include "../../../mem/BitWriter.hpp"
#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../CompressionLevel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace erbsland::compression::impl {

/// Encodes one complete Bzip2 representation.
/// @tested{ByteCompressionTest}
class Bzip2Encoder final {
private:
    static constexpr auto cStreamMagic = uint32_t{0x425a68U};         ///< `BZh` stream signature.
    static constexpr auto cBlockMarker = uint64_t{0x314159265359ULL}; ///< Marker that starts a data block.
    static constexpr auto cEndMarker = uint64_t{0x177245385090ULL};   ///< Marker that ends a stream.
    static constexpr auto cBlockInputFactor = std::size_t{80'000U};   ///< Safe input bytes per block-level unit.
    static constexpr auto cRleLiteralLimit = std::size_t{4U};         ///< Literals before an RLE count byte.
    static constexpr auto cMaximumRleRun = std::size_t{259U};         ///< Longest run represented by RLE1.
    static constexpr auto cAlphabetValueCount = std::size_t{256U};    ///< Number of possible byte values.
    static constexpr auto cAlphabetGroupCount = std::size_t{16U};     ///< Number of alphabet-presence groups.
    static constexpr auto cAlphabetGroupSize = std::size_t{16U};      ///< Byte values represented per group.
    static constexpr auto cSelectorGroupSize = std::size_t{50U};      ///< Symbols encoded by one selector.
    static constexpr auto cHuffmanGroupCount = std::size_t{2U};       ///< Number of encoder Huffman groups.

public:
    /// Encode one continuous stream using bounded input and output.
    void encodeStream(CodecReader &input, const CodecOutput::Write &output);

    /// Create an encoder for a compression level.
    explicit Bzip2Encoder(CompressionLevel level);

private:
    /// Map the public compression level to a Bzip2 block-size digit.
    [[nodiscard]] static auto blockLevel(CompressionLevel level) noexcept -> uint8_t;
    /// Apply the first Bzip2 run-length transform.
    [[nodiscard]] static auto rle1(mem::ConstByteSpan input) -> mem::ByteBlock;
    /// Apply the Burrows-Wheeler transform.
    [[nodiscard]] auto bwt(mem::ConstByteSpan input) -> Bzip2BwtResult;
    /// Apply move-to-front encoding and zero-run encoding.
    [[nodiscard]] static auto mtfRle(mem::ConstByteSpan input) -> Bzip2MtfResult;
    /// Transform and encode one Bzip2 block.
    void encodeBlock(mem::ConstByteSpan input);

private:
    std::vector<uint32_t> _order;       ///< Reused rotation order.
    std::vector<uint32_t> _classes;     ///< Reused rotation equivalence classes.
    std::vector<uint32_t> _scratch;     ///< Reused transform sorting workspace.
    std::vector<uint32_t> _classCounts; ///< Reused class counting workspace.
    uint8_t _blockLevel;                ///< Encoded Bzip2 block-size level.
    mem::BitWriter _writer;             ///< Compressed output bit stream.
    uint32_t _combinedCrc{};            ///< Running combined block checksum.
};

}
