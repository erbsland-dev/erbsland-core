// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509Parser.hpp"

#include "Asn1ObjectIdentifierCodec.hpp"
#include "DerParser.hpp"

#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteReader.hpp"
#include "../../network/IpVersion.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"
#include "../../time/Date.hpp"
#include "../../time/DateTime.hpp"
#include "../../time/Time.hpp"

#include <limits>
#include <set>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace text;
using namespace text::literals;
using namespace unit;
using namespace mem;

X509Parser::X509Parser(Asn1Node root, const X509CertificateProfileMode mode) : _root{std::move(root)}, _mode{mode} {
}

auto X509Parser::parse(const ByteBlock &der, const X509CertificateProfileMode mode) -> X509Certificate {
    return X509Parser{DerParser{der}.parseDocument(), mode}.parseCertificate();
}

auto X509Parser::parsePublicKey(const ByteBlock &der) -> PublicKey {
    auto parser = X509Parser{DerParser{der}.parseDocument(), X509CertificateProfileMode::Strict};
    return parser.parseSubjectPublicKeyInfo(parser._root);
}

auto X509Parser::parseAlgorithmIdentifier(const ByteBlock &der) -> X509AlgorithmIdentifier {
    auto parser = X509Parser{DerParser{der}.parseDocument(), X509CertificateProfileMode::Strict};
    return parser.parseAlgorithmIdentifierNode(parser._root);
}

auto X509Parser::parseCertificate() -> X509Certificate {
    // RFC 5280 section 4.1: Certificate ::= SEQUENCE { tbsCertificate, signatureAlgorithm, signatureValue }.
    const auto root = _root;
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "Certificate"_el);
    if (root.childCount() != ItemCount{3U}) {
        throwParseError(
            "Certificate must contain exactly TBSCertificate, AlgorithmIdentifier, and signature."_el, root);
    }
    _values.root = root;
    parseTbsCertificate(root.child(ItemIndex{0U}));
    _values.signatureAlgorithm = parseAlgorithmIdentifierNode(root.child(ItemIndex{1U}));
    const auto signatureNode = root.child(ItemIndex{2U});
    requireNode(
        signatureNode,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::BitString),
        false,
        "signatureValue"_el);
    const auto signatureContent = signatureNode.contentData();
    _values.signatureUnusedBitCount = signatureContent.span().front().toUInt8();
    _values.signature = signatureContent.slice(ByteIndex{1U}, signatureContent.length() - ByteLength{1U});
    if (_values.signatureUnusedBitCount != 0U) {
        throwParseError("An X.509 signature BIT STRING must contain complete octets."_el, signatureNode);
    }
    if (_values.tbsSignatureAlgorithm.toDer() != _values.signatureAlgorithm.toDer()) {
        addIssue(
            X509CertificateProfileIssueCategory::SignatureAlgorithmMismatch,
            "signatureAlgorithm"_el,
            _values.signatureAlgorithm.oid(),
            "TBSCertificate and outer signature algorithm identifiers differ."_el);
    }
    return X509Certificate{X509CertificateDataPtr{new PortableX509CertificateData{std::move(_values)}}};
}

