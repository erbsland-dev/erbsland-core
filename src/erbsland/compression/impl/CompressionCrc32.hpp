// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteSpan.hpp"

#include <array>
#include <cstdint>

namespace erbsland::compression::impl {

/// Incremental ZIP-compatible CRC-32.
/// @tested{CodecBufferTest CompressionStreamingTest}
class CompressionCrc32 final {
public:
    /// Incorporate a byte span.
    void update(mem::ConstByteSpan data) noexcept {
        for (const auto byte : data) {
            _value = (_value >> 8U) ^ cTable[(_value ^ byte.toUInt32()) & 0xffU];
        }
    }
    /// Get the finalized checksum.
    auto value() const noexcept -> uint32_t { return ~_value; }

private:
    /// Reflected CRC-32 transition for each possible low byte.
    inline static constexpr auto cTable = [] {
        auto result = std::array<uint32_t, 256U>{};
        for (uint32_t byte{}; byte < result.size(); ++byte) {
            auto value = byte;
            for (unsigned bit{}; bit < 8U; ++bit) {
                value = (value >> 1U) ^ (0xedb88320U & (0U - (value & 1U)));
            }
            result[byte] = value;
        }
        return result;
    }();

private:
    uint32_t _value{0xffffffffU}; ///< Running reflected checksum.
};

}
