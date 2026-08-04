// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../NistPrimeCurve.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl::ecdsa_signer {

/// Constant-schedule ECDSA P-256 public-key derivation and deterministic signing.
///
/// Signing follows FIPS 186-5 section 6.4.1 with deterministic per-message secrets from RFC 6979 section 3.2.
/// Secret modular arithmetic and scalar multiplication use fixed limb and bit schedules with masked selection.
/// @tested{SigningPrivateKeyTest}
/// Derive an uncompressed P-256 public point from one private scalar.
[[nodiscard]] auto publicKey(mem::ConstByteSpan privateScalar) -> mem::ByteBlock;
/// Sign one exact message using deterministic ECDSA P-256/SHA-256.
[[nodiscard]] auto sign(mem::ConstByteSpan privateScalar, mem::ConstByteSpan message) -> mem::ByteBlock;
/// Parse and validate one exact P-256 private scalar.
[[nodiscard]] auto decodePrivateScalar(mem::ConstByteSpan bytes) -> NistPrimeCurve::Number;
/// Hash the message with SHA-256.
[[nodiscard]] auto hashMessage(mem::ConstByteSpan message) -> mem::ByteBlock;
/// Generate RFC 6979 k for P-256/SHA-256.
[[nodiscard]] auto deterministicNonce(const NistPrimeCurve::Number &privateScalar, mem::ConstByteSpan hash)
    -> NistPrimeCurve::Number;
/// Calculate one HMAC-SHA-256 value from up to four byte sequences.
[[nodiscard]] auto hmac(
    mem::ConstByteSpan key,
    mem::ConstByteSpan first,
    mem::ConstByteSpan second = {},
    mem::ConstByteSpan third = {},
    mem::ConstByteSpan fourth = {}) -> mem::ByteBlock;

}
