// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DerParser.hpp"

#include "Asn1ObjectIdentifierCodec.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/EncodingError.hpp"
#include "../../text/StringDecoder.hpp"

#include <limits>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace unit;
using namespace text;
using namespace text::literals;

DerParser::DerParser(mem::ByteBlock der) noexcept : _der{std::move(der)}, _reader{_der} {
}

auto DerParser::parseDocument() -> Asn1Node {
    if (_der.length() > cMaximumLength) {
        throwParseError("DER certificate exceeds the fixed one MiB limit."_el, ByteIndex::zero());
    }
    if (_der.isEmpty()) {
        throwParseError("DER input is empty."_el, ByteIndex::zero());
    }
    auto root = parseNode(0U);
    if (!_reader.isAtEnd()) {
        throwParseError("DER input contains trailing data."_el, _reader.position());
    }
    return root;
}

auto DerParser::parseNode(const std::size_t depth) -> Asn1Node {
    // ITU-T X.690 section 8.1: parse one identifier-length-contents value; sections 10 and 11 add DER rules.
    if (depth >= cMaximumDepth) {
        throw err::OutOfRangeError{"DER nesting exceeds the fixed depth limit."_el};
    }
    if (_nodeCount >= cMaximumNodes) {
        throw err::OutOfRangeError{"DER input exceeds the fixed node-count limit."_el};
    }
    ++_nodeCount;
    const auto encodedOffset = _reader.position();
    const auto identifier = readByte();
    const auto tagClass = static_cast<Asn1TagClass>((identifier >> 6U) & 0x03U);
    const auto constructed = (identifier & 0x20U) != 0U;
    const auto tagNumber = readTagNumber(identifier);
    const auto contentLength = readLength();
    if (!_reader.canRead(contentLength)) {
        throwParseError("DER value is truncated."_el, _reader.position());
    }
    const auto contentOffset = _reader.position();
    const auto endOffset = contentOffset + contentLength;
    validateUniversalForm(tagClass, tagNumber, constructed, encodedOffset);
    auto children = util::List<Asn1Node>{};
    if (!constructed) {
        validatePrimitive(tagClass, tagNumber, contentOffset, contentLength);
        _reader.setPosition(endOffset);
    } else {
        while (_reader.position() < endOffset) {
            const auto childOffset = _reader.position();
            children.append(parseNode(depth + 1U));
            if (_reader.position() > endOffset) {
                throwParseError("DER child exceeds its parent value."_el, childOffset);
            }
        }
        if (_reader.position() != endOffset) {
            throwParseError("DER constructed value has an invalid length."_el, encodedOffset);
        }
        if (tagClass == Asn1TagClass::Universal && tagNumber == static_cast<uint32_t>(Asn1UniversalType::Set)) {
            validateSetOrder(children);
        }
    }
    const auto headerLength = encodedOffset.absoluteDistanceTo(contentOffset);
    const auto encodedLength = encodedOffset.absoluteDistanceTo(endOffset);
    return Asn1Node{_der.slice(encodedOffset, encodedLength), headerLength, tagNumber, std::move(children)};
}

auto DerParser::readTagNumber(const uint8_t identifier) -> uint32_t {
    const auto tagNumber = static_cast<uint32_t>(identifier & 0x1FU);
    if (tagNumber != 0x1FU) {
        return tagNumber;
    }
    // ITU-T X.690 section 8.1.2.4: high-tag-number form is an unsigned base-128 integer.
    const auto tagOffset = _reader.position();
    if (!_reader.canRead(ByteLength::one())) {
        throwParseError("DER input is truncated."_el, tagOffset);
    }
    if ((_reader.peekByte().toUInt8() & 0x7fU) == 0U) {
        throwParseError("DER tag number is not minimally encoded."_el, tagOffset);
    }
    auto decodedTagNumber = uint64_t{};
    try {
        decodedTagNumber = _reader.readIntegerOrThrow<uint64_t>(mem::ByteIntegerFormat::UnsignedBase128);
    } catch (const err::OutOfRangeError &) {
        throwParseError("DER tag number is truncated."_el, tagOffset);
    } catch (const err::OverflowError &) {
        throwParseError("DER tag number is too large."_el, tagOffset);
    } catch (const err::ParseError &) {
        throwParseError("DER tag number is not minimally encoded."_el, tagOffset);
    }
    if (decodedTagNumber > std::numeric_limits<uint32_t>::max()) {
        throwParseError("DER tag number is too large."_el, tagOffset);
    }
    if (decodedTagNumber < 31U) {
        throwParseError("DER tag number is not minimally encoded."_el, tagOffset);
    }
    return static_cast<uint32_t>(decodedTagNumber);
}

