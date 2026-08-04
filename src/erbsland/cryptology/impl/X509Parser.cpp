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
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "subjectAltName"_el);
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
        return X509GeneralName{kind, String{value}, {}, {}, {}, node};
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
                const auto partIndex = ByteIndex{part * 8U};
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
        return X509GeneralName{X509GeneralName::Kind::RegisteredId, {}, {}, {}, decodeImplicitOid(node), node};
    }
    if (!node.isConstructed()) {
        throwParseError("Structured GeneralName alternative must use constructed encoding."_el, node);
    }
    if (tag == 0U) {
        if (node.childCount() != ItemCount{2U} || !node.child(ItemIndex{0U}).toObjectIdentifier().has_value() ||
            !isNode(node.child(ItemIndex{1U}), Asn1TagClass::Context, 0U, true)) {
            throwParseError("otherName GeneralName is malformed."_el, node);
        }
    }
    return X509GeneralName{X509GeneralName::Kind::Unsupported, {}, {}, {}, {}, node};
}

void X509Parser::parseSubjectKeyIdentifier(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.2: SubjectKeyIdentifier is an OCTET STRING.
    const auto root = extension.asn1();
    requireNode(
        root,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::OctetString),
        false,
        "subjectKeyIdentifier"_el);
    _values.subjectKeyIdentifier = root.contentData();
}

void X509Parser::parseAuthorityKeyIdentifier(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.1: ordered implicit keyIdentifier, authorityCertIssuer, and serial fields.
    const auto root = extension.asn1();
    requireNode(
        root,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Sequence),
        true,
        "authorityKeyIdentifier"_el);
    auto previousTag = std::optional<uint32_t>{};
    auto hasIssuer = false;
    auto hasSerialNumber = false;
    for (const auto &child : root.children()) {
        if (child.tagClass() != Asn1TagClass::Context || child.tagNumber() > 2U ||
            (previousTag.has_value() && child.tagNumber() <= *previousTag)) {
            throwParseError("Authority Key Identifier fields are unknown, duplicated, or out of order."_el, child);
        }
        previousTag = child.tagNumber();
        if (isNode(child, Asn1TagClass::Context, 0U, false)) {
            _values.authorityKeyIdentifier = child.contentData();
        } else if (isNode(child, Asn1TagClass::Context, 1U, true)) {
            if (child.childCount() == ItemCount{}) {
                throwParseError("Authority Key Identifier issuer GeneralNames must not be empty."_el, child);
            }
            for (const auto &name : child.children()) {
                // The issuer names are not exposed, but fully decoding each value validates the extension syntax.
                [[maybe_unused]] const auto validatedIssuerName = parseGeneralName(name);
            }
            hasIssuer = true;
        } else if (isNode(child, Asn1TagClass::Context, 2U, false)) {
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
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "basicConstraints"_el);
    auto index = std::size_t{};
    auto certificateAuthority = false;
    auto pathLength = std::optional<uint32_t>{};
    if (root.childCount().toSizeT() > index &&
        root.child(ItemIndex{index}).universalType() == Asn1UniversalType::Boolean) {
        certificateAuthority = *root.child(ItemIndex{index++}).toBoolean();
        if (!certificateAuthority) {
            throwParseError("Basic Constraints must omit its default false cA value."_el, root);
        }
    }
    if (root.childCount().toSizeT() > index) {
        pathLength = static_cast<uint32_t>(
            unsignedInteger(root.child(ItemIndex{index++}), std::numeric_limits<uint32_t>::max()));
        if (!certificateAuthority) {
            throwParseError("Basic Constraints pathLenConstraint requires cA true."_el, root);
        }
    }
    if (index != root.childCount().toSizeT()) {
        throwParseError("Basic Constraints contains unexpected fields."_el, root);
    }
    _values.basicConstraints = X509BasicConstraints{certificateAuthority, pathLength};
}

