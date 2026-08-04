// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsSignatureScheme.hpp"

#include "../x509/X509AlgorithmIdentifier.hpp"

#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"

#include <cstdint>
#include <span>

namespace erbsland::cryptology {

using namespace text::literals;

auto TlsSignatureScheme::isAllowedForCertificateVerify() const noexcept -> bool {
    // RFC 8446 section 4.2.3 defines PKCS#1 v1.5 schemes only for certificate signatures, not signed TLS messages.
    switch (_value) {
    case RsaPkcs1Sha256:
    case RsaPkcs1Sha384:
        return false;
    case EcdsaSecp256r1Sha256:
    case EcdsaSecp384r1Sha384:
    case RsaPssRsaeSha256:
    case RsaPssRsaeSha384:
    case Ed25519:
    case RsaPssPssSha256:
    case RsaPssPssSha384:
        return true;
    }
    return false;
}

auto TlsSignatureScheme::isAllowedForCertificateSignature() const noexcept -> bool {
    // RFC 8446 section 4.2.3 permits every supported mapping here for signatures appearing in certificates.
    switch (_value) {
    case RsaPkcs1Sha256:
    case RsaPkcs1Sha384:
    case EcdsaSecp256r1Sha256:
    case EcdsaSecp384r1Sha384:
    case RsaPssRsaeSha256:
    case RsaPssRsaeSha384:
    case Ed25519:
    case RsaPssPssSha256:
    case RsaPssPssSha384:
        return true;
    }
    return false;
}

auto TlsSignatureScheme::toString() const -> text::String {
    // RFC 8446 section 4.2.3 assigns these stable names to the corresponding 16-bit values.
    switch (_value) {
    case RsaPkcs1Sha256:
        return "rsa_pkcs1_sha256"_el;
    case RsaPkcs1Sha384:
        return "rsa_pkcs1_sha384"_el;
    case EcdsaSecp256r1Sha256:
        return "ecdsa_secp256r1_sha256"_el;
    case EcdsaSecp384r1Sha384:
        return "ecdsa_secp384r1_sha384"_el;
    case RsaPssRsaeSha256:
        return "rsa_pss_rsae_sha256"_el;
    case RsaPssRsaeSha384:
        return "rsa_pss_rsae_sha384"_el;
    case Ed25519:
        return "ed25519"_el;
    case RsaPssPssSha256:
        return "rsa_pss_pss_sha256"_el;
    case RsaPssPssSha384:
        return "rsa_pss_pss_sha384"_el;
    }
    return {};
}

auto TlsSignatureScheme::fromRawValue(const uint16_t value) noexcept -> std::optional<TlsSignatureScheme> {
    // RFC 8446 section 4.2.3 lists the code points in signature-construction order.
    switch (value) {
    case RsaPkcs1Sha256:
        return TlsSignatureScheme{RsaPkcs1Sha256};
    case RsaPkcs1Sha384:
        return TlsSignatureScheme{RsaPkcs1Sha384};
    case EcdsaSecp256r1Sha256:
        return TlsSignatureScheme{EcdsaSecp256r1Sha256};
    case EcdsaSecp384r1Sha384:
        return TlsSignatureScheme{EcdsaSecp384r1Sha384};
    case RsaPssRsaeSha256:
        return TlsSignatureScheme{RsaPssRsaeSha256};
    case RsaPssRsaeSha384:
        return TlsSignatureScheme{RsaPssRsaeSha384};
    case Ed25519:
        return TlsSignatureScheme{Ed25519};
    case RsaPssPssSha256:
        return TlsSignatureScheme{RsaPssPssSha256};
    case RsaPssPssSha384:
        return TlsSignatureScheme{RsaPssPssSha384};
    default:
        return std::nullopt;
    }
}

auto TlsSignatureScheme::fromRawValueOrThrow(const uint16_t value) -> TlsSignatureScheme {
    if (const auto result = fromRawValue(value); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unsupported TLS signature scheme."};
}

auto TlsSignatureScheme::signatureAlgorithmIdentifier() const -> X509AlgorithmIdentifier {
    // RFC 8446 section 4.2.3 maps PKCS#1 v1.5 schemes to the RFC 8017 SHA-2 AlgorithmIdentifier values.
    switch (_value) {
    case RsaPkcs1Sha256: {
        static constexpr uint8_t cDer[]{
            0x30U, 0x0dU, 0x06U, 0x09U, 0x2aU, 0x86U, 0x48U, 0x86U, 0xf7U, 0x0dU, 0x01U, 0x01U, 0x0bU, 0x05U, 0x00U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    case RsaPkcs1Sha384: {
        static constexpr uint8_t cDer[]{
            0x30U, 0x0dU, 0x06U, 0x09U, 0x2aU, 0x86U, 0x48U, 0x86U, 0xf7U, 0x0dU, 0x01U, 0x01U, 0x0cU, 0x05U, 0x00U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    // RFC 8446 section 4.2.3 and RFC 5758 section 3.2 map each named-curve ECDSA scheme to an absent-parameter OID.
    case EcdsaSecp256r1Sha256: {
        static constexpr uint8_t cDer[]{
            0x30U, 0x0aU, 0x06U, 0x08U, 0x2aU, 0x86U, 0x48U, 0xceU, 0x3dU, 0x04U, 0x03U, 0x02U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    case EcdsaSecp384r1Sha384: {
        static constexpr uint8_t cDer[]{
            0x30U, 0x0aU, 0x06U, 0x08U, 0x2aU, 0x86U, 0x48U, 0xceU, 0x3dU, 0x04U, 0x03U, 0x03U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    // RFC 8446 section 4.2.3 requires matching message/MGF1 hashes, digest-sized salt, and trailer field 1 for PSS.
    // The fixed DER makes the hashes and salt explicit; canonical DER omits trailerField because its required value 1
    // is the ASN.1 DEFAULT.
    case RsaPssRsaeSha256:
    case RsaPssPssSha256: {
        static constexpr uint8_t cDer[]{
            0x30U,
            0x41U,
            0x06U,
            0x09U,
            0x2aU,
            0x86U,
            0x48U,
            0x86U,
            0xf7U,
            0x0dU,
            0x01U,
            0x01U,
            0x0aU,
            0x30U,
            0x34U,
            0xa0U,
            0x0fU,
            0x30U,
            0x0dU,
            0x06U,
            0x09U,
            0x60U,
            0x86U,
            0x48U,
            0x01U,
            0x65U,
            0x03U,
            0x04U,
            0x02U,
            0x01U,
            0x05U,
            0x00U,
            0xa1U,
            0x1cU,
            0x30U,
            0x1aU,
            0x06U,
            0x09U,
            0x2aU,
            0x86U,
            0x48U,
            0x86U,
            0xf7U,
            0x0dU,
            0x01U,
            0x01U,
            0x08U,
            0x30U,
            0x0dU,
            0x06U,
            0x09U,
            0x60U,
            0x86U,
            0x48U,
            0x01U,
            0x65U,
            0x03U,
            0x04U,
            0x02U,
            0x01U,
            0x05U,
            0x00U,
            0xa2U,
            0x03U,
            0x02U,
            0x01U,
            0x20U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    case RsaPssRsaeSha384:
    case RsaPssPssSha384: {
        static constexpr uint8_t cDer[]{
            0x30U,
            0x41U,
            0x06U,
            0x09U,
            0x2aU,
            0x86U,
            0x48U,
            0x86U,
            0xf7U,
            0x0dU,
            0x01U,
            0x01U,
            0x0aU,
            0x30U,
            0x34U,
            0xa0U,
            0x0fU,
            0x30U,
            0x0dU,
            0x06U,
            0x09U,
            0x60U,
            0x86U,
            0x48U,
            0x01U,
            0x65U,
            0x03U,
            0x04U,
            0x02U,
            0x02U,
            0x05U,
            0x00U,
            0xa1U,
            0x1cU,
            0x30U,
            0x1aU,
            0x06U,
            0x09U,
            0x2aU,
            0x86U,
            0x48U,
            0x86U,
            0xf7U,
            0x0dU,
            0x01U,
            0x01U,
            0x08U,
            0x30U,
            0x0dU,
            0x06U,
            0x09U,
            0x60U,
            0x86U,
            0x48U,
            0x01U,
            0x65U,
            0x03U,
            0x04U,
            0x02U,
            0x02U,
            0x05U,
            0x00U,
            0xa2U,
            0x03U,
            0x02U,
            0x01U,
            0x30U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    // RFC 8446 section 4.2.3 and RFC 8410 sections 3 and 6 identify pure Ed25519 with absent parameters.
    case Ed25519: {
        static constexpr uint8_t cDer[]{0x30U, 0x05U, 0x06U, 0x03U, 0x2bU, 0x65U, 0x70U};
        return X509AlgorithmIdentifier::fromDerOrThrow(mem::ByteBlock::fromSpan(std::span{cDer}));
    }
    }
    throw err::ParseError{"Unsupported TLS signature scheme."};
}

auto TlsSignatureScheme::requiresRsaEncryptionKey() const noexcept -> bool {
    return _value == RsaPssRsaeSha256 || _value == RsaPssRsaeSha384;
}

auto TlsSignatureScheme::requiresRsaPssKey() const noexcept -> bool {
    return _value == RsaPssPssSha256 || _value == RsaPssPssSha384;
}

}
