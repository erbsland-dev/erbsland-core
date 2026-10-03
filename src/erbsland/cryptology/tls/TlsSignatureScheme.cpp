// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsSignatureScheme.hpp"

#include "../x509/X509AlgorithmIdentifier.hpp"

#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace erbsland::cryptology {

using namespace mem;
using namespace text::literals;

auto TlsSignatureScheme::isAllowedForCertificateVerify() const noexcept -> bool {
    // RFC 8446 section 4.2.3 defines PKCS#1 v1.5 schemes only for certificate signatures, not signed TLS messages.
    switch (_value) {
    case RsaPkcs1Sha256:
    case RsaPkcs1Sha384:
    case RsaPkcs1Sha512:
        return false;
    case EcdsaSecp256r1Sha256:
    case EcdsaSecp384r1Sha384:
    case RsaPssRsaeSha256:
    case RsaPssRsaeSha384:
    case RsaPssRsaeSha512:
    case Ed25519:
    case RsaPssPssSha256:
    case RsaPssPssSha384:
    case RsaPssPssSha512:
        return true;
    }
    return false;
}

auto TlsSignatureScheme::isAllowedForCertificateSignature() const noexcept -> bool {
    // RFC 8446 section 4.2.3 permits every supported mapping here for signatures appearing in certificates.
    switch (_value) {
    case RsaPkcs1Sha256:
    case RsaPkcs1Sha384:
    case RsaPkcs1Sha512:
    case EcdsaSecp256r1Sha256:
    case EcdsaSecp384r1Sha384:
    case RsaPssRsaeSha256:
    case RsaPssRsaeSha384:
    case RsaPssRsaeSha512:
    case Ed25519:
    case RsaPssPssSha256:
    case RsaPssPssSha384:
    case RsaPssPssSha512:
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
    case RsaPkcs1Sha512:
        return "rsa_pkcs1_sha512"_el;
    case EcdsaSecp256r1Sha256:
        return "ecdsa_secp256r1_sha256"_el;
    case EcdsaSecp384r1Sha384:
        return "ecdsa_secp384r1_sha384"_el;
    case RsaPssRsaeSha256:
        return "rsa_pss_rsae_sha256"_el;
    case RsaPssRsaeSha384:
        return "rsa_pss_rsae_sha384"_el;
    case RsaPssRsaeSha512:
        return "rsa_pss_rsae_sha512"_el;
    case Ed25519:
        return "ed25519"_el;
    case RsaPssPssSha256:
        return "rsa_pss_pss_sha256"_el;
    case RsaPssPssSha384:
        return "rsa_pss_pss_sha384"_el;
    case RsaPssPssSha512:
        return "rsa_pss_pss_sha512"_el;
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
    case RsaPkcs1Sha512:
        return TlsSignatureScheme{RsaPkcs1Sha512};
    case EcdsaSecp256r1Sha256:
        return TlsSignatureScheme{EcdsaSecp256r1Sha256};
    case EcdsaSecp384r1Sha384:
        return TlsSignatureScheme{EcdsaSecp384r1Sha384};
    case RsaPssRsaeSha256:
        return TlsSignatureScheme{RsaPssRsaeSha256};
    case RsaPssRsaeSha384:
        return TlsSignatureScheme{RsaPssRsaeSha384};
    case RsaPssRsaeSha512:
        return TlsSignatureScheme{RsaPssRsaeSha512};
    case Ed25519:
        return TlsSignatureScheme{Ed25519};
    case RsaPssPssSha256:
        return TlsSignatureScheme{RsaPssPssSha256};
    case RsaPssPssSha384:
        return TlsSignatureScheme{RsaPssPssSha384};
    case RsaPssPssSha512:
        return TlsSignatureScheme{RsaPssPssSha512};
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
    // RFC 8017 appendix A.2: PKCS#1 v1.5 SHA-2 identifiers differ only in the final OID component.
    if (_value == RsaPkcs1Sha256 || _value == RsaPkcs1Sha384 || _value == RsaPkcs1Sha512) {
        auto der = std::array<uint8_t, 15U>{
            0x30U, 0x0dU, 0x06U, 0x09U, 0x2aU, 0x86U, 0x48U, 0x86U, 0xf7U, 0x0dU, 0x01U, 0x01U, 0x0bU, 0x05U, 0x00U};
        if (_value == RsaPkcs1Sha384) {
            der[12U] = 0x0cU;
        } else if (_value == RsaPkcs1Sha512) {
            der[12U] = 0x0dU;
        }
        return X509AlgorithmIdentifier::fromDerOrThrow(ByteBlock::fromSpan(std::span{der}));
    }
    // RFC 5758 section 3.2: ECDSA signature OIDs have absent parameters.
    if (_value == EcdsaSecp256r1Sha256 || _value == EcdsaSecp384r1Sha384) {
        auto der = std::array<uint8_t, 12U>{
            0x30U, 0x0aU, 0x06U, 0x08U, 0x2aU, 0x86U, 0x48U, 0xceU, 0x3dU, 0x04U, 0x03U, 0x02U};
        if (_value == EcdsaSecp384r1Sha384) {
            der[11U] = 0x03U;
        }
        return X509AlgorithmIdentifier::fromDerOrThrow(ByteBlock::fromSpan(std::span{der}));
    }
    // RFC 8446 section 4.2.3: PSS uses matching message/MGF1 hashes and digest-sized salt.
    // RFC 4055 section 3.1: encode both hashes and salt explicitly; omit the DEFAULT trailerField of 1.
    if (requiresRsaEncryptionKey() || requiresRsaPssKey()) {
        auto hashIdentifier = uint8_t{1U};
        auto saltLength = uint8_t{32U};
        if (_value == RsaPssRsaeSha384 || _value == RsaPssPssSha384) {
            hashIdentifier = 2U;
            saltLength = 48U;
        } else if (_value == RsaPssRsaeSha512 || _value == RsaPssPssSha512) {
            hashIdentifier = 3U;
            saltLength = 64U;
        }
        const auto der = std::array<uint8_t, 67U>{
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
            hashIdentifier,
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
            hashIdentifier,
            0x05U,
            0x00U,
            0xa2U,
            0x03U,
            0x02U,
            0x01U,
            saltLength};
        return X509AlgorithmIdentifier::fromDerOrThrow(ByteBlock::fromSpan(std::span{der}));
    }
    // RFC 8410 sections 3 and 6: Ed25519 has absent parameters.
    if (_value == Ed25519) {
        static constexpr uint8_t cDer[]{0x30U, 0x05U, 0x06U, 0x03U, 0x2bU, 0x65U, 0x70U};
        return X509AlgorithmIdentifier::fromDerOrThrow(ByteBlock::fromSpan(std::span{cDer}));
    }
    throw err::ParseError{"Unsupported TLS signature scheme."_el};
}

auto TlsSignatureScheme::requiresRsaEncryptionKey() const noexcept -> bool {
    return _value == RsaPssRsaeSha256 || _value == RsaPssRsaeSha384 || _value == RsaPssRsaeSha512;
}

auto TlsSignatureScheme::requiresRsaPssKey() const noexcept -> bool {
    return _value == RsaPssPssSha256 || _value == RsaPssPssSha384 || _value == RsaPssPssSha512;
}

}