void X509Parser::parseTbsCertificate(const Asn1Node &node) {
    // RFC 5280 sections 4.1.2.1-4.1.2.9 define the ordered TBSCertificate fields parsed below.
    requireNode(
        node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "TBSCertificate"_el);
    _values.tbsNode = node;
    const auto children = node.children();
    auto index = std::size_t{};
    if (!children.isEmpty() && isNode(children.toRawValue()[0U], Asn1TagClass::Context, 0U, true)) {
        const auto explicitVersion = children.toRawValue()[index++];
        if (explicitVersion.childCount() != ItemCount{1U}) {
            throwParseError("X.509 version field must contain one INTEGER."_el, explicitVersion);
        }
        const auto version = unsignedInteger(explicitVersion.child(ItemIndex{0U}), 255U);
        if (version == 0U) {
            _values.version = X509Version::V1;
            addIssue(
                X509CertificateProfileIssueCategory::VersionField,
                "version"_el,
                {},
                "Certificate explicitly encodes the default version 1 value."_el);
        } else if (version == 1U) {
            _values.version = X509Version::V2;
        } else if (version == 2U) {
            _values.version = X509Version::V3;
        } else {
            _values.version = X509Version::Unknown;
            addIssue(
                X509CertificateProfileIssueCategory::VersionField,
                "version"_el,
                {},
                "Certificate contains an unknown X.509 version value."_el);
        }
    } else {
        _values.version = X509Version::V1;
    }
    if (children.count().toSizeT() < index + 6U) {
        throwParseError("TBSCertificate is missing required fields."_el, node);
    }
    const auto serialNode = children.toRawValue()[index++];
    requireNode(
        serialNode,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Integer),
        false,
        "serialNumber"_el);
    _values.serialNumber = serialNode.contentData();
    const auto serial = _values.serialNumber.span();
    const auto negative = (serial.front().toUInt8() & 0x80U) != 0U;
    auto magnitudeLength = serial.size();
    if (!serial.empty() && serial.front().toUInt8() == 0U) {
        --magnitudeLength;
    }
    auto zero = true;
    for (const auto byte : serial) {
        zero = zero && byte.toUInt8() == 0U;
    }
    if (negative || zero || magnitudeLength > 20U) {
        addIssue(
            X509CertificateProfileIssueCategory::SerialNumber,
            "serialNumber"_el,
            {},
            "Certificate serial number must be positive and at most 20 octets."_el);
    }
    _values.tbsSignatureAlgorithm = parseAlgorithmIdentifierNode(children.toRawValue()[index++]);
    _values.issuer = parseName(children.toRawValue()[index++]);
    const auto validity = children.toRawValue()[index++];
    requireNode(
        validity, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "validity"_el);
    if (validity.childCount() != ItemCount{2U}) {
        throwParseError("X.509 Validity must contain exactly two times."_el, validity);
    }
    _values.validFrom = parseTime(validity.child(ItemIndex{0U}));
    _values.validTo = parseTime(validity.child(ItemIndex{1U}));
    if (_values.validTo < _values.validFrom) {
        addIssue(
            X509CertificateProfileIssueCategory::ValidityRange,
            "validity"_el,
            {},
            "Certificate not-after time precedes its not-before time."_el);
    }
    _values.subject = parseName(children.toRawValue()[index++]);
    _values.publicKey = parseSubjectPublicKeyInfo(children.toRawValue()[index++]);
    auto hasIssuerUniqueId = false;
    auto hasSubjectUniqueId = false;
    auto hasExtensions = false;
    auto previousOptionalTag = uint32_t{};
    while (index < children.count().toSizeT()) {
        const auto optional = children.toRawValue()[index++];
        if (optional.tagClass() != Asn1TagClass::Context || optional.tagNumber() <= previousOptionalTag ||
            optional.tagNumber() > 3U) {
            throwParseError("TBSCertificate optional fields are missing, unknown, or out of order."_el, optional);
        }
        previousOptionalTag = optional.tagNumber();
        if (isNode(optional, Asn1TagClass::Context, 1U, false)) {
            if (hasIssuerUniqueId) {
                throwParseError("TBSCertificate contains a duplicate issuerUniqueID."_el, optional);
            }
            hasIssuerUniqueId = true;
            validateImplicitBitString(optional, "issuerUniqueID"_el);
        } else if (isNode(optional, Asn1TagClass::Context, 2U, false)) {
            if (hasSubjectUniqueId) {
                throwParseError("TBSCertificate contains a duplicate subjectUniqueID."_el, optional);
            }
            hasSubjectUniqueId = true;
            validateImplicitBitString(optional, "subjectUniqueID"_el);
        } else if (isNode(optional, Asn1TagClass::Context, 3U, true)) {
            if (hasExtensions) {
                throwParseError("TBSCertificate contains duplicate extensions containers."_el, optional);
            }
            hasExtensions = true;
            parseExtensions(optional);
        } else {
            throwParseError("TBSCertificate contains an unexpected optional field."_el, optional);
        }
    }
    validateTbsProfile(hasIssuerUniqueId, hasSubjectUniqueId, hasExtensions);
}

auto X509Parser::parseAlgorithmIdentifierNode(const Asn1Node &node) -> X509AlgorithmIdentifier {
    // RFC 5280 sections 4.1.1.2 and 4.1.2.3 require matching AlgorithmIdentifier values.
    requireNode(
        node,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Sequence),
        true,
        "AlgorithmIdentifier"_el);
    if (node.childCount() < ItemCount{1U} || node.childCount() > ItemCount{2U}) {
        throwParseError("AlgorithmIdentifier must contain an OID and optional parameters."_el, node);
    }
    const auto oid = node.child(ItemIndex{0U}).toObjectIdentifier();
    if (!oid.has_value()) {
        throwParseError("AlgorithmIdentifier starts with no OBJECT IDENTIFIER."_el, node);
    }
    return X509AlgorithmIdentifier{
        *oid, node.childCount() == ItemCount{2U} ? node.child(ItemIndex{1U}) : Asn1Node{}, node.encodedData()};
}

