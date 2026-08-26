// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PrivateKeyParser.hpp"

#include "DerParser.hpp"
#include "SecureEraseGuard.hpp"

#include "algorithm/ecdsa_signature/EcdsaSigner.hpp"
#include "algorithm/ed25519_signature/Ed25519Signer.hpp"
#include "algorithm/rsa_signature/RsaSigner.hpp"

#include "../asn1/Asn1UniversalType.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/AsciiCategory.hpp"
#include "../../text/base_n/BaseNDecoder.hpp"
#include "../../text/base_n/BaseNFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;
using namespace unit;

auto PrivateKeyParser::parse() const -> SigningPrivateKey {
    // RFC 5208 section 6: PrivateKeyInfo is exactly version, privateKeyAlgorithm, and privateKey. Attributes are
    // intentionally rejected in the initial server-identity profile so no unreviewed metadata survives import.
    const auto root = DerParser{_der}.parseDocument();
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "PrivateKeyInfo"_el);
    if (root.childCount() != ItemCount{3U}) {
        throwParseError("PKCS#8 PrivateKeyInfo must contain exactly version, algorithm, and private key."_el);
    }
    if (decodeSmallInteger(root.child(ItemIndex{0U}), "version"_el) != 0U) {
        throwParseError("PKCS#8 PrivateKeyInfo version must be zero."_el);
    }
    const auto algorithm = root.child(ItemIndex{1U});
    requireNode(
        algorithm,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Sequence),
        true,
        "privateKeyAlgorithm"_el);
    if (algorithm.childCount() < ItemCount{1U} || algorithm.childCount() > ItemCount{2U}) {
        throwParseError("PKCS#8 privateKeyAlgorithm must contain an OID and optional parameters."_el);
    }
    const auto oid = algorithm.child(ItemIndex{0U}).toObjectIdentifier();
    if (!oid.has_value()) {
        throwParseError("PKCS#8 privateKeyAlgorithm contains no object identifier."_el);
    }
    const auto privateKeyNode = root.child(ItemIndex{2U});
    requireNode(
        privateKeyNode,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::OctetString),
        false,
        "privateKey"_el);
    auto privateKey = privateKeyNode.contentData();
    privateKey.markAsSensitive();
    const auto privateKeyEraseGuard = SecureEraseGuard{privateKey};
    if (oid->toString() == "1.3.101.112"_el) {
        return parseEd25519(algorithm, privateKey);
    }
    if (oid->toString() == "1.2.840.10045.2.1"_el) {
        return parseEcdsaP256(algorithm, privateKey);
    }
    if (oid->toString() == "1.2.840.113549.1.1.1"_el || oid->toString() == "1.2.840.113549.1.1.10"_el) {
        return parseRsa(algorithm, privateKey);
    }
    throwParseError("PKCS#8 contains an unsupported private-key algorithm."_el);
}

auto PrivateKeyParser::decodePem(const text::String &pem) -> mem::ByteBlock {
    static constexpr auto cMaximumPemLength = std::size_t{2U * 1024U * 1024U};
    if (pem.length().toSizeTOrThrow() > cMaximumPemLength) {
        throw err::OutOfRangeError{"Private-key PEM exceeds the fixed two-MiB source limit."_el};
    }
    auto reader = text::StringCharReader{pem};
    reader.advanceWhile(text::AsciiCategory::Whitespace);
    if (!reader.advanceIf("-----BEGIN PRIVATE KEY-----"_el) ||
        reader.advanceWhile(text::AsciiCategory::Whitespace).isZero()) {
        throwParseError("Expected an exact PRIVATE KEY PEM pre-encapsulation boundary."_el);
    }
    auto base64 = text::StringEditor{};
    while (!reader.isAtEnd() && reader.peek() != U'-') {
        const auto character = reader.read();
        if (character.isAsciiWhitespace()) {
            continue;
        }
        if (!character.isAsciiCategory(text::AsciiCategory::Base64Text)) {
            throwParseError("Private-key PEM contains a non-Base64 character."_el);
        }
        base64.append(character);
    }
    if (base64.isEmpty() || !reader.advanceIf("-----END PRIVATE KEY-----"_el)) {
        throwParseError("Private-key PEM has empty data or no exact post-encapsulation boundary."_el);
    }
    reader.advanceWhile(text::AsciiCategory::Whitespace);
    if (!reader.isAtEnd()) {
        throwParseError("Private-key PEM contains trailing data or another block."_el);
    }
    auto format = text::base_n::BaseNFormat::base64();
    format.setWhitespace({});
    auto result = text::base_n::BaseNDecoder{text::String{base64}, format}.toDataOrThrow(DerParser::cMaximumLength);
    result.markAsSensitive();
    return result;
}

