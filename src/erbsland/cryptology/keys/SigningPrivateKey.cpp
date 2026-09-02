// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SigningPrivateKey.hpp"

#include "../CryptologyError.hpp"
#include "../impl/algorithm/ecdsa_signature/EcdsaSignature.hpp"
#include "../impl/algorithm/ecdsa_signature/EcdsaSigner.hpp"
#include "../impl/algorithm/ed25519_signature/Ed25519Signer.hpp"
#include "../impl/algorithm/rsa_signature/RsaSignature.hpp"
#include "../impl/algorithm/rsa_signature/RsaSigner.hpp"
#include "../impl/CryptologyOids.hpp"
#include "../impl/DerEncoder.hpp"
#include "../impl/EncryptedPrivateKeyCodec.hpp"
#include "../impl/PemCodec.hpp"
#include "../impl/PemDerFileTools.hpp"
#include "../impl/PrivateKeyParser.hpp"
#include "../impl/SecureEraseGuard.hpp"
#include "../impl/SigningKeyEncoding.hpp"
#include "../impl/SigningKeyGenerator.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../path/Path.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringEncoder.hpp"
#include "../../text/StringEncoding.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cryptology {

using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

SigningPrivateKey::SigningPrivateKey(
    const SigningKeyAlgorithm algorithm, const ConstByteSpan privateData, PublicKey publicKey) :
    _algorithm{algorithm}, _privateData{privateData}, _publicKey{std::move(publicKey)} {
}

SigningPrivateKey::~SigningPrivateKey() {
    secureErase();
}

