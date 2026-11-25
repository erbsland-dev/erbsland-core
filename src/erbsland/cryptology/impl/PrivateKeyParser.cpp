// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PrivateKeyParser.hpp"

#include "CryptologyAsn1Limits.hpp"
#include "CryptologyOids.hpp"
#include "EcPrivateScalarPolicy.hpp"
#include "PemCodec.hpp"
#include "SecureEraseGuard.hpp"
#include "SigningKeyEncoding.hpp"

#include "algorithm/ecdsa_signature/EcdsaSigner.hpp"
#include "algorithm/ed25519_signature/Ed25519Signer.hpp"
#include "algorithm/rsa_signature/RsaSigner.hpp"

#include "../../data/asn1/Asn1UniversalType.hpp"
#include "../../data/der/DerEncoder.hpp"
#include "../../data/der/DerParser.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cryptology::impl {

using data::asn1::Asn1Node;
using data::asn1::Asn1Tag;
using data::asn1::Asn1TagClass;
using data::asn1::Asn1UniversalType;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto PrivateKeyParser::parse() const -> SigningPrivateKey {
    // RFC 5208 section 6: PrivateKeyInfo is exactly version, privateKeyAlgorithm, and privateKey. Attributes are
    // intentionally rejected in the initial server-identity profile so no unreviewed metadata survives import.
    const auto root = data::der::DerParser::parseOrThrow(_der, cCryptologyAsn1Limits);
    requireNode(root, Asn1UniversalType::Sequence, true, "PrivateKeyInfo"_el);
    if (root.childCount() != ItemCount{3U}) {
        throwParseError("PKCS#8 PrivateKeyInfo must contain exactly version, algorithm, and private key."_el);
    }
    if (decodeSmallInteger(root.child(ItemIndex{0U}), "version"_el) != 0U) {
        throwParseError("PKCS#8 PrivateKeyInfo version must be zero."_el);
    }
    const auto algorithm = root.child(ItemIndex{1U});
    requireNode(algorithm, Asn1UniversalType::Sequence, true, "privateKeyAlgorithm"_el);
    if (algorithm.childCount() < ItemCount{1U} || algorithm.childCount() > ItemCount{2U}) {
        throwParseError("PKCS#8 privateKeyAlgorithm must contain an OID and optional parameters."_el);
    }
    const auto oid = algorithm.child(ItemIndex{0U}).toObjectIdentifier();
    if (!oid.has_value()) {
        throwParseError("PKCS#8 privateKeyAlgorithm contains no object identifier."_el);
    }
    const auto privateKeyNode = root.child(ItemIndex{2U});
    requireNode(privateKeyNode, Asn1UniversalType::OctetString, false, "privateKey"_el);
    auto privateKey = privateKeyNode.contentData();
    privateKey.markAsSensitive();
    const auto privateKeyEraseGuard = SecureEraseGuard{privateKey};
    if (oid->toString() == cryptology_oids::ed25519) {
        return parseEd25519(algorithm, privateKey);
    }
    if (oid->toString() == cryptology_oids::ecPublicKey) {
        return parseEcdsa(algorithm, privateKey);
    }
    if (oid->toString() == cryptology_oids::rsaEncryption || oid->toString() == cryptology_oids::rsaPss) {
        return parseRsa(algorithm, privateKey);
    }
    throwParseError("PKCS#8 contains an unsupported private-key algorithm."_el);
}

auto PrivateKeyParser::decodePem(const String &pem) -> ByteBlock {
    auto blocks = PemCodec{pem, PemLabel::PrivateKey}.decode();
    auto result = blocks.first();
    result.markAsSensitive();
    return result;
}

auto PrivateKeyParser::parseEd25519(const Asn1Node &algorithm, const ByteBlock &privateKey) const -> SigningPrivateKey {
    // RFC 8410 sections 3 and 7: id-Ed25519 parameters are absent and CurvePrivateKey is an inner OCTET STRING.
    if (algorithm.childCount() != ItemCount{1U}) {
        throwParseError("Ed25519 PKCS#8 algorithm parameters must be absent."_el);
    }
    const auto inner = data::der::DerParser::parseOrThrow(privateKey, cCryptologyAsn1Limits);
    requireNode(inner, Asn1UniversalType::OctetString, false, "CurvePrivateKey"_el);
    auto seed = inner.contentData();
    seed.markAsSensitive();
    const auto seedEraseGuard = SecureEraseGuard{seed};
    if (seed.length() != ByteCount{32U}) {
        throwParseError("Ed25519 CurvePrivateKey must contain exactly 32 octets."_el);
    }

    // RFC 8410 section 4: SubjectPublicKeyInfo is SEQUENCE { id-Ed25519, BIT STRING ENC(A) }.
    const auto encodedPublicKey = ed25519_signer::publicKey(seed.span());
    auto encoder = data::der::DerEncoder{};
    signing_key_encoding::appendEd25519PublicKey(encoder, encodedPublicKey.span());
    return SigningPrivateKey{SigningKeyAlgorithm::Ed25519, seed.span(), PublicKey::fromDerOrThrow(encoder.encoded())};
}

