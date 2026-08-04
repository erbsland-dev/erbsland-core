// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cryptology::impl::galois {

/// Reverse the complete bit order of a GHASH field element.
/// This converts the bit-string convention from NIST SP 800-38D, Section 6.3 into the
/// least-significant-bit polynomial convention used by PMULL and PCLMULQDQ, and back.
/// @param value The field element to reflect.
/// @return The reflected field element.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto reflect(const mem::ByteArray<16> &value) noexcept -> mem::ByteArray<16>;

/// Reduce a 256-bit little-endian polynomial product into the GHASH field.
/// This applies x^128 = x^7 + x^2 + x + 1 from NIST SP 800-38D, Section 6.3.
/// @param product Four least-significant-word-first polynomial words.
/// @return The canonical big-endian GHASH field element.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto reduce(std::array<uint64_t, 4> product) noexcept -> mem::ByteArray<16>;

}
