// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DerEncoder.hpp"

#include "algorithm/NistPrimeCurve.hpp"

#include "../keys/PublicKey_fwd.hpp"
#include "../keys/SigningKeyAlgorithm.hpp"

#include "../../mem/ByteSpan.hpp"
#include "../../text/String_fwd.hpp"

namespace erbsland::cryptology::impl::signing_key_encoding {

/// Append canonical Ed25519 SubjectPublicKeyInfo shared by import and generation.
/// @param encoder Destination DER encoder.
/// @param point Encoded Ed25519 public point.
/// @tested{SigningPrivateKeyTest}
void appendEd25519PublicKey(DerEncoder &encoder, mem::ConstByteSpan point);
/// Append canonical EC SubjectPublicKeyInfo shared by import and generation.
/// @param encoder Destination DER encoder.
/// @param point Encoded EC public point.
/// @param curve Named curve for the public point.
/// @tested{SigningPrivateKeyTest}
void appendEcPublicKey(DerEncoder &encoder, mem::ConstByteSpan point, NistPrimeCurve::Name curve);
/// Append PKCS#8 PrivateKeyInfo from normalized private data and a public key.
/// @param encoder Destination DER encoder, marked sensitive by this function.
/// @param algorithm Signing-key algorithm.
/// @param privateData Normalized private-key data.
/// @param publicKey Matching public key.
/// @tested{SigningPrivateKeyTest}
void appendPrivateKey(
    DerEncoder &encoder, SigningKeyAlgorithm algorithm, mem::ConstByteSpan privateData, const PublicKey &publicKey);

}