auto PrivateKeyParser::parseEd25519(const Asn1Node &algorithm, const mem::ByteBlock &privateKey) const
    -> SigningPrivateKey {
    // RFC 8410 sections 3 and 7: id-Ed25519 parameters are absent and CurvePrivateKey is an inner OCTET STRING.
    if (algorithm.childCount() != ItemCount{1U}) {
        throwParseError("Ed25519 PKCS#8 algorithm parameters must be absent."_el);
    }
    const auto inner = DerParser{privateKey}.parseDocument();
    requireNode(
        inner,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::OctetString),
        false,
        "CurvePrivateKey"_el);
    auto seed = inner.contentData();
    seed.markAsSensitive();
    const auto seedEraseGuard = SecureEraseGuard{seed};
    if (seed.length() != ByteLength{32U}) {
        throwParseError("Ed25519 CurvePrivateKey must contain exactly 32 octets."_el);
    }

    // RFC 8410 section 4: SubjectPublicKeyInfo is SEQUENCE { id-Ed25519, BIT STRING ENC(A) }.
    const auto encodedPublicKey = ed25519_signer::publicKey(seed.span());
    auto spki = mem::ByteBlockEditor{
        mem::Byte{0x30U},
        mem::Byte{0x2aU},
        mem::Byte{0x30U},
        mem::Byte{0x05U},
        mem::Byte{0x06U},
        mem::Byte{0x03U},
        mem::Byte{0x2bU},
        mem::Byte{0x65U},
        mem::Byte{0x70U},
        mem::Byte{0x03U},
        mem::Byte{0x21U},
        mem::Byte{0x00U}};
    spki.append(encodedPublicKey.span());
    return SigningPrivateKey{
        SigningKeyAlgorithm::Ed25519, seed.span(), PublicKey::fromDerOrThrow(mem::ByteBlock{spki})};
}