auto SigningPrivateKey::signTlsCertificateVerify(const TlsSignatureScheme scheme, const ConstByteSpan message) const
    -> ByteBlock {
    if (isEmpty()) {
        throw err::LogicError{"A private key is required for signing."_el};
    }
    if (!supports(scheme)) {
        throw err::ParameterError{"The TLS signature scheme is incompatible with this private key."_el, "scheme"_el};
    }
    auto result = ByteBlock{};
    _privateData.withUnprotectedData([&](const ConstByteSpan privateData) -> void {
        // RFC 8446 section 4.4.3: the caller supplies the complete signed content; the selected primitive signs it
        // exactly and the protected-data callback erases its temporary plaintext on success or exception.
        switch (_algorithm) {
        case SigningKeyAlgorithm::Ed25519:
            result = impl::ed25519_signer::sign(privateData, message);
            return;
        case SigningKeyAlgorithm::EcdsaP256:
            result = impl::ecdsa_signer::sign(privateData, message);
            return;
        case SigningKeyAlgorithm::EcdsaP384:
            result = impl::ecdsa_signer::sign(privateData, message, impl::NistPrimeCurve::Name::P384);
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
    case SigningKeyAlgorithm::EcdsaP384:
        return scheme == TlsSignatureScheme::EcdsaSecp384r1Sha384;
    case SigningKeyAlgorithm::Rsa:
        try {
            const auto key = impl::rsa_signature::decodePublicKey(_publicKey);
            const auto keyOid = _publicKey.algorithm().oid().toString();
            const auto rsaeScheme =
                scheme == TlsSignatureScheme::RsaPssRsaeSha256 || scheme == TlsSignatureScheme::RsaPssRsaeSha384;
            const auto pssScheme =
                scheme == TlsSignatureScheme::RsaPssPssSha256 || scheme == TlsSignatureScheme::RsaPssPssSha384;
            if ((rsaeScheme && keyOid != impl::cryptology_oids::rsaEncryption) ||
                (pssScheme && keyOid != impl::cryptology_oids::rsaPss) || (!rsaeScheme && !pssScheme)) {
                return false;
            }
            const auto hash = HashAlgorithm{
                (scheme == TlsSignatureScheme::RsaPssRsaeSha256 || scheme == TlsSignatureScheme::RsaPssPssSha256)
                    ? HashAlgorithm::Sha2_256
                    : HashAlgorithm::Sha2_384};
            return impl::rsa_signature::parametersMatchKey(
                {impl::rsa_signature::Padding::Pss, hash, hash.digestSize().toSizeT()}, key.restrictions);
        } catch (const err::RuntimeError &) {
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
            return publicKeyValue.algorithm().oid().toString() == impl::cryptology_oids::ed25519 &&
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
        case SigningKeyAlgorithm::EcdsaP384: {
            const auto parameters =
                impl::ecdsa_signature::Parameters{impl::NistPrimeCurve::Name::P384, HashAlgorithm::Sha2_384};
            const auto curve = impl::NistPrimeCurve{impl::NistPrimeCurve::Name::P384};
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
            if (expectedOid != candidateOid || expected.modulus.compare(candidate.modulus) != 0 ||
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
    } catch (const err::RuntimeError &) {
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

auto SigningPrivateKey::toDer() const -> ByteBlock {
    if (isEmpty()) {
        throw err::LogicError{"A private key is required for encoding."_el};
    }
    auto encoder = impl::DerEncoder{};
    _privateData.withUnprotectedData([&](const ConstByteSpan privateData) -> void {
        impl::signing_key_encoding::appendPrivateKey(encoder, _algorithm, privateData, _publicKey);
    });
    auto result = encoder.encoded();
    result.markAsSensitive();
    return result;
}

auto SigningPrivateKey::toPem() const -> String {
    auto der = toDer();
    const auto eraseGuard = impl::SecureEraseGuard{der};
    auto result = impl::PemCodec{der, impl::PemLabel::PrivateKey}.encode();
    result.markAsSensitive();
    return result;
}

void SigningPrivateKey::writeToFile(const path::Path &path, const PemDerFormat format) const {
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::PrivateKey, true};
    if (file.outputFormat(format) == PemDerFormat::Pem) {
        file.writePem(toPem());
    } else {
        file.writeDer(toDer());
    }
}

auto SigningPrivateKey::toEncryptedDer(const String &password) const -> ByteBlock {
    if (password.isEmpty()) {
        throw err::ParameterError{"A private-key password must not be empty."_el, "password"_el};
    }
    auto passwordBytes = StringEncoder{password}.encode(StringEncoding::Utf8, StringBomMode::Reject);
    passwordBytes.markAsSensitive();
    const auto passwordEraseGuard = impl::SecureEraseGuard{passwordBytes};
    auto plaintext = toDer();
    const auto plaintextEraseGuard = impl::SecureEraseGuard{plaintext};
    return impl::encrypted_private_key_codec::encrypt(plaintext.span(), passwordBytes.span());
}

auto SigningPrivateKey::toEncryptedPem(const String &password) const -> String {
    return impl::PemCodec{toEncryptedDer(password), impl::PemLabel::EncryptedPrivateKey}.encode();
}

void SigningPrivateKey::writeEncryptedToFile(
    const path::Path &path, const String &password, const PemDerFormat format) const {
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::PrivateKey, true};
    if (file.outputFormat(format) == PemDerFormat::Pem) {
        file.writePem(toEncryptedPem(password));
    } else {
        file.writeDer(toEncryptedDer(password));
    }
}

auto SigningPrivateKey::generate(const SigningKeyProfile profile) -> SigningPrivateKey {
    return impl::SigningKeyGenerator{profile}.generate();
}

auto SigningPrivateKey::fromDer(const ByteBlock &der) noexcept -> SigningPrivateKey {
    try {
        return fromDerOrThrow(der);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto SigningPrivateKey::fromDerOrThrow(const ByteBlock &der) -> SigningPrivateKey {
    return impl::PrivateKeyParser{der}.parse();
}

auto SigningPrivateKey::fromPem(const String &pem) noexcept -> SigningPrivateKey {
    try {
        return fromPemOrThrow(pem);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto SigningPrivateKey::fromPemOrThrow(const String &pem) -> SigningPrivateKey {
    auto der = impl::PrivateKeyParser::decodePem(pem);
    const auto eraseGuard = impl::SecureEraseGuard{der};
    return fromDerOrThrow(der);
}

auto SigningPrivateKey::fromFile(const path::Path &path, const PemDerFormat format) noexcept -> SigningPrivateKey {
    try {
        return fromFileOrThrow(path, format);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto SigningPrivateKey::fromFileOrThrow(const path::Path &path, const PemDerFormat format) -> SigningPrivateKey {
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::PrivateKey, true};
    auto data = file.read();
    data.markAsSensitive();
    const auto eraseGuard = impl::SecureEraseGuard{data};
    if (file.inputFormat(data, format) == PemDerFormat::Pem) {
        return fromPemOrThrow(impl::PemDerFileTools::toPemText(data));
    }
    return fromDerOrThrow(data);
}

auto SigningPrivateKey::fromEncryptedDer(const ByteBlock &der, const String &password) noexcept -> SigningPrivateKey {
    try {
        return fromEncryptedDerOrThrow(der, password);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto SigningPrivateKey::fromEncryptedDerOrThrow(const ByteBlock &der, const String &password) -> SigningPrivateKey {
    if (password.isEmpty()) {
        throw err::ParameterError{"A private-key password must not be empty."_el, "password"_el};
    }
    try {
        auto passwordBytes = StringEncoder{password}.encode(StringEncoding::Utf8, StringBomMode::Reject);
        passwordBytes.markAsSensitive();
        const auto passwordEraseGuard = impl::SecureEraseGuard{passwordBytes};
        auto plaintext = impl::encrypted_private_key_codec::decrypt(der, passwordBytes.span());
        const auto plaintextEraseGuard = impl::SecureEraseGuard{plaintext};
        return fromDerOrThrow(plaintext);
    } catch (const err::RuntimeError &) {
        throw CryptologyError{"Private-key decryption failed."_el};
    }
}

auto SigningPrivateKey::fromEncryptedPem(const String &pem, const String &password) noexcept -> SigningPrivateKey {
    try {
        return fromEncryptedPemOrThrow(pem, password);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto SigningPrivateKey::fromEncryptedPemOrThrow(const String &pem, const String &password) -> SigningPrivateKey {
    if (password.isEmpty()) {
        throw err::ParameterError{"A private-key password must not be empty."_el, "password"_el};
    }
    try {
        const auto values = impl::PemCodec{pem, impl::PemLabel::EncryptedPrivateKey}.decode();
        return fromEncryptedDerOrThrow(values.getRefOrThrow(ItemIndex::zero()), password);
    } catch (const err::RuntimeError &) {
        throw CryptologyError{"Private-key decryption failed."_el};
    }
}

auto SigningPrivateKey::fromEncryptedFile(
    const path::Path &path, const String &password, const PemDerFormat format) noexcept -> SigningPrivateKey {
    try {
        return fromEncryptedFileOrThrow(path, password, format);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto SigningPrivateKey::fromEncryptedFileOrThrow(
    const path::Path &path, const String &password, const PemDerFormat format) -> SigningPrivateKey {
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::PrivateKey, true};
    const auto data = file.read();
    if (file.inputFormat(data, format) == PemDerFormat::Pem) {
        return fromEncryptedPemOrThrow(impl::PemDerFileTools::toPemText(data), password);
    }
    return fromEncryptedDerOrThrow(data, password);
}

}
