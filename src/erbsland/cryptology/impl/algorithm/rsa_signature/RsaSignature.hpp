// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RsaSignatureTypes.hpp"

#include "../../../../mem/ByteSpan.hpp"
#include "../../../keys/PublicKey.hpp"
#include "../../../x509/X509AlgorithmIdentifier.hpp"

#include <optional>

namespace erbsland::cryptology::impl::rsa_signature {

/// Verify one X.509 RSA signature.
/// The implementation follows RFC 8017 with X.509 processing from RFC 3279, RFC 4055, and RFC 5280. All RSA operands
/// are public; the bounded modular arithmetic is deliberately variable-time.
/// @param publicKey The exact X.509 SubjectPublicKeyInfo.
/// @param signatureAlgorithm The X.509 signature AlgorithmIdentifier.
/// @param message The exact signed message.
/// @param signature The signature octets.
/// @return `true` only for a valid signature.
/// @throws err::ParseError If an encoding, algorithm, parameter, or fixed security bound is invalid.
[[nodiscard]] auto verify(
    const PublicKey &publicKey,
    const X509AlgorithmIdentifier &signatureAlgorithm,
    mem::ConstByteSpan message,
    mem::ConstByteSpan signature) -> bool;
/// Decode and validate the signature AlgorithmIdentifier.
[[nodiscard]] auto decodeSignatureParameters(const X509AlgorithmIdentifier &algorithm) -> SignatureParameters;
/// Decode RSASSA-PSS-params with RFC defaults and the supported SHA-2 policy.
[[nodiscard]] auto decodePssParameters(const Asn1Node &parameters) -> SignatureParameters;
/// Decode and validate an RSA SubjectPublicKeyInfo.
[[nodiscard]] auto decodePublicKey(const PublicKey &publicKey) -> PublicKeyData;
/// Decode an AlgorithmIdentifier embedded in RSASSA-PSS parameters.
[[nodiscard]] auto decodeHashAlgorithmIdentifier(const Asn1Node &node) -> HashAlgorithm;
/// Decode one bounded nonnegative DER INTEGER into a native size.
[[nodiscard]] auto decodeSmallInteger(const Asn1Node &node, const text::String &name) -> std::size_t;
/// Return positive INTEGER magnitude octets without the optional sign octet.
[[nodiscard]] auto decodePositiveInteger(const Asn1Node &node, const text::String &name) -> mem::ByteBlock;
/// Require one exact ASN.1 node shape.
void requireNode(
    const Asn1Node &node, Asn1TagClass tagClass, uint32_t tagNumber, bool constructed, const text::String &name);
/// Require exact DER NULL parameters.
void requireNullParameters(const Asn1Node &parameters, const text::String &name);
/// Throw a parse failure with a stable RSA context.
[[noreturn]] void throwParseError(const text::String &reason);
/// Apply RSAVP1: `m = s^e mod n`.
[[nodiscard]] auto rsaVerificationPrimitive(const PublicKeyData &key, mem::ConstByteSpan signature)
    -> std::optional<mem::ByteBlock>;
/// Verify EMSA-PSS as specified by RFC 8017 section 9.1.2.
[[nodiscard]] auto verifyPss(
    const PublicKeyData &key,
    const SignatureParameters &parameters,
    mem::ConstByteSpan message,
    const mem::ByteBlock &encodedMessage) -> bool;
/// Verify EMSA-PKCS1-v1_5 as specified by RFC 8017 sections 8.2.2 and 9.2.
[[nodiscard]] auto verifyPkcs1V15(
    const PublicKeyData &key,
    const SignatureParameters &parameters,
    mem::ConstByteSpan message,
    const mem::ByteBlock &encodedMessage) -> bool;
/// Generate MGF1(seed, maskLen) as specified by RFC 8017 appendix B.2.1.
[[nodiscard]] auto mgf1(HashAlgorithm hash, mem::ConstByteSpan seed, std::size_t length) -> mem::ByteBlock;
/// Hash one exact message with the selected SHA-2 algorithm.
[[nodiscard]] auto hashMessage(HashAlgorithm hash, mem::ConstByteSpan message) -> mem::ByteBlock;
/// Check whether signature parameters satisfy optional id-RSASSA-PSS key restrictions.
[[nodiscard]] auto parametersMatchKey(
    const SignatureParameters &signature, const std::optional<SignatureParameters> &restrictions) noexcept -> bool;

}
