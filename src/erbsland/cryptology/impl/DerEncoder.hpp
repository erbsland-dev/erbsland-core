// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor_fwd.hpp"
#include "../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl::der_encoder {

/// Minimal canonical DER encoder for generated signing-key public values and signatures.
/// @tested{SigningPrivateKeyTest}
/// Wrap content in one canonical DER value with the given low tag octet.
[[nodiscard]] auto wrap(uint8_t tag, mem::ConstByteSpan content) -> mem::ByteBlock;
/// Encode one constructed SEQUENCE from already encoded child values.
[[nodiscard]] auto sequence(mem::ConstByteSpan children) -> mem::ByteBlock;
/// Encode one nonnegative INTEGER from a big-endian magnitude.
[[nodiscard]] auto positiveInteger(mem::ConstByteSpan magnitude) -> mem::ByteBlock;
/// Encode one OCTET STRING.
[[nodiscard]] auto octetString(mem::ConstByteSpan content) -> mem::ByteBlock;
/// Encode one BIT STRING containing complete octets.
[[nodiscard]] auto bitString(mem::ConstByteSpan content) -> mem::ByteBlock;
/// Append one canonical DER length.
void appendLength(mem::ByteBlockEditor &result, std::size_t length);

}
