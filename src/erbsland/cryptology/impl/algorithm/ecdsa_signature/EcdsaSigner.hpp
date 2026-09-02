// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../NistPrimeCurve.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"
#include "../../../HashAlgorithm.hpp"

namespace erbsland::cryptology::impl::ecdsa_signer {

/// Constant-schedule ECDSA P-256/P-384 public-key derivation and deterministic signing.
///
/// Signing follows FIPS 186-5 section 6.4.1 with deterministic per-message secrets from RFC 6979 section 3.2.
/// Secret modular arithmetic and scalar multiplication use fixed limb and bit schedules with masked selection.
/// @tested{SigningPrivateKeyTest}
/// Derive an uncompressed public point from one private scalar.
[[nodiscard]] auto publicKey(
    mem::ConstByteSpan privateScalar, NistPrimeCurve::Name curveName = NistPrimeCurve::Name::P256) -> mem::ByteBlock;
/// Sign one exact message using deterministic ECDSA with the curve-matched SHA-2 hash.
[[nodiscard]] auto sign(
    mem::ConstByteSpan privateScalar,
    mem::ConstByteSpan message,
    NistPrimeCurve::Name curveName = NistPrimeCurve::Name::P256) -> mem::ByteBlock;
/// Parse and validate one exact private scalar.
[[nodiscard]] auto decodePrivateScalar(mem::ConstByteSpan bytes, NistPrimeCurve::Name curveName)
    -> NistPrimeCurve::Number;
/// Hash the message with the selected SHA-2 algorithm.
[[nodiscard]] auto hashMessage(mem::ConstByteSpan message, HashAlgorithm hash) -> mem::ByteBlock;
/// Generate RFC 6979 k for the selected curve and hash.
[[nodiscard]] auto deterministicNonce(
    const NistPrimeCurve::Number &privateScalar,
    mem::ConstByteSpan hash,
    NistPrimeCurve::Name curveName,
    HashAlgorithm hashAlgorithm) -> NistPrimeCurve::Number;
/// Calculate one HMAC value from up to four byte sequences.
[[nodiscard]] auto hmac(
    HashAlgorithm hash,
    mem::ConstByteSpan key,
    mem::ConstByteSpan first,
    mem::ConstByteSpan second = {},
    mem::ConstByteSpan third = {},
    mem::ConstByteSpan fourth = {}) -> mem::ByteBlock;

}
