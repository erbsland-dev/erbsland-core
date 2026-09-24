// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CodecOutput.hpp"
#include "../CodecReader.hpp"

#include "../../../mem/BitWriter.hpp"
#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../CompressionLevel.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace erbsland::compression::impl {

/// Encodes one complete Deflate representation.
/// @tested{ByteCompressionTest}
class DeflateEncoder final {
private:
    static constexpr auto cMinimumMatchLength = std::size_t{3U};                 ///< Minimum LZ77 match length.
    static constexpr auto cMaximumMatchLength = std::size_t{258U};               ///< Maximum LZ77 match length.
    static constexpr auto cMaximumMatchDistance = std::size_t{32'768U};          ///< Maximum Deflate window distance.
    static constexpr auto cChainTableSize = std::size_t{65'536U};                ///< Power-of-two history ring size.
    static constexpr auto cHashTableSize = std::size_t{32'768U};                 ///< Entries in the match hash table.
    static constexpr auto cNoPosition = std::numeric_limits<std::size_t>::max(); ///< Missing match-chain position.
    static constexpr auto cHashMultiplier = uint32_t{2'654'435'761U};  ///< Multiplicative sequence-hash constant.
    static constexpr auto cEndOfBlockSymbol = uint16_t{256U};          ///< Fixed-Huffman end-of-block symbol.
    static constexpr auto cMaximumStoredLength = std::size_t{65'535U}; ///< Maximum bytes in a stored block.

public:
    /// Encode one continuous stream using bounded input and output.
    void encodeStream(CodecReader &input, const CodecOutput::Write &output);

    /// Create an encoder for a compression level.
    explicit DeflateEncoder(CompressionLevel level);

private:
    /// Reverse the requested low-order bits for Deflate wire order.
    [[nodiscard]] static auto reverseBits(uint32_t value, unsigned count) noexcept -> uint16_t;
    /// Get the canonical fixed code and length for a symbol.
    static void fixedCode(uint16_t symbol, uint16_t &code, unsigned &length) noexcept;
    /// Write one symbol with the fixed Huffman table.
    void writeFixedSymbol(uint16_t symbol);
    /// Get the match-chain search depth for the configured level.
    [[nodiscard]] auto chainDepth() const noexcept -> std::size_t;
    /// Hash a three-byte sequence for match lookup.
    [[nodiscard]] static auto hash(mem::ConstByteSpan input, std::size_t position) noexcept -> std::size_t;
    /// Encode the input with fixed Huffman codes and LZ77 matches.
    void encodeFixed();
    /// Write a Deflate length symbol and its extra bits.
    void writeLength(std::size_t length);
    /// Write a Deflate distance symbol and its extra bits.
    void writeDistance(std::size_t distance);

private:
    std::size_t _start{};                                         ///< Prefix reserved for match history.
    bool _final{true};                                            ///< Final block flag.
    mem::ConstByteSpan _input;                                    ///< Uncompressed input bytes.
    CompressionLevel _level;                                      ///< Requested compression effort.
    mem::BitWriter _writer{mem::BitOrder::LeastSignificantFirst}; ///< Compressed output bit stream.
};

}
