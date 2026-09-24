// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// Reads an end-marked Zstandard bitstream backward.
/// @tested{ZstandardInternalTest}
class ZstandardReverseBitReader final {
public:
    /// Create a reader for `[begin, end)` and consume its final end-marker bit.
    ZstandardReverseBitReader(mem::ConstByteSpan data, std::size_t begin, std::size_t end);

public: // read
    /// Test whether the requested number of bits can be buffered.
    [[nodiscard]] auto canRead(std::size_t count) noexcept -> bool;
    /// Read bits in Zstandard reverse-stream order.
    [[nodiscard]] auto read(std::size_t count) -> uint32_t;
    /// Discard bits already known to be available.
    void discard(std::size_t count);
    /// Peek at buffered bits after ensuring `count` bits are available.
    [[nodiscard]] auto peek(std::size_t count) -> uint32_t;
    /// Peek at up to `count` bits and pad unavailable low bits with zero.
    [[nodiscard]] auto peekPadded(std::size_t count) -> uint32_t;
    /// Return the number of bits that remain in this stream.
    [[nodiscard]] auto remainingBitCount() const noexcept -> std::size_t;
    /// Test whether the stream was consumed exactly.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool;

private:
    mem::ConstByteSpan _data; ///< Complete containing byte sequence.
    std::size_t _begin;       ///< First byte of this bitstream.
    std::size_t _position;    ///< First buffered byte.
    uint64_t _bits{};         ///< Buffered bits, next bits in the high used end.
    std::size_t _count{};     ///< Number of valid buffered bits.
};

}