void X509Parser::parseKeyUsage(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.3 and X.690 section 11.2: KeyUsage is a canonical named BIT STRING.
    const auto root = extension.asn1();
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::BitString), false, "keyUsage"_el);
    const auto contentData = root.contentData();
    const auto content = contentData.span();
    const auto unusedBitCount = static_cast<std::size_t>(content.front().toUInt8());
    const auto bitCount = (content.size() - 1U) * 8U - unusedBitCount;
    if (bitCount == 0U || bitCount > 9U || content.back().toUInt8() == 0U) {
        throwParseError("Key Usage has no bits or contains bits outside its named range."_el, root);
    }
    auto canonicalUnusedBitCount = std::size_t{};
    auto lastByte = content.back().toUInt8();
    while ((lastByte & 1U) == 0U) {
        ++canonicalUnusedBitCount;
        lastByte >>= 1U;
    }
    if (canonicalUnusedBitCount != unusedBitCount) {
        throwParseError("Key Usage does not use canonical named-bit-list encoding."_el, root);
    }
    auto usages = X509KeyUsages{};
    for (auto bit = std::size_t{}; bit < 9U; ++bit) {
        const auto byteIndex = 1U + bit / 8U;
        if (byteIndex >= content.size()) {
            break;
        }
        if ((content[byteIndex].toUInt8() & (0x80U >> (bit % 8U))) != 0U) {
            usages |= static_cast<X509KeyUsage>(1U << bit);
        }
    }
    _values.keyUsage = usages;
}

