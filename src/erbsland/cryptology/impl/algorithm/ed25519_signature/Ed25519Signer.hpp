// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Ed25519SignatureTypes.hpp"

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl::ed25519_signer {

using Scalar = mem::ByteArray<32U>;
using WideScalar = mem::ByteArray<64U>;

/// Constant-schedule pure Ed25519 public-key derivation and signing.
///
/// The implementation follows RFC 8032 sections 5.1.5 and 5.1.6. Secret scalar multiplication, scalar reduction,
/// addition, and multiplication execute fixed iteration counts and use masked selection. Expanded seeds, nonces,
/// secret scalars, and projective scratch points are explicitly erased on every exit.
/// @tested{SigningPrivateKeyTest}
/// Derive the canonical 32-octet public-key encoding from one private seed.
/// @param seed The exact 32-octet RFC 8032 private seed.
/// @return The canonical encoded public point.
/// @throws err::ParameterError If the seed length is invalid.
[[nodiscard]] auto publicKey(mem::ConstByteSpan seed) -> mem::ByteArray<32U>;
/// Sign one exact message with one private seed.
/// @param seed The exact 32-octet RFC 8032 private seed.
/// @param message The exact message bytes.
/// @return The 64-octet `ENC(R) || ENC(S)` signature.
/// @throws err::ParameterError If the seed length is invalid or hashing exceeds its bound.
[[nodiscard]] auto sign(mem::ConstByteSpan seed, mem::ConstByteSpan message) -> mem::ByteBlock;
/// Expand and clamp the secret scalar from SHA-512(seed).
[[nodiscard]] auto expandedScalar(mem::ConstByteSpan digest) -> Scalar;
/// Multiply the standard base point by a secret scalar with a fixed double-and-add-always schedule.
[[nodiscard]] auto multiplyBaseSecret(mem::ConstByteSpan scalar) noexcept -> ed25519_signature::Point;
/// Select one projective point without secret-dependent branches.
[[nodiscard]] auto selectPoint(
    const ed25519_signature::Point &first, const ed25519_signature::Point &second, uint64_t selectSecond) noexcept
    -> ed25519_signature::Point;
/// Encode a nonidentity projective point as RFC 8032 `ENC(P)`.
[[nodiscard]] auto encodePoint(const ed25519_signature::Point &point) noexcept -> Scalar;
/// Reduce a little-endian integer of up to 64 octets modulo the subgroup order L.
[[nodiscard]] auto reduceScalar(mem::ConstByteSpan value) noexcept -> Scalar;
/// Add two reduced scalars modulo L.
[[nodiscard]] auto addScalars(const Scalar &left, const Scalar &right) noexcept -> Scalar;
/// Multiply two reduced scalars modulo L.
[[nodiscard]] auto multiplyScalars(const Scalar &left, const Scalar &right) noexcept -> Scalar;
/// Shift a reduced scalar left and append one input bit, then reduce modulo L.
void shiftAppendReduce(Scalar &value, uint8_t bit) noexcept;
/// Conditionally subtract L from one scalar.
void reduceOnce(Scalar &value) noexcept;

}