auto PrivateKeyParser::parseEcdsaP256(const Asn1Node &algorithm, const mem::ByteBlock &privateKey) const
    -> SigningPrivateKey {
    // RFC 5480 section 2.1.1: id-ecPublicKey parameters contain the named-curve OID prime256v1.
    if (algorithm.childCount() != ItemCount{2U}) {
        throwParseError("A P-256 PKCS#8 algorithm must contain named-curve parameters."_el);
    }
    const auto curveOid = algorithm.child(ItemIndex{1U}).toObjectIdentifier();
    if (!curveOid.has_value() || curveOid->toString() != "1.2.840.10045.3.1.7"_el) {
        throwParseError("Only the prime256v1 named curve is supported for signing."_el);
    }

    // RFC 5915 section 3: ECPrivateKey contains version 1, a fixed-width private scalar, and an optional public key.
    const auto root = DerParser{privateKey}.parseDocument();
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "ECPrivateKey"_el);
    if (root.childCount() < ItemCount{2U} || root.childCount() > ItemCount{3U} ||
        decodeSmallInteger(root.child(ItemIndex{0U}), "ECPrivateKey version"_el) != 1U) {
        throwParseError("P-256 ECPrivateKey must contain version 1, privateKey, and optional publicKey."_el);
    }
    const auto scalarNode = root.child(ItemIndex{1U});
    requireNode(
        scalarNode,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::OctetString),
        false,
        "EC private scalar"_el);
    auto scalar = scalarNode.contentData();
    scalar.markAsSensitive();
    const auto scalarEraseGuard = SecureEraseGuard{scalar};
    const auto publicPoint = ecdsa_signer::publicKey(scalar.span());

    if (root.childCount() == ItemCount{3U}) {
        const auto publicKeyField = root.child(ItemIndex{2U});
        if (publicKeyField.tagClass() != Asn1TagClass::Context || publicKeyField.tagNumber() != 1U ||
            !publicKeyField.isConstructed() || publicKeyField.childCount() != ItemCount{1U}) {
            throwParseError("P-256 ECPrivateKey contains unsupported parameters or malformed publicKey."_el);
        }
        const auto bitString = publicKeyField.child(ItemIndex{0U});
        requireNode(
            bitString,
            Asn1TagClass::Universal,
            static_cast<uint32_t>(Asn1UniversalType::BitString),
            false,
            "EC public key"_el);
        const auto content = bitString.contentData();
        if (content.length() != ByteLength{66U} || content.span().front().toUInt8() != 0U ||
            !publicPoint.isEqualConstTime(content.span().subspan(1U))) {
            throwParseError("P-256 ECPrivateKey embedded public key does not match its private scalar."_el);
        }
    }

    // RFC 5480 sections 2.1.1 and 2.2: build canonical id-ecPublicKey/prime256v1 SubjectPublicKeyInfo.
    auto spki = mem::ByteBlockEditor{
        mem::Byte{0x30U},
        mem::Byte{0x59U},
        mem::Byte{0x30U},
        mem::Byte{0x13U},
        mem::Byte{0x06U},
        mem::Byte{0x07U},
        mem::Byte{0x2aU},
        mem::Byte{0x86U},
        mem::Byte{0x48U},
        mem::Byte{0xceU},
        mem::Byte{0x3dU},
        mem::Byte{0x02U},
        mem::Byte{0x01U},
        mem::Byte{0x06U},
        mem::Byte{0x08U},
        mem::Byte{0x2aU},
        mem::Byte{0x86U},
        mem::Byte{0x48U},
        mem::Byte{0xceU},
        mem::Byte{0x3dU},
        mem::Byte{0x03U},
        mem::Byte{0x01U},
        mem::Byte{0x07U},
        mem::Byte{0x03U},
        mem::Byte{0x42U},
        mem::Byte{0x00U}};
    spki.append(publicPoint);
    return SigningPrivateKey{
        SigningKeyAlgorithm::EcdsaP256, scalar.span(), PublicKey::fromDerOrThrow(mem::ByteBlock{spki})};
}

auto PrivateKeyParser::parseRsa(const Asn1Node &algorithm, const mem::ByteBlock &privateKey) const
    -> SigningPrivateKey {
    // RFC 5208 section 6 and RFC 8017 appendix A.1.2: the privateKey OCTET STRING contains one version-0
    // two-prime RSAPrivateKey. The exact algorithm encoding retains RSAE or RSASSA-PSS restrictions.
    const auto publicKeyDer = rsa_signer::publicKey(privateKey.span(), algorithm.encodedData().span());
    return SigningPrivateKey{SigningKeyAlgorithm::Rsa, privateKey.span(), PublicKey::fromDerOrThrow(publicKeyDer)};
}

void PrivateKeyParser::requireNode(
    const Asn1Node &node,
    const Asn1TagClass tagClass,
    const uint32_t tagNumber,
    const bool constructed,
    [[maybe_unused]] const text::String &name) {
    if (node.isEmpty() || node.tagClass() != tagClass || node.tagNumber() != tagNumber ||
        node.isConstructed() != constructed) {
        throwParseError("PKCS#8 contains an ASN.1 value with an unexpected type or form."_el);
    }
}

auto PrivateKeyParser::decodeSmallInteger(const Asn1Node &node, [[maybe_unused]] const text::String &name) -> uint32_t {
    requireNode(node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Integer), false, name);
    const auto content = node.contentData();
    if (content.isEmpty() || content.length() > ByteLength{4U} || (content.span().front().toUInt8() & 0x80U) != 0U) {
        throwParseError("PKCS#8 contains an invalid small nonnegative INTEGER."_el);
    }
    auto result = uint32_t{};
    for (const auto byte : content.span()) {
        result = (result << 8U) | byte.toUInt32();
    }
    return result;
}

void PrivateKeyParser::throwParseError(const text::String &reason) {
    throw err::ParseError{reason};
}

}