void X509Parser::parseExtendedKeyUsage(const X509Extension &extension) {
    // RFC 5280 section 4.2.1.12: ExtendedKeyUsage is a nonempty sequence of unique purpose OIDs.
    const auto root = extension.asn1();
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "extendedKeyUsage"_el);
    if (root.childCount() == ItemCount{}) {
        throwParseError("Extended Key Usage must not be empty."_el, root);
    }
    auto seen = std::set<String>{};
    for (const auto &child : root.children()) {
        const auto oid = child.toObjectIdentifier();
        if (!oid.has_value()) {
            throwParseError("Extended Key Usage contains a non-OID value."_el, child);
        }
        if (!seen.insert(oid->toString()).second) {
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

auto X509Parser::decodeImplicitOid(const Asn1Node &node) -> Asn1ObjectIdentifier {
    // X.690 section 8.19 is independent of the surrounding implicit tag; decode the content directly.
    return Asn1ObjectIdentifier::fromStringOrThrow(
        Asn1ObjectIdentifierCodec{node.contentData(), nodeContentByteIndex(node)}.decode());
}

void X509Parser::validateImplicitBitString(const Asn1Node &node, const String &name) const {
    const auto contentData = node.contentData();
    const auto content = contentData.span();
    if (content.empty() || content.front().toUInt8() > 7U) {
        throwParseError(fieldDiagnostic(name, " contains an invalid BIT STRING."_el), node);
    }
    const auto unused = content.front().toUInt8();
    if ((content.size() == 1U && unused != 0U) ||
        (unused != 0U && (content.back().toUInt8() & ((1U << unused) - 1U)) != 0U)) {
        throwParseError(fieldDiagnostic(name, " contains malformed unused bits."_el), node);
    }
}

void X509Parser::validateImplicitSerialNumber(const Asn1Node &node, const String &name) const {
    const auto contentData = node.contentData();
    const auto content = contentData.span();
    if (content.empty() || (content.front().toUInt8() & 0x80U) != 0U) {
        throwParseError(fieldDiagnostic(name, " must contain a nonnegative INTEGER."_el), node);
    }
    if (content.size() > 1U && content.front().toUInt8() == 0U && (content[1U].toUInt8() & 0x80U) == 0U) {
        throwParseError(fieldDiagnostic(name, " contains a nonminimal INTEGER."_el), node);
    }
    auto magnitudeLength = content.size();
    if (content.front().toUInt8() == 0U) {
        --magnitudeLength;
    }
    auto zero = true;
    for (const auto byte : content) {
        zero = zero && byte.toUInt8() == 0U;
    }
    if (zero || magnitudeLength > 20U) {
        throwParseError(fieldDiagnostic(name, " must be positive and at most 20 octets."_el), node);
    }
}

auto X509Parser::unsignedInteger(const Asn1Node &node, const uint64_t maximum) const -> uint64_t {
    requireNode(node, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Integer), false, "INTEGER"_el);
    auto content = node.contentData();
    if ((content.span().front().toUInt8() & 0x80U) != 0U) {
        throwParseError("Expected a nonnegative INTEGER."_el, node);
    }
    if (content.length() > ByteLength::one() && content.span().front().toUInt8() == 0U) {
        content = content.slice(ByteIndex{1U}, content.length() - ByteLength::one());
    }
    const auto format = [&]() -> ByteIntegerFormat {
        switch (content.length().toSizeT()) {
        case 1U:
            return ByteIntegerFormat::UnsignedFixed8Bit;
        case 2U:
            return ByteIntegerFormat::UnsignedFixed16Bit;
        case 3U:
            return ByteIntegerFormat::UnsignedFixed24Bit;
        case 4U:
            return ByteIntegerFormat::UnsignedFixed32Bit;
        case 5U:
            return ByteIntegerFormat::UnsignedFixed40Bit;
        case 6U:
            return ByteIntegerFormat::UnsignedFixed48Bit;
        case 7U:
            return ByteIntegerFormat::UnsignedFixed56Bit;
        case 8U:
            return ByteIntegerFormat::UnsignedFixed64Bit;
        default:
            throwParseError("INTEGER exceeds its supported range."_el, node);
        }
    }();
    auto reader = ByteReader{std::move(content)};
    reader.setEndianness(Endianness::Big);
    const auto value = reader.readIntegerOrThrow<uint64_t>(format);
    if (value > maximum) {
        throwParseError("INTEGER exceeds its supported range."_el, node);
    }
    return value;
}

auto X509Parser::isNode(
    const Asn1Node &node, const Asn1TagClass tagClass, const uint32_t tagNumber, const bool constructed) noexcept
    -> bool {
    return !node.isEmpty() && node.tagClass() == tagClass && node.tagNumber() == tagNumber &&
        node.isConstructed() == constructed;
}

void X509Parser::requireNode(
    const Asn1Node &node,
    const Asn1TagClass tagClass,
    const uint32_t tagNumber,
    const bool constructed,
    const String &name) const {
    if (!isNode(node, tagClass, tagNumber, constructed)) {
        auto message = StringEditor{};
        message.append("Unexpected ASN.1 node for "_el);
        message.append(name);
        message.append(U'.');
        throwParseError(String{message}, node);
    }
}

auto X509Parser::asciiText(const Asn1Node &node) const -> String {
    auto result = StringEditor{};
    const auto content = node.contentData();
    for (const auto byte : content.span()) {
        if (byte.toUInt8() > 0x7FU) {
            throwParseError("Expected ASCII content."_el, node);
        }
        result.append(Char{byte.toUInt32()});
    }
    return String{result};
}

auto X509Parser::fieldDiagnostic(const String &name, const String &detail) -> String {
    auto result = StringEditor{};
    result.append(name);
    result.append(detail);
    return String{result};
}

auto X509Parser::decimalDigit(const ConstByteSpan bytes, const std::size_t index, const ByteIndex contentByteIndex)
    -> uint32_t {
    if (index >= bytes.size() || bytes[index].toUInt8() < '0' || bytes[index].toUInt8() > '9') {
        throw err::ParseError{
            "X.509 time contains a nondigit component."_el, contentByteIndex + ByteLength::fromSizeT(index)};
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

auto X509Parser::nodeByteIndex(const Asn1Node &node) const noexcept -> ByteIndex {
    if (node.isEmpty()) {
        return ByteIndex::zero();
    }
    return ByteIndex::end(_root.encodedStorageByteIndex().absoluteDistanceTo(node.encodedStorageByteIndex()));
}

auto X509Parser::nodeContentByteIndex(const Asn1Node &node) const noexcept -> ByteIndex {
    if (node.isEmpty()) {
        return ByteIndex::zero();
    }
    return nodeByteIndex(node) + node._headerLength;
}

void X509Parser::throwParseError(String reason, const Asn1Node &node) const {
    throw err::ParseError{std::move(reason), nodeByteIndex(node)};
}

}
