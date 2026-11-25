// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509Parser.hpp"

#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../network/IpVersion.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringSet.hpp"
#include "../../time/Date.hpp"
#include "../../time/Time.hpp"
#include "../../time/Timestamp.hpp"

#include <limits>
#include <set>
#include <utility>

namespace erbsland::cryptology::impl {

using data::asn1::Asn1Node;
using data::asn1::Asn1ObjectIdentifier;
using data::asn1::Asn1Tag;
using data::asn1::Asn1TagClass;
using data::asn1::Asn1UniversalType;
using namespace text;
using namespace text::literals;
using namespace unit;
using namespace mem;

void X509Parser::parseKnownExtension(const X509Extension &extension) {
    const auto &oid = extension.oid().toString();
    const auto known = oid == "2.5.29.14"_el || oid == "2.5.29.15"_el || oid == "2.5.29.17"_el ||
        oid == "2.5.29.19"_el || oid == "2.5.29.35"_el || oid == "2.5.29.37"_el;
    if (!known) {
        return;
    }
    try {
        if (oid == "2.5.29.14"_el) {
            parseSubjectKeyIdentifier(extension);
        } else if (oid == "2.5.29.15"_el) {
            parseKeyUsage(extension);
        } else if (oid == "2.5.29.17"_el) {
            parseSubjectAlternativeNames(extension);
        } else if (oid == "2.5.29.19"_el) {
            parseBasicConstraints(extension);
        } else if (oid == "2.5.29.35"_el) {
            parseAuthorityKeyIdentifier(extension);
        } else if (oid == "2.5.29.37"_el) {
            parseExtendedKeyUsage(extension);
        }
    } catch (const err::ParseError &) {
        if (_mode == X509CertificateProfileMode::Strict) {
            throw;
        }
        addIssue(
            X509CertificateProfileIssueCategory::ExtensionValue,
            "extensions"_el,
            extension.oid(),
            "Recognized X.509 extension contains a malformed value."_el);
    }
}

void X509Parser::parseSubjectAlternativeNames(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.6: subjectAltName is a nonempty GeneralNames sequence.
    const auto root = extension.asn1();
    requireNode(root, Asn1UniversalType::Sequence, true, "subjectAltName"_el);
    if (root.childCount() == ItemCount{}) {
        throwParseError("subjectAltName must not be empty."_el, root);
    }
    for (const auto &node : root.children()) {
        _values.subjectAlternativeNames.append(parseGeneralName(node));
    }
}

auto X509Parser::parseGeneralName(const Asn1Node &node) -> X509GeneralName {
    // RFC 5280 section 4.2.1.6 and GeneralName ASN.1: alternatives use implicit context-specific tags.
    if (node.tagClass() != Asn1TagClass::Context) {
        throwParseError("GeneralName must use a context-specific tag."_el, node);
    }
    const auto tag = node.tagNumber();
    if (tag > 8U) {
        throwParseError("GeneralName uses an unknown context-specific tag."_el, node);
    }
    if (tag == 1U || tag == 2U || tag == 6U) {
        if (node.isConstructed()) {
            throwParseError("IA5 GeneralName must use primitive encoding."_el, node);
        }
        const auto content = node.contentData();
        if (content.isEmpty()) {
            throwParseError("IA5 GeneralName must not be empty."_el, node);
        }
        auto value = StringEditor{};
        for (const auto byte : content.span()) {
            if (byte.toUInt8() > 0x7FU) {
                throwParseError("IA5 GeneralName contains non-ASCII data."_el, node);
            }
            value.append(Char{byte.toUInt32()});
        }
        const auto kind = tag == 1U ? X509GeneralName::Kind::Email
                                    : (tag == 2U ? X509GeneralName::Kind::Dns : X509GeneralName::Kind::Uri);
        return X509GeneralName{kind, value, {}, {}, {}, node};
    }
    if (tag == 4U) {
        if (!node.isConstructed() || node.childCount() != ItemCount{1U}) {
            throwParseError("directoryName GeneralName must explicitly contain one Name."_el, node);
        }
        return X509GeneralName{
            X509GeneralName::Kind::Directory, {}, {}, parseName(node.child(ItemIndex{0U})), {}, node};
    }
    if (tag == 7U) {
        const auto content = node.contentData();
        const auto bytes = content.span();
        if (node.isConstructed() || (bytes.size() != 4U && bytes.size() != 16U)) {
            throwParseError("iPAddress GeneralName must contain four or sixteen octets."_el, node);
        }
        auto addressBytes = network::IpAddress::Bytes{};
        if (bytes.size() == 4U) {
            addressBytes.setIntegerOrThrow(
                ByteIndex{0U},
                content.getIntegerOrThrow<uint32_t>(ByteIndex::zero(), Endianness::Big),
                Endianness::Big);
        } else {
            for (auto part = std::size_t{}; part < 2U; ++part) {
                const auto partIndex = ByteIndex{part * 8U, ByteIndex::Unchecked{}};
                addressBytes.setIntegerOrThrow(
                    partIndex, content.getIntegerOrThrow<uint64_t>(partIndex, Endianness::Big), Endianness::Big);
            }
        }
        const auto address = network::IpAddress::fromBytes(
            bytes.size() == 4U ? network::IpVersion::V4 : network::IpVersion::V6, addressBytes);
        return X509GeneralName{X509GeneralName::Kind::IpAddress, {}, address, {}, {}, node};
    }
    if (tag == 8U) {
        if (node.isConstructed()) {
            throwParseError("registeredID GeneralName must use primitive encoding."_el, node);
        }
        return X509GeneralName{
            X509GeneralName::Kind::RegisteredId,
            {},
            {},
            {},
            node.toObjectIdentifierOrThrow({Asn1TagClass::Context, 8U}),
            node};
    }
    if (!node.isConstructed()) {
        throwParseError("Structured GeneralName alternative must use constructed encoding."_el, node);
    }
    if (tag == 0U) {
        if (node.childCount() != ItemCount{2U} || !node.child(ItemIndex{0U}).toObjectIdentifier().has_value() ||
            !node.child(ItemIndex{1U}).matches({Asn1TagClass::Context, 0U}, true)) {
            throwParseError("otherName GeneralName is malformed."_el, node);
        }
    }
    return X509GeneralName{X509GeneralName::Kind::Unsupported, {}, {}, {}, {}, node};
}

void X509Parser::parseSubjectKeyIdentifier(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.2: SubjectKeyIdentifier is an OCTET STRING.
    const auto root = extension.asn1();
    requireNode(root, Asn1UniversalType::OctetString, false, "subjectKeyIdentifier"_el);
    _values.subjectKeyIdentifier = root.contentData();
}

void X509Parser::parseAuthorityKeyIdentifier(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.1: ordered implicit keyIdentifier, authorityCertIssuer, and serial fields.
    const auto root = extension.asn1();
    requireNode(root, Asn1UniversalType::Sequence, true, "authorityKeyIdentifier"_el);
    auto previousTag = std::optional<uint32_t>{};
    auto hasIssuer = false;
    auto hasSerialNumber = false;
    for (const auto &child : root.children()) {
        if (child.tagClass() != Asn1TagClass::Context || child.tagNumber() > 2U ||
            (previousTag.has_value() && child.tagNumber() <= *previousTag)) {
            throwParseError("Authority Key Identifier fields are unknown, duplicated, or out of order."_el, child);
        }
        previousTag = child.tagNumber();
        if (child.matches({Asn1TagClass::Context, 0U}, false)) {
            _values.authorityKeyIdentifier = child.contentData();
        } else if (child.matches({Asn1TagClass::Context, 1U}, true)) {
            if (child.childCount() == ItemCount{}) {
                throwParseError("Authority Key Identifier issuer GeneralNames must not be empty."_el, child);
            }
            for (const auto &name : child.children()) {
                // The issuer names are not exposed, but fully decoding each value validates the extension syntax.
                [[maybe_unused]] const auto validatedIssuerName = parseGeneralName(name);
            }
            hasIssuer = true;
        } else if (child.matches({Asn1TagClass::Context, 2U}, false)) {
            validateImplicitSerialNumber(child, "authorityCertSerialNumber"_el);
            hasSerialNumber = true;
        } else {
            throwParseError("Authority Key Identifier field has an invalid ASN.1 form."_el, child);
        }
    }
    if (hasIssuer != hasSerialNumber) {
        throwParseError("Authority Key Identifier issuer and serial number must occur together."_el, root);
    }
}

void X509Parser::parseBasicConstraints(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.9: cA defaults to FALSE and pathLenConstraint requires cA TRUE.
    const auto root = extension.asn1();
    requireNode(root, Asn1UniversalType::Sequence, true, "basicConstraints"_el);
    auto index = ItemIndex::zero();
    auto certificateAuthority = false;
    auto pathLength = std::optional<uint32_t>{};
    if (ItemIndex::end(root.childCount()) > index && root.child(index).universalType() == Asn1UniversalType::Boolean) {
        certificateAuthority = *root.child(index++).toBoolean();
        if (!certificateAuthority) {
            throwParseError("Basic Constraints must omit its default false cA value."_el, root);
        }
    }
    if (ItemIndex::end(root.childCount()) > index) {
        pathLength = static_cast<uint32_t>(unsignedInteger(root.child(index++), std::numeric_limits<uint32_t>::max()));
        if (!certificateAuthority) {
            throwParseError("Basic Constraints pathLenConstraint requires cA true."_el, root);
        }
    }
    if (index != ItemIndex::end(root.childCount())) {
        throwParseError("Basic Constraints contains unexpected fields."_el, root);
    }
    _values.basicConstraints = X509BasicConstraints{certificateAuthority, pathLength};
}

void X509Parser::parseKeyUsage(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.3 and X.690 section 11.2: KeyUsage is a canonical named BIT STRING.
    const auto root = extension.asn1();
    const auto bits = root.toBitString();
    if (!bits) {
        throwParseError("Expected a Key Usage BIT STRING."_el, root);
    }
    const auto content = bits->bytes.span();
    const auto bitCount = content.size() * 8U - bits->unusedBits;
    if (bitCount == 0U || bitCount > 9U || content.back().toUInt8() == 0U) {
        throwParseError("Key Usage has no bits or contains bits outside its named range."_el, root);
    }
    auto canonicalUnusedBitCount = std::size_t{};
    auto lastByte = content.back().toUInt8();
    while ((lastByte & 1U) == 0U) {
        ++canonicalUnusedBitCount;
        lastByte >>= 1U;
    }
    if (canonicalUnusedBitCount != bits->unusedBits) {
        throwParseError("Key Usage does not use canonical named-bit-list encoding."_el, root);
    }
    auto usages = X509KeyUsages{};
    for (auto bit = std::size_t{}; bit < bitCount; ++bit) {
        if ((content[bit / 8U].toUInt8() & (0x80U >> (bit % 8U))) != 0U) {
            usages |= static_cast<X509KeyUsage>(1U << bit);
        }
    }
    _values.keyUsage = usages;
}

void X509Parser::parseExtendedKeyUsage(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.12: ExtendedKeyUsage is a nonempty sequence of unique purpose OIDs.
    const auto root = extension.asn1();
    requireNode(root, Asn1UniversalType::Sequence, true, "extendedKeyUsage"_el);
    if (root.childCount() == ItemCount{}) {
        throwParseError("Extended Key Usage must not be empty."_el, root);
    }
    auto seen = StringSet{};
    for (const auto &child : root.children()) {
        const auto oid = child.toObjectIdentifier();
        if (!oid.has_value()) {
            throwParseError("Extended Key Usage contains a non-OID value."_el, child);
        }
        if (!seen.tryInsert(oid->toString())) {
            throwParseError("Extended Key Usage contains a duplicate OID."_el, child);
        }
        _values.extendedKeyUsage.append(*oid);
    }
}

void X509Parser::validateTbsProfile(
    const bool hasIssuerUniqueId, const bool hasSubjectUniqueId, const bool hasExtensions) {
    if (_values.version == X509Version::V1 && (hasIssuerUniqueId || hasSubjectUniqueId || hasExtensions)) {
        addIssue(
            X509CertificateProfileIssueCategory::VersionField,
            "version"_el,
            {},
            "Version 1 certificate contains fields introduced in a later version."_el);
    }
    if (_values.version == X509Version::V2 && hasExtensions) {
        addIssue(
            X509CertificateProfileIssueCategory::VersionField,
            "version"_el,
            {},
            "Version 2 certificate contains version 3 extensions."_el);
    }
    if (_values.issuer.isEmpty()) {
        addIssue(
            X509CertificateProfileIssueCategory::NameValue,
            "issuer"_el,
            {},
            "Certificate issuer name must not be empty."_el);
    }
    if (_values.subject.isEmpty()) {
        auto hasCriticalSubjectAlternativeName = false;
        for (const auto &extension : _values.extensions) {
            if (extension.oid().toString() == "2.5.29.17"_el && extension.isCritical()) {
                hasCriticalSubjectAlternativeName = true;
                break;
            }
        }
        if (!hasCriticalSubjectAlternativeName) {
            addIssue(
                X509CertificateProfileIssueCategory::NameValue,
                "subject"_el,
                Asn1ObjectIdentifier::fromStringOrThrow("2.5.29.17"_el),
                "An empty subject requires a critical Subject Alternative Name extension."_el);
        }
    }
}

void X509Parser::addIssue(
    const X509CertificateProfileIssueCategory category, String field, Asn1ObjectIdentifier oid, String diagnostic) {
    if (_mode == X509CertificateProfileMode::Strict) {
        throwParseError(std::move(diagnostic), _values.root);
    }
    _values.profileIssues.append(
        X509CertificateProfileIssue{category, std::move(field), std::move(oid), std::move(diagnostic)});
}

void X509Parser::validateImplicitBitString(const Asn1Node &node, const String &name) const {
    if (!node.toBitString(node.tag())) {
        throwParseError(fieldDiagnostic(name, " contains an invalid BIT STRING."_el), node);
    }
}

void X509Parser::validateImplicitSerialNumber(const Asn1Node &node, const String &name) const {
    const auto magnitude = node.toUnsignedMagnitude(node.tag());
    if (!magnitude || magnitude->isEmpty() || magnitude->length() > ByteCount{20U}) {
        throwParseError(fieldDiagnostic(name, " must be positive and at most 20 octets."_el), node);
    }
}

auto X509Parser::unsignedInteger(const Asn1Node &node, const uint64_t maximum) const -> uint64_t {
    const auto value = node.toUnsignedInteger();
    if (!value || *value > maximum) {
        throwParseError("Expected a nonnegative INTEGER in the supported range."_el, node);
    }
    return *value;
}

void X509Parser::requireNode(
    const Asn1Node &node, const Asn1Tag tag, const bool constructed, const String &name) const {
    if (!node.matches(tag, constructed)) {
        auto message = StringEditor{};
        message.append("Unexpected ASN.1 node for "_el);
        message.append(name);
        message.append(U'.');
        throwParseError(message, node);
    }
}

auto X509Parser::fieldDiagnostic(const String &name, const String &detail) -> String {
    auto result = StringEditor{};
    result.append(name);
    result.append(detail);
    return result;
}

auto X509Parser::decimalDigit(const ConstByteSpan bytes, const std::size_t index, const ByteIndex contentByteIndex)
    -> uint32_t {
    if (index >= bytes.size() || bytes[index].toUInt8() < '0' || bytes[index].toUInt8() > '9') {
        throw err::ParseError{
            "X.509 time contains a nondigit component."_el, contentByteIndex + ByteCount::fromSizeT(index)};
    }
    return bytes[index].toUInt32() - static_cast<uint32_t>('0');
}

auto X509Parser::decimalPair(const ConstByteSpan bytes, const std::size_t index, const ByteIndex contentByteIndex)
    -> uint32_t {
    return decimalDigit(bytes, index, contentByteIndex) * 10U + decimalDigit(bytes, index + 1U, contentByteIndex);
}

auto X509Parser::decimalQuad(const ConstByteSpan bytes, const std::size_t index, const ByteIndex contentByteIndex)
    -> uint32_t {
    return decimalPair(bytes, index, contentByteIndex) * 100U + decimalPair(bytes, index + 2U, contentByteIndex);
}

void X509Parser::throwParseError(String reason, const Asn1Node &node) const {
    throw err::ParseError{std::move(reason), node.encodedByteIndex()};
}

}