auto PrivateKeyParser::parseEcdsa(const Asn1Node &algorithm, const ByteBlock &privateKey) const -> SigningPrivateKey {
    // RFC 5480 section 2.1.1: id-ecPublicKey parameters contain one supported named-curve OID.
    if (algorithm.childCount() != ItemCount{2U}) {
        throwParseError("An EC PKCS#8 algorithm must contain named-curve parameters."_el);
    }
    const auto curveOid = algorithm.child(ItemIndex{1U}).toObjectIdentifier();
    if (!curveOid.has_value()) {
        throwParseError("The EC named-curve parameters are malformed."_el);
    }
    auto curveName = NistPrimeCurve::Name::P256;
    auto keyAlgorithm = SigningKeyAlgorithm::EcdsaP256;
    if (curveOid->toString() == cryptology_oids::secp384r1) {
        curveName = NistPrimeCurve::Name::P384;
        keyAlgorithm = SigningKeyAlgorithm::EcdsaP384;
    } else if (curveOid->toString() != cryptology_oids::secp256r1) {
        throwParseError("Only the prime256v1 and secp384r1 named curves are supported for signing."_el);
    }
    const auto curve = NistPrimeCurve{curveName};

    // RFC 5915 section 3: ECPrivateKey contains version 1, a fixed-width private scalar, and an optional public key.
    const auto root = data::der::DerParser::parseOrThrow(privateKey, cCryptologyAsn1Limits);
    requireNode(root, Asn1UniversalType::Sequence, true, "ECPrivateKey"_el);
    if (root.childCount() < ItemCount{2U} || root.childCount() > ItemCount{3U} ||
        decodeSmallInteger(root.child(ItemIndex{0U}), "ECPrivateKey version"_el) != 1U) {
        throwParseError("ECPrivateKey must contain version 1, privateKey, and optional publicKey."_el);
    }
    const auto scalarNode = root.child(ItemIndex{1U});
    requireNode(scalarNode, Asn1UniversalType::OctetString, false, "EC private scalar"_el);
    auto scalar = scalarNode.contentData();
    scalar.markAsSensitive();
    const auto scalarEraseGuard = SecureEraseGuard{scalar};
    if (!ec_private_scalar_policy::isAcceptable(scalar.span(), curveName)) {
        throwParseError("EC private scalar has an invalid width or is outside the accepted signing-key interval."_el);
    }
    const auto publicPoint = ecdsa_signer::publicKey(scalar.span(), curveName);

    if (root.childCount() == ItemCount{3U}) {
        const auto publicKeyField = root.child(ItemIndex{2U});
        if (!publicKeyField.matches({Asn1TagClass::Context, 1U}, true) ||
            publicKeyField.childCount() != ItemCount{1U}) {
            throwParseError("ECPrivateKey contains unsupported parameters or malformed publicKey."_el);
        }
        const auto point = publicKeyField.child(ItemIndex::zero()).toBitString();
        if (!point || point->unusedBits != 0U || !publicPoint.isEqualConstTime(point->bytes.span())) {
            throwParseError("ECPrivateKey embedded public key does not match its private scalar."_el);
        }
    }

    // RFC 5480 sections 2.1.1 and 2.2: rebuild canonical named-curve SubjectPublicKeyInfo.
    auto encoder = data::der::DerEncoder{};
    signing_key_encoding::appendEcPublicKey(encoder, publicPoint.span(), curveName);
    return SigningPrivateKey{keyAlgorithm, scalar.span(), PublicKey::fromDerOrThrow(encoder.encoded())};
}

auto PrivateKeyParser::parseRsa(const Asn1Node &algorithm, const ByteBlock &privateKey) const -> SigningPrivateKey {
    // RFC 5208 section 6 and RFC 8017 appendix A.1.2: the privateKey OCTET STRING contains one version-0
    // two-prime RSAPrivateKey. The exact algorithm encoding retains RSAE or RSASSA-PSS restrictions.
    auto encoder = data::der::DerEncoder{};
    rsa_signer::appendPublicKey(encoder, privateKey.span(), algorithm.encodedData());
    return SigningPrivateKey{SigningKeyAlgorithm::Rsa, privateKey.span(), PublicKey::fromDerOrThrow(encoder.encoded())};
}

void PrivateKeyParser::requireNode(
    const Asn1Node &node, const Asn1Tag tag, const bool constructed, [[maybe_unused]] const String &name) {
    if (!node.matches(tag, constructed)) {
        throwParseError("PKCS#8 contains an ASN.1 value with an unexpected type or form."_el);
    }
}

auto PrivateKeyParser::decodeSmallInteger(const Asn1Node &node, [[maybe_unused]] const String &name) -> uint32_t {
    const auto value = node.toUnsignedInteger();
    if (!value || *value > UINT32_MAX) {
        throwParseError("PKCS#8 contains an invalid small nonnegative INTEGER."_el);
    }
    return static_cast<uint32_t>(*value);
}

void PrivateKeyParser::throwParseError(const String &reason) {
    throw err::ParseError{reason};
}

}
