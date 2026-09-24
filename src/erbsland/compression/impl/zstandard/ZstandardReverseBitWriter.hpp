// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBuffer.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// Builds an end-marked Zstandard reverse bitstream in logical decoding order.
/// @tested{ZstandardInternalTest}
class ZstandardReverseBitWriter final {
public:
    /// Append a numeric field, most-significant bit first.
    void append(uint32_t value, uint8_t count);
    /// Serialize all fields and the mandatory end marker.
    [[nodiscard]] auto takeBytes() -> mem::ByteBuffer;

private:
    /// Read a field from the packed logical stream.
    [[nodiscard]] auto read(std::size_t position, std::size_t count) const noexcept -> uint8_t;

private:
    mem::ByteBuffer _bytes;  ///< Packed logical bits in decoder consumption order.
    std::size_t _bitCount{}; ///< Number of logical bits in the packed stream.
};

}
