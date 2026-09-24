// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionCodec.hpp"

#include "../../../mem/ByteBlockEditor_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../text/String.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>

namespace erbsland::compression::impl {

/// Codec for raw and Core-wrapped LZ4 blocks.
/// @tested{CompressionCodecTest ByteCompressionTest}
class Lz4Codec final : public CompressionCodec {
private:
    static constexpr auto cMinimumMatchLength = std::size_t{4U};                 ///< Minimum LZ4 match length.
    static constexpr auto cHashTableSize = std::size_t{1U << 16U};               ///< Entries in the match hash table.
    static constexpr auto cNoPosition = std::numeric_limits<std::size_t>::max(); ///< Missing chain position.
    /// Match-search depth for each compression level.
    inline static constexpr auto cSearchDepths = std::array<std::size_t, 5U>{1U, 4U, 16U, 64U, 256U};
    static constexpr auto cHashMultiplier = uint32_t{2'654'435'761U}; ///< Multiplicative sequence-hash constant.
    static constexpr auto cMaximumOffset = std::size_t{65'535U};      ///< Maximum LZ4 match offset.
    static constexpr auto cLengthNibbleMaximum = uint8_t{15U};        ///< Largest length stored in a token nibble.
    static constexpr auto cLengthExtensionMaximum = uint8_t{255U};    ///< Continuation value for extended lengths.
    static constexpr auto cLastLiteralLength = std::size_t{5U};       ///< Required final literal allowance.
    static constexpr auto cLastMatchStartDistance = std::size_t{12U}; ///< Required distance from end for a match.
    static constexpr auto cSizeOverhead = std::size_t{16U};           ///< Fixed worst-case payload allowance.

public:
    /// Create an LZ4 codec.
    Lz4Codec(CompressionFormat format, CompressionLevel level) noexcept : CompressionCodec{format, level} {}

public: // implement CompressionCodec
    [[nodiscard]] auto algorithm() const noexcept -> CompressionAlgorithm override {
        return CompressionAlgorithm::Lz4Block;
    }
    [[nodiscard]] auto supportsStreaming() const noexcept -> bool override { return false; }

protected: // implement CompressionCodec
    void compressPayload(
        CodecReader &input,
        const CodecOutput::Write &output,
        const CompressionTransferOptions &transfer,
        const CompressionOptions &options) const override;
    void decompressPayload(
        CodecReader &input,
        const CodecOutput::Write &output,
        const CompressionTransferOptions &transfer,
        const DecompressionOptions &options) const override;
    [[nodiscard]] auto maximumPayloadLength(unit::ByteLength length) const -> unit::ByteLength override;

private:
    /// Compress one buffered LZ4 block.
    [[nodiscard]] auto compressBlock(const mem::ByteBlock &data) const -> mem::ByteBlock;
    /// Decode and validate one buffered LZ4 block.
    [[nodiscard]] auto decompressBlock(const mem::ByteBlock &data, const DecompressionOptions &options) const
        -> mem::ByteBlock;
    /// Find a match at a position with at least twelve input bytes remaining.
    [[nodiscard]] static auto findMatch(
        mem::ConstByteSpan input,
        std::size_t position,
        std::size_t candidate,
        std::span<const std::size_t> previous,
        std::size_t depth) noexcept -> std::pair<std::size_t, std::size_t>;
    /// Read a little-endian four-byte sequence for hashing.
    [[nodiscard]] static auto readUInt32(mem::ConstByteSpan data, std::size_t index) noexcept -> uint32_t;
    /// Hash a four-byte sequence into the match table.
    [[nodiscard]] static auto hashSequence(uint32_t value) noexcept -> std::size_t;
    /// Append continuation bytes for an LZ4 length field.
    static void appendExtendedLength(mem::ByteBlockEditor &output, std::size_t length);
    /// Append one literal and optional match sequence.
    static void appendSequence(
        mem::ByteBlockEditor &output,
        mem::ConstByteSpan input,
        std::size_t literalBegin,
        std::size_t literalLength,
        std::size_t matchOffset,
        std::size_t matchLength,
        bool hasMatch);
    /// Throw a malformed-data error with the supplied detail.
    [[noreturn]] static void throwMalformed(const text::String &message);
    /// Read continuation bytes for an LZ4 length field.
    [[nodiscard]] static auto readExtendedLength(mem::ConstByteSpan input, std::size_t &position, std::size_t length)
        -> std::size_t;
};

}
