// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZstandardHuffmanCode.hpp"

#include "../../../mem/ByteBuffer.hpp"
#include "../../../mem/ByteSpan.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::compression::impl {

/// Deterministic length-limited Huffman encoder for Zstandard literals.
/// @tested{ZstandardInternalTest}
class ZstandardHuffmanEncoder final {
public:
    /// Create an encoder for one literal sequence.
    explicit ZstandardHuffmanEncoder(mem::ConstByteSpan literals) noexcept;

    /// Build a complete compressed literals section, or no candidate when direct weights cannot represent it.
    [[nodiscard]] auto encode() const -> std::optional<mem::ByteBuffer>;

private:
    /// Build a deterministic complete tree and its canonical codes.
    [[nodiscard]] static auto buildTree(
        mem::ConstByteSpan literals,
        std::array<uint8_t, 256U> &weights,
        std::array<ZstandardHuffmanCode, 256U> &codes,
        uint8_t &tableBits) -> std::optional<std::size_t>;
    /// Encode one literal range as a reverse bitstream.
    [[nodiscard]] static auto encodeStream(
        mem::ConstByteSpan literals,
        std::size_t begin,
        std::size_t size,
        const std::array<ZstandardHuffmanCode, 256U> &codes) -> mem::ByteBuffer;
    /// Append a compressed-literals section header.
    static void appendHeader(
        mem::ByteBuffer &output, std::size_t regeneratedSize, std::size_t compressedSize, bool fourStreams);
    /// Append a little-endian 16-bit size.
    static void append16(mem::ByteBuffer &output, std::size_t value);

private:
    mem::ConstByteSpan _literals; ///< Literal sequence to encode.
};

}
