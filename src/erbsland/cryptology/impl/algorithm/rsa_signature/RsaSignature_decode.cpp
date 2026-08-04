// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSignature.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ItemIndex.hpp"
#include "../../DerParser.hpp"

#include <bit>
#include <limits>
#include <optional>

namespace erbsland::cryptology::impl::rsa_signature {

using namespace text::literals;
using namespace unit;

auto decodeSignatureParameters(const X509AlgorithmIdentifier &algorithm) -> SignatureParameters {
    // RFC 8017 appendix A.2.4 identifies PKCS#1 v1.5 SHA-256/SHA-384 signatures by distinct OIDs with NULL parameters.
    const auto oid = algorithm.oid().toString();
    if (oid == "1.2.840.113549.1.1.11"_el) {
        requireNullParameters(algorithm.parameters(), "sha256WithRSAEncryption"_el);
        return {Padding::Pkcs1V15, HashAlgorithm::Sha2_256, 0U};
    }
    if (oid == "1.2.840.113549.1.1.12"_el) {
        requireNullParameters(algorithm.parameters(), "sha384WithRSAEncryption"_el);
        return {Padding::Pkcs1V15, HashAlgorithm::Sha2_384, 0U};
    }

    // RFC 4055 section 3.1 and RFC 5756 require explicit RSASSA-PSS-params beside a certificate signature value.
    if (oid == "1.2.840.113549.1.1.10"_el) {
        if (algorithm.parameters().isEmpty()) {
            throwParseError("RSA-PSS signature parameters are absent."_el);
        }
        return decodePssParameters(algorithm.parameters());
    }
    throwParseError("The X.509 signature algorithm is not a supported RSA SHA-2 algorithm."_el);
}

auto decodePssParameters(const Asn1Node &parameters) -> SignatureParameters {
    // RFC 4055 section 3.1 defines RSASSA-PSS-params as an ordered SEQUENCE of explicit context fields [0]--[3].
    requireNode(
        parameters,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Sequence),
        true,
        "RSASSA-PSS-params"_el);

    auto hash = std::optional<HashAlgorithm>{}; // An omitted [0] denotes unsupported SHA-1, not a silent SHA-2 default.
    auto mgfHash = std::optional<HashAlgorithm>{}; // An omitted [1] likewise denotes MGF1 with SHA-1.
    auto saltLength = std::size_t{20U};
    auto trailerField = std::size_t{1U};
    auto previousTag = std::optional<uint32_t>{};
    for (const auto &field : parameters.children()) {
        if (field.tagClass() != Asn1TagClass::Context || !field.isConstructed() || field.tagNumber() > 3U ||
            field.childCount() != ItemCount{1U}) {
            throwParseError("RSASSA-PSS-params contains an invalid field."_el);
        }
        if (previousTag.has_value() && field.tagNumber() <= *previousTag) {
            throwParseError("RSASSA-PSS-params fields are duplicated or out of order."_el);
        }
        previousTag = field.tagNumber();
        const auto value = field.child(ItemIndex::zero());
        switch (field.tagNumber()) {
        case 0U:
            hash = decodeHashAlgorithmIdentifier(value);
            break;
        case 1U: {
            // RFC 4055 section 2.2 permits only id-mgf1 here; its parameters are the complete hash AlgorithmIdentifier.
            requireNode(
                value,
                Asn1TagClass::Universal,
                static_cast<uint32_t>(Asn1UniversalType::Sequence),
                true,
                "maskGenAlgorithm"_el);
            if (value.childCount() != ItemCount{2U}) {
                throwParseError("The RSA-PSS mask-generation AlgorithmIdentifier is malformed."_el);
            }
            const auto mgfOid = value.child(ItemIndex{0U}).toObjectIdentifier();
            if (!mgfOid.has_value() || mgfOid->toString() != "1.2.840.113549.1.1.8"_el) {
                throwParseError("RSA-PSS requires MGF1."_el);
            }
            mgfHash = decodeHashAlgorithmIdentifier(value.child(ItemIndex{1U}));
            break;
        }
        case 2U:
            saltLength = decodeSmallInteger(value, "saltLength"_el);
            break;
        case 3U:
            trailerField = decodeSmallInteger(value, "trailerField"_el);
            break;
        default:
            throwParseError("RSASSA-PSS-params contains an unsupported field."_el);
        }
    }