auto X509Parser::parseName(const Asn1Node &node) -> X509Name {
    // RFC 5280 section 4.1.2.4 and X.501 section 9.2: Name is an RDNSequence of attribute sets.
    requireNode(node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "Name"_el);
    auto rdns = util::List<X509RelativeDistinguishedName>{};
    for (const auto &rdnNode : node.children()) {
        requireNode(
            rdnNode,
            Asn1TagClass::Universal,
            static_cast<uint32_t>(Asn1UniversalType::Set),
            true,
            "RelativeDistinguishedName"_el);
        if (rdnNode.childCount() == ItemCount{}) {
            throwParseError("RelativeDistinguishedName must not be empty."_el, rdnNode);
        }
        auto attributes = util::List<X509NameAttribute>{};
        for (const auto &attributeNode : rdnNode.children()) {
            requireNode(
                attributeNode,
                Asn1TagClass::Universal,
                static_cast<uint32_t>(Asn1UniversalType::Sequence),
                true,
                "AttributeTypeAndValue"_el);
            if (attributeNode.childCount() != ItemCount{2U}) {
                throwParseError("AttributeTypeAndValue must contain exactly an OID and a value."_el, attributeNode);
            }
            const auto oid = attributeNode.child(ItemIndex{0U}).toObjectIdentifier();
            if (!oid.has_value()) {
                throwParseError("Name attribute has no OBJECT IDENTIFIER type."_el, attributeNode);
            }
            const auto valueNode = attributeNode.child(ItemIndex{1U});
            const auto value = valueNode.toString();
            if (!value.has_value()) {
                addIssue(
                    X509CertificateProfileIssueCategory::NameValue,
                    "name"_el,
                    *oid,
                    "Name attribute uses an unsupported or malformed string value."_el);
            }
            attributes.append(X509NameAttribute{*oid, value.value_or(String{}), valueNode});
        }
        rdns.append(X509RelativeDistinguishedName{std::move(attributes)});
    }
    return X509Name{std::move(rdns), node};
}

auto X509Parser::parseTime(const Asn1Node &node) -> time::DateTime {
    // RFC 5280 section 4.1.2.5 requires UTC Zulu time and the 2050 UTCTime/GeneralizedTime split.
    const auto type = node.universalType();
    const auto contentData = node.contentData();
    const auto content = contentData.span();
    const auto contentByteIndex = nodeContentByteIndex(node);
    uint32_t year{};
    auto position = std::size_t{};
    if (type == Asn1UniversalType::UtcTime) {
        if (content.size() != 13U || content.back().toUInt8() != 'Z') {
            throwParseError("RFC 5280 UTCTime must use YYMMDDHHMMSSZ."_el, node);
        }
        const auto shortYear = decimalPair(content, 0U, contentByteIndex);
        year = shortYear >= 50U ? 1900U + shortYear : 2000U + shortYear;
        position = 2U;
    } else if (type == Asn1UniversalType::GeneralizedTime) {
        if (content.size() != 15U || content.back().toUInt8() != 'Z') {
            throwParseError("RFC 5280 GeneralizedTime must use YYYYMMDDHHMMSSZ."_el, node);
        }
        year = decimalQuad(content, 0U, contentByteIndex);
        if (year < 2050U) {
            throwParseError("RFC 5280 requires UTCTime for years before 2050."_el, node);
        }
        position = 4U;
    } else {
        throwParseError("X.509 validity value is not UTCTime or GeneralizedTime."_el, node);
    }
    const auto month = decimalPair(content, position, contentByteIndex);
    const auto day = decimalPair(content, position + 2U, contentByteIndex);
    const auto hour = decimalPair(content, position + 4U, contentByteIndex);
    const auto minute = decimalPair(content, position + 6U, contentByteIndex);
    const auto second = decimalPair(content, position + 8U, contentByteIndex);
    if (month < 1U || month > 12U || day < 1U || day > 31U || hour > 23U || minute > 59U || second > 59U) {
        throwParseError("X.509 validity time contains an out-of-range component."_el, node);
    }
    const auto date = time::Date{time::Year{year}, time::Month{month}, time::Day{day}};
    if (!date.isValid()) {
        throwParseError("X.509 validity time contains an invalid calendar date."_el, node);
    }
    return time::DateTime{date, time::Time{time::Hour{hour}, time::Minute{minute}, time::Second{second}}};
}

