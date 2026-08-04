// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesOperations.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl::aes {

auto multiply(const mem::Byte left, const mem::Byte right) noexcept -> mem::Byte {
    auto result = uint8_t{};
    auto multiplicand = left.toUInt8();
    auto multiplier = right.toUInt8();

    // FIPS 197, Section 4.4: eight polynomial coefficients are processed regardless of the operand values.
    for (auto bit = 0U; bit < 8U; ++bit) {
        const auto multiplierMask = static_cast<uint8_t>(0U - (multiplier & 1U));
        result ^= static_cast<uint8_t>(multiplicand & multiplierMask);
        const auto highBitMask = static_cast<uint8_t>(0U - (multiplicand >> 7U));
        const auto shifted = static_cast<unsigned int>(multiplicand) << 1U;
        multiplicand = static_cast<uint8_t>(shifted ^ (0x1bU & static_cast<unsigned int>(highBitMask)));
        multiplier >>= 1U;
    }
    return mem::Byte{result};
}

auto substitute(const mem::Byte value) noexcept -> mem::Byte {
    // FIPS 197, Section 5.1.1: x^254 gives the multiplicative inverse, including the defined zero mapping.
    const auto x2 = multiply(value, value);
    const auto x4 = multiply(x2, x2);
    const auto x8 = multiply(x4, x4);
    const auto x16 = multiply(x8, x8);
    const auto x32 = multiply(x16, x16);
    const auto x64 = multiply(x32, x32);
    const auto x128 = multiply(x64, x64);
    const auto inverse = multiply(multiply(multiply(x128, x64), multiply(x32, x16)), multiply(x8, multiply(x4, x2)));

    // FIPS 197, Equation 5.4: apply the affine transformation over GF(2).
    return inverse ^ inverse.rotatedLeft(1) ^ inverse.rotatedLeft(2) ^ inverse.rotatedLeft(3) ^ inverse.rotatedLeft(4) ^
        mem::Byte{0x63U};
}

auto inverseSubstitute(const mem::Byte value) noexcept -> mem::Byte {
    // FIPS 197, Section 5.3.2: undo the affine transformation before taking the field inverse.
    const auto transformed = value.rotatedLeft(1) ^ value.rotatedLeft(3) ^ value.rotatedLeft(6) ^ mem::Byte{0x05U};
    const auto x2 = multiply(transformed, transformed);
    const auto x4 = multiply(x2, x2);
    const auto x8 = multiply(x4, x4);
    const auto x16 = multiply(x8, x8);
    const auto x32 = multiply(x16, x16);
    const auto x64 = multiply(x32, x32);
    const auto x128 = multiply(x64, x64);
    return multiply(multiply(multiply(x128, x64), multiply(x32, x16)), multiply(x8, multiply(x4, x2)));
}

}