auto DerParser::readLength() -> ByteLength {
    // ITU-T X.690 sections 8.1.3 and 10.1: DER uses minimally encoded definite lengths.
    const auto lengthOffset = _reader.position();
    const auto firstLength = readByte();
    if ((firstLength & 0x80U) == 0U) {
        return ByteLength{firstLength};
    }
    const auto lengthOctets = ByteLength{firstLength & 0x7FU};
    if (lengthOctets.isZero()) {
        throwParseError("DER forbids indefinite lengths."_el, lengthOffset);
    }
    // A minimally encoded four-octet length is already larger than the fixed one MiB document limit.
    if (lengthOctets > ByteLength{3U}) {
        throwParseError("DER length is too large."_el, lengthOffset);
    }
    if (!_reader.canRead(lengthOctets)) {
        throwParseError("DER input is truncated."_el, _reader.position());
    }
    if (_reader.peekByte().toUInt8() == 0U) {
        throwParseError("DER length is not minimally encoded."_el, _reader.position());
    }
    _reader.setEndianness(mem::Endianness::Big);
    auto length = uint32_t{};
    switch (lengthOctets.toSizeT()) {
    case 1U:
        length = _reader.readIntegerOrThrow<uint32_t>(mem::ByteIntegerFormat::UnsignedFixed8Bit);
        break;
    case 2U:
        length = _reader.readIntegerOrThrow<uint32_t>(mem::ByteIntegerFormat::UnsignedFixed16Bit);
        break;
    case 3U:
        length = _reader.readIntegerOrThrow<uint32_t>(mem::ByteIntegerFormat::UnsignedFixed24Bit);
        break;
    default:
        throwParseError("DER length is invalid."_el, lengthOffset);
    }
    if (length < 128U) {
        throwParseError("DER length is not minimally encoded."_el, lengthOffset);
    }
    return ByteLength{length};
}

auto DerParser::readByte() -> uint8_t {
    if (!_reader.canRead(ByteLength::one())) {
        throwParseError("DER input is truncated."_el, _reader.position());
    }
    return _reader.readByte().toUInt8();
}

void DerParser::validateUniversalForm(
    const Asn1TagClass tagClass,
    const uint32_t tagNumber,
    const bool constructed,
    const ByteIndex encodedOffset) const {
    // ITU-T X.690 sections 8 and 10: universal types have fixed primitive/constructed forms in DER.
    if (tagClass != Asn1TagClass::Universal) {
        return;
    }
    if (tagNumber == 0U) {
        throwParseError("DER forbids the end-of-contents tag."_el, encodedOffset);
    }
    const auto isContainer = tagNumber == static_cast<uint32_t>(Asn1UniversalType::Sequence) ||
        tagNumber == static_cast<uint32_t>(Asn1UniversalType::Set);
    if (isContainer != constructed) {
        throwParseError("DER value uses an invalid primitive or constructed form."_el, encodedOffset);
    }
    switch (static_cast<Asn1UniversalType>(tagNumber)) {
    case Asn1UniversalType::Boolean:
    case Asn1UniversalType::Integer:
    case Asn1UniversalType::BitString:
    case Asn1UniversalType::OctetString:
    case Asn1UniversalType::Null:
    case Asn1UniversalType::ObjectIdentifier:
    case Asn1UniversalType::Utf8String:
    case Asn1UniversalType::NumericString:
    case Asn1UniversalType::PrintableString:
    case Asn1UniversalType::TeletexString:
    case Asn1UniversalType::Ia5String:
    case Asn1UniversalType::UtcTime:
    case Asn1UniversalType::GeneralizedTime:
    case Asn1UniversalType::UniversalString:
    case Asn1UniversalType::BmpString:
        if (constructed) {
            throwParseError("DER string or primitive value must use primitive encoding."_el, encodedOffset);
        }
        break;
    default:
        break;
    }
}