auto X509Parser::parseSubjectPublicKeyInfo(const Asn1Node &node) -> PublicKey {
    // RFC 5280 section 4.1.2.7: SubjectPublicKeyInfo contains an AlgorithmIdentifier and BIT STRING.
    requireNode(
        node,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Sequence),
        true,
        "SubjectPublicKeyInfo"_el);
    if (node.childCount() != ItemCount{2U}) {
        throwParseError("SubjectPublicKeyInfo must contain algorithm and subjectPublicKey."_el, node);
    }
    auto algorithm = parseAlgorithmIdentifierNode(node.child(ItemIndex{0U}));
    const auto keyNode = node.child(ItemIndex{1U});
    requireNode(
        keyNode,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::BitString),
        false,
        "subjectPublicKey"_el);
    const auto content = keyNode.contentData();
    return PublicKey{
        std::move(algorithm),
        content.slice(ByteIndex{1U}, content.length() - ByteLength{1U}),
        content.span().front().toUInt8(),
        node,
        node.encodedData()};
}

void X509Parser::parseExtensions(const Asn1Node &explicitNode) {
    // RFC 5280 sections 4.1.2.9 and 4.2: v3 Extensions are explicit [3] and each extnValue wraps DER in OCTET STRING.
    if (explicitNode.childCount() != ItemCount{1U}) {
        throwParseError("Extensions explicit field must contain one SEQUENCE."_el, explicitNode);
    }
    const auto sequence = explicitNode.child(ItemIndex{0U});
    requireNode(
        sequence, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "Extensions"_el);
    if (sequence.childCount() == ItemCount{}) {
        throwParseError("Extensions must contain at least one extension."_el, sequence);
    }
    auto seen = std::set<String>{};
    for (const auto &extensionNode : sequence.children()) {
        requireNode(
            extensionNode,
            Asn1TagClass::Universal,
            static_cast<uint32_t>(Asn1UniversalType::Sequence),
            true,
            "Extension"_el);
        if (extensionNode.childCount() < ItemCount{2U} || extensionNode.childCount() > ItemCount{3U}) {
            throwParseError("Extension must contain OID, optional critical flag, and extnValue."_el, extensionNode);
        }
        auto childIndex = std::size_t{};
        const auto oid = extensionNode.child(ItemIndex{childIndex++}).toObjectIdentifier();
        if (!oid.has_value()) {
            throwParseError("Extension has no OBJECT IDENTIFIER."_el, extensionNode);
        }
        if (!seen.insert(oid->toString()).second) {
            throwParseError("Certificate contains a duplicate extension OID."_el, extensionNode);
        }
        auto critical = false;
        auto valueNode = extensionNode.child(ItemIndex{childIndex});
        if (valueNode.universalType() == Asn1UniversalType::Boolean) {
            critical = *valueNode.toBoolean();
            ++childIndex;
            if (!critical) {
                throwParseError("DER Extension must omit the default false critical flag."_el, valueNode);
            }
        }
        if (childIndex + 1U != extensionNode.childCount().toSizeT()) {
            throwParseError("Extension has an invalid field order."_el, extensionNode);
        }
        valueNode = extensionNode.child(ItemIndex{childIndex});
        requireNode(
            valueNode,
            Asn1TagClass::Universal,
            static_cast<uint32_t>(Asn1UniversalType::OctetString),
            false,
            "extnValue"_el);
        const auto value = valueNode.contentData();
        auto innerNode = Asn1Node{};
        auto innerReadable = true;
        try {
            innerNode = DerParser{value}.parseDocument();
        } catch (const err::ParseError &) {
            innerReadable = false;
            addIssue(
                X509CertificateProfileIssueCategory::ExtensionValue,
                "extensions"_el,
                *oid,
                "X.509 extension value does not contain one canonical DER value."_el);
        }
        auto extension = X509Extension{*oid, critical, value, innerNode};
        _values.extensions.append(extension);
        if (innerReadable) {
            parseKnownExtension(extension);
        }
    }
}

}
