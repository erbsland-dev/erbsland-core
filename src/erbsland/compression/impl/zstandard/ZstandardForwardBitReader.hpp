// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// Reads the forward, least-significant-bit-first fields used by Zstandard table descriptions.
/// @tested{ZstandardInternalTest}
class ZstandardForwardBitReader final {
public:
    /// Create a reader at the supplied byte position.
    ZstandardForwardBitReader(mem::ConstByteSpan data, std::size_t position) noexcept;

public: // read
    /// Ensure that at least `count` bits are buffered.
    [[nodiscard]] auto ensure(std::size_t count) -> bool;
    /// Peek at the requested low-order bits without advancing.
    [[nodiscard]] auto peek(std::size_t count) -> uint32_t;
    /// Read the requested low-order bits.
    [[nodiscard]] auto read(std::size_t count) -> uint32_t;
    /// Return the first byte not consumed by the logical bit position.
    [[nodiscard]] auto consumedBytePosition() const noexcept -> std::size_t;

private:
    mem::ConstByteSpan _data; ///< Complete containing byte sequence.
    std::size_t _position;    ///< Next byte to buffer.
    uint64_t _bits{};         ///< Buffered bits, next bits in the low end.
    std::size_t _count{};     ///< Number of valid buffered bits.
};

}
