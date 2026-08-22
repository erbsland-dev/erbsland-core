// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteBlock.hpp"
#include "../ByteBlockEditor_fwd.hpp"
#include "../ByteCompressionAlgorithm.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace erbsland::mem::impl {

/// Implements raw byte-compression algorithms.
/// @tested{ByteCompressionTest}
class ByteCompressionCodec final {
public:
    /// Create a codec for one raw compression algorithm.
    explicit ByteCompressionCodec(ByteCompressionAlgorithm algorithm) noexcept : _algorithm{algorithm} {}

    // defaults
    ~ByteCompressionCodec() = default;
    ByteCompressionCodec(const ByteCompressionCodec &) = default;
    ByteCompressionCodec(ByteCompressionCodec &&) = default;
    auto operator=(const ByteCompressionCodec &) -> ByteCompressionCodec & = default;
    auto operator=(ByteCompressionCodec &&) -> ByteCompressionCodec & = default;

public:
    /// Compress a complete raw block.
    [[nodiscard]] auto compress(ConstByteSpan data, bool sensitive) const -> ByteBlock;
    /// Decompress a complete raw block.
    [[nodiscard]] auto decompress(ConstByteSpan data, unit::ByteLength originalLength, bool sensitive) const
        -> ByteBlock;

private:
    /// Read one little-endian 32-bit sequence from an in-bounds input position.
    [[nodiscard]] static auto readUInt32(ConstByteSpan data, std::size_t index) noexcept -> uint32_t;
    /// Hash one four-byte sequence for the LZ4 match table.
    [[nodiscard]] static auto hashSequence(uint32_t value) noexcept -> std::size_t;
    /// Append an LZ4 extended length.
    static void appendExtendedLength(ByteBlockEditor &output, std::size_t length);
    /// Append one LZ4 sequence.
    static void appendSequence(
        ByteBlockEditor &output,
        ConstByteSpan input,
        std::size_t literalBegin,
        std::size_t literalLength,
        std::size_t matchOffset,
        std::size_t matchLength,
        bool hasMatch);
    /// Compress one LZ4 block.
    [[nodiscard]] static auto compressLz4(ConstByteSpan input, bool sensitive) -> ByteBlock;
    /// Throw a malformed-data error.
    [[noreturn]] static void throwMalformed(std::string_view message);
    /// Read an LZ4 extended length.
    [[nodiscard]] static auto readExtendedLength(ConstByteSpan input, std::size_t &position, std::size_t length)
        -> std::size_t;
    /// Decompress one LZ4 block.
    [[nodiscard]] static auto decompressLz4(ConstByteSpan input, unit::ByteLength originalLength, bool sensitive)
        -> ByteBlock;

private:
    ByteCompressionAlgorithm _algorithm; ///< The selected raw compression algorithm.
};

}
