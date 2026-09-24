// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// Calculates the CRC used by Bzip2 blocks and streams.
/// @tested{ByteCompressionTest}
class Bzip2Crc final {
private:
    static constexpr auto cInitialValue = uint32_t{0xffffffffU}; ///< Initial CRC register value.
    static constexpr auto cHighBit = uint32_t{0x80000000U};      ///< Highest bit in the CRC register.
    static constexpr auto cPolynomial = uint32_t{0x04c11db7U};   ///< Bzip2 CRC-32 generator polynomial.
    static constexpr auto cByteBitCount = 8U;                    ///< Number of bits incorporated per byte.

public:
    /// Incorporate one uncompressed byte.
    void update(uint8_t value) noexcept;
    /// Get the finalized CRC value.
    [[nodiscard]] auto value() const noexcept -> uint32_t;

private:
    uint32_t _value{cInitialValue}; ///< Current unfinalized CRC register.
};

}