    // This milestone intentionally rejects RFC defaults to SHA-1 and mixed message/MGF hashes instead of guessing.
    if (!hash.has_value() || !mgfHash.has_value() || *hash != *mgfHash) {
        throwParseError("RSA-PSS requires explicit matching SHA-256 or SHA-384 message and MGF1 hashes."_el);
    }
    if (trailerField != 1U) {
        throwParseError("RSA-PSS supports only the trailer field value 1 (0xbc)."_el);
    }
    // FIPS 186-5 section 5.4 constrains accepted PSS salts to at most the selected hash output length.
    if (saltLength > hash->digestSize().toSizeT()) {
        throwParseError("The RSA-PSS salt length exceeds the selected hash length."_el);
    }
    return {Padding::Pss, *hash, saltLength};
}

auto decodeHashAlgorithmIdentifier(const Asn1Node &node) -> HashAlgorithm {
    // RFC 4055 section 2.1 represents each supported SHA-2 choice as an AlgorithmIdentifier with NULL parameters.
    requireNode(
        node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "HashAlgorithm"_el);
    if (node.childCount() != ItemCount{2U}) {
        throwParseError("An RSA-PSS hash AlgorithmIdentifier must contain an OID and NULL parameters."_el);
    }
    const auto oid = node.child(ItemIndex{0U}).toObjectIdentifier();
    if (!oid.has_value()) {
        throwParseError("An RSA-PSS hash AlgorithmIdentifier contains no object identifier."_el);
    }
    requireNullParameters(node.child(ItemIndex{1U}), "RSA-PSS hash"_el);
    if (oid->toString() == "2.16.840.1.101.3.4.2.1"_el) {
        return HashAlgorithm::Sha2_256;
    }
    if (oid->toString() == "2.16.840.1.101.3.4.2.2"_el) {
        return HashAlgorithm::Sha2_384;
    }
    throwParseError("RSA-PSS uses an unsupported hash algorithm."_el);
}

auto decodePublicKey(const PublicKey &publicKey) -> PublicKeyData {
    auto result = PublicKeyData{};
    const auto keyAlgorithm = publicKey.algorithm();
    const auto keyOid = keyAlgorithm.oid().toString();

    // RFC 3279 section 2.3.1 uses rsaEncryption with mandatory NULL parameters for general-purpose RSA keys.
    if (keyOid == "1.2.840.113549.1.1.1"_el) {
        requireNullParameters(keyAlgorithm.parameters(), "rsaEncryption"_el);
    } else if (keyOid == "1.2.840.113549.1.1.10"_el) {
        // RFC 4055 section 3.1 allows absent id-RSASSA-PSS key parameters; present parameters restrict every use.
        if (!keyAlgorithm.parameters().isEmpty()) {
            result.restrictions = decodePssParameters(keyAlgorithm.parameters());
        }
    } else {
        throwParseError("SubjectPublicKeyInfo does not contain a supported RSA public key."_el);
    }
    if (publicKey.unusedBitCount() != 0U) {
        throwParseError("An RSA subjectPublicKey BIT STRING must contain complete octets."_el);
    }

    // RFC 3279 section 2.3.1 carries one DER RSAPublicKey SEQUENCE inside the subjectPublicKey BIT STRING.
    const auto root = DerParser{publicKey.keyData()}.parseDocument();
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "RSAPublicKey"_el);
    if (root.childCount() != ItemCount{2U}) {
        throwParseError("RSAPublicKey must contain exactly modulus and publicExponent."_el);
    }
    const auto modulus = decodePositiveInteger(root.child(ItemIndex{0U}), "modulus"_el);
    result.exponent = decodePositiveInteger(root.child(ItemIndex{1U}), "publicExponent"_el);

    // FIPS 186-5 sections 5.1 and 5.4 require modern RSA moduli of at least 2048 bits; 8192 bounds verification work.
    const auto firstModulusByte = modulus.span().front().toUInt8();
    result.modulusBits = (modulus.span().size() - 1U) * 8U + static_cast<std::size_t>(std::bit_width(firstModulusByte));
    if (result.modulusBits < cMinimumModulusBits || result.modulusBits > cMaximumModulusBits ||
        (modulus.span().back().toUInt8() & 1U) == 0U) {
        throwParseError("The RSA modulus must be odd and between 2048 and 8192 bits."_el);
    }
    result.encodedLength = (result.modulusBits + 7U) / 8U;
    const auto wordCount = (result.modulusBits + cWordBits - 1U) / cWordBits;
    result.modulus = numberFromBigEndian(modulus.span(), wordCount);

    // FIPS 186-5 section 5.1 requires an odd public exponent strictly above 2^16 and below 2^256.
    if (result.exponent.span().size() > cMaximumExponentBytes || (result.exponent.span().back().toUInt8() & 1U) == 0U) {
        throwParseError("The RSA public exponent is outside the fixed 17--256-bit odd bound."_el);
    }
    const auto exponentIsLargeEnough = result.exponent.span().size() > 3U ||
        (result.exponent.span().size() == 3U &&
            (result.exponent.span()[0U].toUInt8() > 1U || result.exponent.span()[1U].toUInt8() != 0U ||
                result.exponent.span()[2U].toUInt8() != 0U));
    if (!exponentIsLargeEnough) {
        throwParseError("The RSA public exponent must be greater than 65536."_el);
    }
    return result;
}

