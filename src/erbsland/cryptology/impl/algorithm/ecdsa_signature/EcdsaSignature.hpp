// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EcdsaSignatureTypes.hpp"

#include "../../../../mem/ByteSpan.hpp"
#include "../../../keys/PublicKey.hpp"
#include "../../../x509/X509AlgorithmIdentifier.hpp"

#include <optional>

namespace erbsland::cryptology::impl::ecdsa_signature {

/// Test whether an AlgorithmIdentifier selects a supported ECDSA signature family.
[[nodiscard]] auto isSignatureAlgorithm(const X509AlgorithmIdentifier &algorithm) noexcept -> bool;
/// Verify one supported X.509 ECDSA signature.
/// Verification follows FIPS 186-5 section 6.4.2. Public-key encoding follows RFC 5480, signature identifiers follow
/// RFC 5758, and ECDSA-Sig-Value follows RFC 3279. All retained values are public verification inputs.
/// @param publicKey The exact X.509 SubjectPublicKeyInfo.
/// @param signatureAlgorithm The complete X.509 signature AlgorithmIdentifier.
/// @param message The exact signed message.
/// @param signature The canonical DER ECDSA-Sig-Value.
/// @return `true` only for a valid signature.
/// @throws err::ParseError If an encoding, algorithm, named curve, or fixed resource bound is invalid.
[[nodiscard]] auto verify(
    const PublicKey &publicKey,
    const X509AlgorithmIdentifier &signatureAlgorithm,
    mem::ConstByteSpan message,
    mem::ConstByteSpan signature) -> bool;
/// Decode the supported signature algorithm and its absent parameters.
[[nodiscard]] auto decodeParameters(const X509AlgorithmIdentifier &algorithm) -> Parameters;
/// Decode and validate the matching id-ecPublicKey SubjectPublicKeyInfo.
[[nodiscard]] auto decodePublicKey(const PublicKey &publicKey, const Parameters &parameters) -> NistPrimeCurve::Point;
/// Decode one canonical DER ECDSA-Sig-Value, returning no value for out-of-range r or s.
[[nodiscard]] auto decodeSignature(mem::ConstByteSpan signature, const NistPrimeCurve &curve)
    -> std::optional<SignatureValues>;
/// Decode one canonical positive DER INTEGER into a bounded number.
[[nodiscard]] auto decodeInteger(const Asn1Node &node, const NistPrimeCurve &curve)
    -> std::optional<NistPrimeCurve::Number>;
/// Require one exact ASN.1 node shape.
void requireNode(
    const Asn1Node &node, Asn1TagClass tagClass, uint32_t tagNumber, bool constructed, const text::String &name);
/// Throw a parse failure with a stable ECDSA context.
[[noreturn]] void throwParseError(const text::String &reason);

}