void DerParser::validatePrimitive(
    const Asn1TagClass tagClass,
    const uint32_t tagNumber,
    const ByteIndex contentOffset,
    const ByteLength contentLength) const {
    if (tagClass != Asn1TagClass::Universal) {
        return;
    }
    const auto content = _der.span(contentOffset, contentLength);
    switch (static_cast<Asn1UniversalType>(tagNumber)) {
    case Asn1UniversalType::Boolean:
        // ITU-T X.690 sections 8.2 and 11.1: DER BOOLEAN is one octet, FALSE=00 and TRUE=FF.
        if (content.size() != 1U || (content.front().toUInt8() != 0U && content.front().toUInt8() != 0xFFU)) {
            throwParseError("DER BOOLEAN is malformed or noncanonical."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::Integer:
        validateInteger(content, contentOffset);
        break;
    case Asn1UniversalType::BitString:
        validateBitString(content, contentOffset);
        break;
    case Asn1UniversalType::Null:
        // ITU-T X.690 section 8.8: NULL has zero content octets.
        if (!content.empty()) {
            throwParseError("DER NULL contains content octets."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::ObjectIdentifier:
        Asn1ObjectIdentifierCodec{_der.slice(contentOffset, contentLength), contentOffset}.validate();
        break;
    case Asn1UniversalType::Utf8String: {
        // ITU-T X.690 section 8.23.2 requires UTF8String contents to be valid UTF-8.
        try {
            StringDecoder{_der.slice(contentOffset, contentLength)}.validateOrThrow(
                StringEncoding::Utf8, StringBomMode::Reject);
        } catch (const EncodingError &) {
            throwParseError("DER UTF8String contains malformed UTF-8."_el, contentOffset);
        }
        break;
    }
    case Asn1UniversalType::NumericString: {
        // ITU-T X.680 section 41.1 defines NumericString as digits and SPACE.
        auto byteIndex = contentOffset;
        for (const auto byte : content) {
            if (byte.toUInt8() != 0x20U && (byte.toUInt8() < '0' || byte.toUInt8() > '9')) {
                throwParseError("DER NumericString contains an invalid character."_el, byteIndex);
            }
            ++byteIndex;
        }
        break;
    }
    case Asn1UniversalType::PrintableString:
        validatePrintableString(content, contentOffset);
        break;
    case Asn1UniversalType::Ia5String:
    case Asn1UniversalType::UtcTime:
    case Asn1UniversalType::GeneralizedTime: {
        auto byteIndex = contentOffset;
        for (const auto byte : content) {
            if (byte.toUInt8() > 0x7FU) {
                throwParseError("DER ASCII string contains a non-ASCII character."_el, byteIndex);
            }
            ++byteIndex;
        }
        break;
    }
    case Asn1UniversalType::BmpString:
        if (content.size() % 2U != 0U) {
            throwParseError("DER BMPString has an invalid length."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::UniversalString:
        if (content.size() % 4U != 0U) {
            throwParseError("DER UniversalString has an invalid length."_el, contentOffset);
        }
        break;
    default:
        break;
    }
}

void DerParser::validateInteger(const mem::ConstByteSpan content, const ByteIndex contentOffset) const {
    // ITU-T X.690 sections 8.3 and 10.2: two's-complement INTEGER uses the fewest possible octets.
    if (content.empty()) {
        throwParseError("DER INTEGER is empty."_el, contentOffset);
    }
    if (content.size() > 1U) {
        const auto first = content[0U].toUInt8();
        const auto second = content[1U].toUInt8();
        if ((first == 0U && (second & 0x80U) == 0U) || (first == 0xFFU && (second & 0x80U) != 0U)) {
            throwParseError("DER INTEGER is not minimally encoded."_el, contentOffset);
        }
    }
}

void DerParser::validateBitString(const mem::ConstByteSpan content, const ByteIndex contentOffset) const {
    // ITU-T X.690 sections 8.6 and 11.2: first octet is unused-bit count and padding bits are zero.
    if (content.empty() || content.front().toUInt8() > 7U) {
        throwParseError("DER BIT STRING has an invalid unused-bit count."_el, contentOffset);
    }
    const auto unused = content.front().toUInt8();
    if (content.size() == 1U && unused != 0U) {
        throwParseError("Empty DER BIT STRING has unused bits."_el, contentOffset);
    }
    if (unused != 0U && (content.back().toUInt8() & ((1U << unused) - 1U)) != 0U) {
        throwParseError(
            "DER BIT STRING has nonzero unused bits."_el, contentOffset + ByteLength::fromSizeT(content.size() - 1U));
    }
}

void DerParser::validatePrintableString(const mem::ConstByteSpan content, const ByteIndex contentOffset) const {
    // ITU-T X.680 section 41.4 defines the complete PrintableString character repertoire.
    static const auto cPrintableCharacters = CharSet::fromPattern("A-Za-z0-9 '()+,./:=?-"_el);
    auto byteIndex = contentOffset;
    for (const auto byte : content) {
        if (!cPrintableCharacters.contains(Char{static_cast<char32_t>(byte.toUInt8())})) {
            throwParseError("DER PrintableString contains an invalid character."_el, byteIndex);
        }
        ++byteIndex;
    }
}

void DerParser::validateSetOrder(const util::List<Asn1Node> &children) const {
    // ITU-T X.690 section 11.6: SET components are ordered by their complete DER encodings.
    for (auto index = std::size_t{1U}; index < children.count().toSizeT(); ++index) {
        const auto &previous = children.getRefOrThrow(ItemIndex{index - 1U});
        const auto &current = children.getRefOrThrow(ItemIndex{index});
        const auto previousBytes = previous.encodedData();
        const auto currentBytes = current.encodedData();
        if (currentBytes < previousBytes) {
            const auto derStorageIndex = mem::impl::UnsafeByteBlockAccess{_der}.dataView().range().index();
            const auto byteIndex =
                ByteIndex::end(derStorageIndex.absoluteDistanceTo(current.encodedStorageByteIndex()));
            throwParseError("DER SET elements are not in canonical order."_el, byteIndex);
        }
    }
}

void DerParser::throwParseError(String reason, const ByteIndex byteIndex) {
    throw err::ParseError{std::move(reason), byteIndex};
}

}
