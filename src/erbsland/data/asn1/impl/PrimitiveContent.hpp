// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteSpan.hpp"

namespace erbsland::data::asn1::impl {

/// Test the minimal signed two's-complement content of INTEGER and ENUMERATED.
/// @param content Primitive content.
/// @return Whether the value uses the shortest signed representation.
/// @tested{Asn1NodeTest BerCodecTest}
[[nodiscard]] auto isMinimalInteger(mem::ConstByteSpan content) noexcept -> bool;

/// Test BIT STRING padding and its unused-bit count.
/// @param content Primitive content including the count octet.
/// @return Whether the count and padding are valid.
/// @tested{Asn1NodeTest DerEncoderTest}
[[nodiscard]] auto isValidBitString(mem::ConstByteSpan content) noexcept -> bool;

}
