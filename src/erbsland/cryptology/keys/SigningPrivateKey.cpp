// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SigningPrivateKey.hpp"

#include "../impl/algorithm/ecdsa_signature/EcdsaSignature.hpp"
#include "../impl/algorithm/ecdsa_signature/EcdsaSigner.hpp"
#include "../impl/algorithm/ed25519_signature/Ed25519Signer.hpp"
#include "../impl/algorithm/rsa_signature/RsaSignature.hpp"
#include "../impl/algorithm/rsa_signature/RsaSigner.hpp"
#include "../impl/PrivateKeyParser.hpp"
#include "../impl/SecureEraseGuard.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

SigningPrivateKey::SigningPrivateKey(
    const SigningKeyAlgorithm algorithm, const mem::ConstByteSpan privateData, PublicKey publicKey) :
    _algorithm{algorithm}, _privateData{privateData}, _publicKey{std::move(publicKey)} {
}

SigningPrivateKey::~SigningPrivateKey() {
    secureErase();
}

auto SigningPrivateKey::signTlsCertificateVerify(
    const TlsSignatureScheme scheme, const mem::ConstByteSpan message) const -> mem::ByteBlock {
    if (isEmpty()) {
        throw err::LogicError{"A private key is required for signing."_el};
    }
    if (!supports(scheme)) {
        throw err::ParameterError{"The TLS signature scheme is incompatible with this private key."_el, "scheme"_el};
    }
    auto result = mem::ByteBlock{};
    _privateData.withUnprotectedData([&](const mem::ConstByteSpan privateData) -> void {
        // RFC 8446 section 4.4.3: the caller supplies the complete signed content; the selected primitive signs it
        // exactly and the protected-data callback erases its temporary plaintext on success or exception.
        switch (_algorithm) {
        case SigningKeyAlgorithm::Ed25519:
            result = impl::ed25519_signer::sign(privateData, message);
            return;
        case SigningKeyAlgorithm::EcdsaP256:
            result = impl::ecdsa_signer::sign(privateData, message);
            return;
        case SigningKeyAlgorithm::Rsa:
            result = impl::rsa_signer::sign(privateData, scheme, message, _publicKey);
            return;
        case SigningKeyAlgorithm::Unknown:
            break;
        }
        throw err::LogicError{"The private-key algorithm has no signing implementation."_el};
    });
    return result;
}

void SigningPrivateKey::secureErase() noexcept {
    _privateData.secureErase();
    _publicKey = {};
    _algorithm = SigningKeyAlgorithm::Unknown;
}

auto SigningPrivateKey::supports(const TlsSignatureScheme scheme) const noexcept -> bool {
    if (isEmpty() || !scheme.isAllowedForCertificateVerify()) {
        return false;
    }
    switch (_algorithm) {
    case SigningKeyAlgorithm::Ed25519:
        return scheme == TlsSignatureScheme::Ed25519;
    case SigningKeyAlgorithm::EcdsaP256:
        return scheme == TlsSignatureScheme::EcdsaSecp256r1Sha256;
    case SigningKeyAlgorithm::Rsa:
        try {
            const auto key = impl::rsa_signature::decodePublicKey(_publicKey);
            const auto keyOid = _publicKey.algorithm().oid().toString();
            const auto rsaeScheme =
                scheme == TlsSignatureScheme::RsaPssRsaeSha256 || scheme == TlsSignatureScheme::RsaPssRsaeSha384;
            const auto pssScheme =
                scheme == TlsSignatureScheme::RsaPssPssSha256 || scheme == TlsSignatureScheme::RsaPssPssSha384;
            if ((rsaeScheme && keyOid != "1.2.840.113549.1.1.1"_el) ||
                (pssScheme && keyOid != "1.2.840.113549.1.1.10"_el) || (!rsaeScheme && !pssScheme)) {
                return false;
            }
            const auto hash = HashAlgorithm{
                (scheme == TlsSignatureScheme::RsaPssRsaeSha256 || scheme == TlsSignatureScheme::RsaPssPssSha256)
                    ? HashAlgorithm::Sha2_256
                    : HashAlgorithm::Sha2_384};
            return impl::rsa_signature::parametersMatchKey(
                {impl::rsa_signature::Padding::Pss, hash, hash.digestSize().toSizeT()}, key.restrictions);
        } catch (...) {
            return false;
        }
    case SigningKeyAlgorithm::Unknown:
        return false;
    }
    return false;
}

