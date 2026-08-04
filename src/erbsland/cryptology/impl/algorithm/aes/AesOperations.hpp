// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/Byte.hpp"

namespace erbsland::cryptology::impl::aes {

/// Multiply two field elements in GF(2^8) using the AES reduction polynomial.
/// This is the fixed-iteration multiplication from FIPS 197, Section 4.4.
/// @param left The first field element.
/// @param right The second field element.
/// @return The field product.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto multiply(mem::Byte left, mem::Byte right) noexcept -> mem::Byte;

/// Apply the AES S-box without secret-indexed lookup tables.
/// This implements the multiplicative inverse and affine transformation from FIPS 197, Section 5.1.1.
/// @param value The input byte.
/// @return The substituted byte.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto substitute(mem::Byte value) noexcept -> mem::Byte;

/// Apply the inverse AES S-box without secret-indexed lookup tables.
/// This implements the inverse affine transformation and field inverse from FIPS 197, Section 5.3.2.
/// @param value The input byte.
/// @return The inverse-substituted byte.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto inverseSubstitute(mem::Byte value) noexcept -> mem::Byte;

}
