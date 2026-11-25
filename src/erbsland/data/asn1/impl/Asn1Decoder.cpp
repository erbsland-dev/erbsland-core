// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1Decoder.hpp"

#include "DecodedNode.hpp"

#include "../../../err/ParseError.hpp"
#include "../../../text/Literals.hpp"

#include <limits>

namespace erbsland::data::asn1::impl {

using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto Asn1Decoder::readHeader(ByteReader &reader, const ByteIndex base) const -> std::optional<Header> {
    const auto start = reader.position();
    const auto offset = base + start.distanceFromZero();
    if (reader.isAtEnd()) {
        return std::nullopt;
    }
    const auto identifier = reader.readByte().toUInt8();
    const auto tagClass = static_cast<Asn1TagClass>(identifier >> 6U);
    const bool constructed = (identifier & 0x20U) != 0U;
    auto number = uint32_t{identifier & 0x1fU};
    if (number == 31U) {
        const auto tagOffset = offset + ByteCount::one();
        number = 0U;
        while (true) {
            if (reader.isAtEnd()) {
                return std::nullopt;
            }
            const auto octet = reader.readByte().toUInt8();
            const auto digit = static_cast<uint32_t>(octet & 0x7fU);
            if ((number == 0U && digit == 0U) || number > (UINT32_MAX >> 7U)) {
                throwParseError("Invalid ASN.1 high tag number."_el, tagOffset);
            }
            number = (number << 7U) | digit;
            if ((octet & 0x80U) == 0U) {
                break;
            }
            if (number > (UINT32_MAX >> 7U)) {
                throwParseError("ASN.1 tag exceeds the supported range."_el, tagOffset);
            }
        }
        if (number < 31U) {
            throwParseError("ASN.1 tag number is not minimally encoded."_el, tagOffset);
        }
    }
    const auto tag = Asn1Tag{tagClass, number};
    validateForm(tag, constructed, offset);
    if (reader.isAtEnd()) {
        return std::nullopt;
    }
    const auto lengthOffset = base + reader.position().distanceFromZero();
    const auto first = reader.readByte().toUInt8();
    uint64_t length = first;
    if (first >= 128U) {
        if (first == 128U || first == 255U) {
            throwParseError("Expected a definite ASN.1 length."_el, lengthOffset);
        }
        const auto octets = first & 0x7fU;
        const auto headerLength = start.absoluteDistanceTo(reader.position()) + ByteCount::fromInteger(octets);
        if (headerLength > _limits.maximumLength) {
            throwParseError("ASN.1 header exceeds the document limit."_el, lengthOffset);
        }
        length = 0U;
        for (auto i = 0U; i < octets; ++i) {
            if (reader.isAtEnd()) {
                return std::nullopt;
            }
            const auto octet = reader.readByte().toUInt8();
            if (_rules == EncodingRules::Der && i == 0U && octet == 0U) {
                throwParseError("DER length has a leading zero."_el, lengthOffset + ByteCount::one());
            }
            if (length > (ByteCount::maximum().toRawValue() >> 8U)) {
                throwParseError("ASN.1 length is too large."_el, lengthOffset);
            }
            length = (length << 8U) | octet;
            if (length > _limits.maximumLength.toRawValue() || length > ByteCount::maximum().toRawValue()) {
                throwParseError("ASN.1 length exceeds the document limit."_el, lengthOffset);
            }
        }
        if (_rules == EncodingRules::Der && length < 128U) {
            throwParseError("DER length is not minimally encoded."_el, lengthOffset);
        }
    }
    const auto headerLength = start.absoluteDistanceTo(reader.position());
    if (headerLength > _limits.maximumLength || length > (_limits.maximumLength - headerLength).toRawValue()) {
        throwParseError("ASN.1 frame exceeds the document limit."_el, lengthOffset);
    }
    return Header{tag, constructed, ByteCount::fromInteger(length)};
}

auto Asn1Decoder::frameLength(const ByteBlock &bytes) const -> std::optional<ByteCount> {
    auto reader = ByteReader{bytes};
    const auto header = readHeader(reader, ByteIndex::zero());
    if (!header && bytes.length() >= _limits.maximumLength) {
        throwParseError("Incomplete ASN.1 header exceeds the frame budget."_el, reader.position());
    }
    if (!header || !reader.canRead(header->length)) {
        return std::nullopt;
    }
    return reader.position().distanceFromZero() + header->length;
}

auto Asn1Decoder::parse(const ByteBlock &bytes, const ByteIndex base) -> Asn1Node {
    if (bytes.length() > _limits.maximumLength) {
        throwParseError("ASN.1 document exceeds the input limit."_el, base);
    }
    _nodes = {};
    auto reader = ByteReader{bytes};
    auto result = parseNode(reader, bytes, base, ItemCount::one());
    if (!reader.isAtEnd()) {
        throwParseError("ASN.1 document contains trailing data."_el, base + reader.position().distanceFromZero());
    }
    return result;
}

auto Asn1Decoder::parseNode(ByteReader &reader, const ByteBlock &source, const ByteIndex base, const ItemCount depth)
    -> Asn1Node {
    const auto start = reader.position();
    const auto offset = base + start.distanceFromZero();
    if (depth > _limits.maximumDepth || depth > ItemCount{32U} || _nodes >= _limits.maximumNodes) {
        throwParseError("ASN.1 document exceeds the nesting or node limit."_el, offset);
    }
    ++_nodes;
    const auto header = readHeader(reader, base);
    const auto contentOffset = base + reader.position().distanceFromZero();
    if (!header) {
        throwParseError("ASN.1 header is truncated."_el, contentOffset);
    }
    auto node = DecodedNode{};
    node.offset = offset;
    node.tag = header->tag;
    node.constructed = header->constructed;
    node.der = _rules == EncodingRules::Der;
    node.headerLength = start.absoluteDistanceTo(reader.position());
    const auto content = reader.readBytes(header->length);
    if (!content) {
        throwParseError("ASN.1 content exceeds the enclosing value."_el, contentOffset);
    }
    node.encoded = source.slice(start, reader.position());
    if (node.constructed) {
        auto children = ByteReader{*content};
        bool tagOrder = true;
        bool encodingOrder = true;
        const bool derSet = node.der && node.tag == Asn1Tag{Asn1UniversalType::Set};
        while (!children.isAtEnd()) {
            auto child = parseNode(children, *content, contentOffset, depth + ItemCount::one());
            if (derSet && !node.children.isEmpty()) {
                const auto &previous = node.children.getRefOrThrow(node.children.lastIndex());
                tagOrder = tagOrder && previous.tag() < child.tag();
                encodingOrder = encodingOrder && previous.encodedData() <= child.encodedData();
                if (!tagOrder && !encodingOrder) {
                    throwParseError("DER elements violate both SET and SET OF ordering."_el, child.encodedByteIndex());
                }
            }
            node.children.append(std::move(child));
        }
    } else if (node.tag.tagClass == Asn1TagClass::Universal) {
        validatePrimitive(static_cast<Asn1UniversalType>(node.tag.number), *content, contentOffset);
    }
    return Asn1Node{std::move(node)};
}

void Asn1Decoder::validateForm(const Asn1Tag tag, const bool constructed, const ByteIndex offset) const {
    if (_rules == EncodingRules::LdapBer && tag.tagClass == Asn1TagClass::Private) {
        throwParseError("Private identifiers are outside the LDAP BER profile."_el, offset);
    }
    if (tag.tagClass != Asn1TagClass::Universal) {
        return;
    }
    const auto type = static_cast<Asn1UniversalType>(tag.number);
    if (type == Asn1UniversalType::None || tag.number == 15U) {
        throwParseError("Reserved ASN.1 universal identifier."_el, offset);
    }
    const bool container = type == Asn1UniversalType::Sequence || type == Asn1UniversalType::Set;
    if (constructed != container) {
        throwParseError("Unexpected ASN.1 primitive or constructed form."_el, offset);
    }
    if (_rules == EncodingRules::LdapBer && !container && type != Asn1UniversalType::Boolean &&
        type != Asn1UniversalType::Integer && type != Asn1UniversalType::Enumerated &&
        type != Asn1UniversalType::OctetString && type != Asn1UniversalType::Null) {
        throwParseError("Universal tag is outside the LDAP BER profile."_el, offset);
    }
}

void Asn1Decoder::throwParseError(String reason, const ByteIndex byteIndex) {
    throw err::ParseError{std::move(reason), byteIndex};
}

}