auto SigningPrivateKey::matches(const PublicKey &publicKeyValue) const -> bool {
    if (isEmpty() || publicKeyValue.isEmpty()) {
        return false;
    }
    try {
        switch (_algorithm) {
        case SigningKeyAlgorithm::Ed25519:
            // RFC 8410 sections 3 and 4: Ed25519 parameters are absent and ENC(A) is one exact canonical point.
            return publicKeyValue.algorithm().oid().toString() == "1.3.101.112"_el &&
                publicKeyValue.algorithm().parameters().isEmpty() && publicKeyValue.unusedBitCount() == 0U &&
                _publicKey.keyData().isEqualConstTime(publicKeyValue.keyData());
        case SigningKeyAlgorithm::EcdsaP256: {
            // RFC 5480 section 2.2: normalize compressed or uncompressed P-256 points to affine coordinates.
            const auto parameters =
                impl::ecdsa_signature::Parameters{impl::NistPrimeCurve::Name::P256, HashAlgorithm::Sha2_256};
            const auto curve = impl::NistPrimeCurve{impl::NistPrimeCurve::Name::P256};
            const auto expected = curve.toAffine(impl::ecdsa_signature::decodePublicKey(_publicKey, parameters));
            const auto candidate = curve.toAffine(impl::ecdsa_signature::decodePublicKey(publicKeyValue, parameters));
            return expected.has_value() && candidate.has_value() && curve.compare(expected->x, candidate->x) == 0 &&
                curve.compare(expected->y, candidate->y) == 0;
        }
        case SigningKeyAlgorithm::Rsa: {
            // RFC 3279 section 2.3.1 and RFC 4055 section 3.1: compare n, e, key OID, and semantic PSS restrictions.
            const auto expected = impl::rsa_signature::decodePublicKey(_publicKey);
            const auto candidate = impl::rsa_signature::decodePublicKey(publicKeyValue);
            const auto expectedOid = _publicKey.algorithm().oid().toString();
            const auto candidateOid = publicKeyValue.algorithm().oid().toString();
            if (expectedOid != candidateOid || impl::rsa_signature::compare(expected.modulus, candidate.modulus) != 0 ||
                expected.exponent != candidate.exponent ||
                expected.restrictions.has_value() != candidate.restrictions.has_value()) {
                return false;
            }
            if (!expected.restrictions.has_value()) {
                return true;
            }
            return expected.restrictions->padding == candidate.restrictions->padding &&
                expected.restrictions->hash == candidate.restrictions->hash &&
                expected.restrictions->saltLength == candidate.restrictions->saltLength;
        }
        case SigningKeyAlgorithm::Unknown:
            return false;
        }
    } catch (...) {
        return false;
    }
    return false;
}

auto SigningPrivateKey::publicKey() const -> PublicKey {
    if (isEmpty()) {
        throw err::LogicError{"A private key is required to access its public key."_el};
    }
    return _publicKey;
}

auto SigningPrivateKey::fromDer(const mem::ByteBlock &der) noexcept -> SigningPrivateKey {
    try {
        return fromDerOrThrow(der);
    } catch (...) {
        return {};
    }
}

auto SigningPrivateKey::fromDerOrThrow(const mem::ByteBlock &der) -> SigningPrivateKey {
    return impl::PrivateKeyParser{der}.parse();
}

auto SigningPrivateKey::fromPem(const text::String &pem) noexcept -> SigningPrivateKey {
    try {
        return fromPemOrThrow(pem);
    } catch (...) {
        return {};
    }
}

auto SigningPrivateKey::fromPemOrThrow(const text::String &pem) -> SigningPrivateKey {
    auto der = impl::PrivateKeyParser::decodePem(pem);
    const auto eraseGuard = impl::SecureEraseGuard{der};
    return fromDerOrThrow(der);
}

}