auto decodeSmallInteger(const Asn1Node &node, const text::String &name) -> std::size_t {
    // RFC 4055 section 3.1 permits a nonnegative saltLength, including zero; trailerField is checked separately.
    requireNode(node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Integer), false, name);
    auto magnitude = node.contentData();
    if (magnitude.isEmpty() || (magnitude.span().front().toUInt8() & 0x80U) != 0U) {
        throwParseError("An RSA-PSS INTEGER must be nonnegative."_el);
    }
    if (magnitude.span().front().toUInt8() == 0U) {
        if (magnitude.length() == ByteLength::one()) {
            return 0U;
        }
        magnitude = magnitude.slice(ByteIndex{1U}, magnitude.length() - ByteLength::one());
    }
    if (magnitude.span().size() > sizeof(std::size_t)) {
        throwParseError("An RSA-PSS integer exceeds the implementation bound."_el);
    }
    auto result = std::size_t{};
    for (const auto byte : magnitude.span()) {
        if (result > (std::numeric_limits<std::size_t>::max() - byte.toUInt8()) / 256U) {
            throwParseError("An RSA-PSS integer exceeds the implementation bound."_el);
        }
        result = result * 256U + byte.toUInt8();
    }
    return result;
}

auto decodePositiveInteger(const Asn1Node &node, [[maybe_unused]] const text::String &name) -> mem::ByteBlock {
    requireNode(node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Integer), false, name);
    auto contents = node.contentData();
    if (contents.isEmpty() || (contents.span().front().toUInt8() & 0x80U) != 0U) {
        throwParseError("An RSA INTEGER must be positive."_el);
    }
    if (contents.span().front().toUInt8() == 0U) {
        if (contents.length() == ByteLength::one()) {
            throwParseError("An RSA INTEGER must be greater than zero."_el);
        }
        contents = contents.slice(ByteIndex{1U}, contents.length() - ByteLength::one());
    }
    return contents;
}

void requireNode(
    const Asn1Node &node,
    const Asn1TagClass tagClass,
    const uint32_t tagNumber,
    const bool constructed,
    [[maybe_unused]] const text::String &name) {
    if (node.isEmpty() || node.tagClass() != tagClass || node.tagNumber() != tagNumber ||
        node.isConstructed() != constructed) {
        throwParseError("An RSA ASN.1 value has an unexpected type or form."_el);
    }
}

void requireNullParameters(const Asn1Node &parameters, [[maybe_unused]] const text::String &name) {
    requireNode(parameters, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Null), false, name);
    if (!parameters.contentData().isEmpty()) {
        throwParseError("An RSA AlgorithmIdentifier NULL parameter contains data."_el);
    }
}

void throwParseError(const text::String &reason) {
    throw err::ParseError{reason};
}

}
