// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1Decoder.hpp"

#include "Asn1ObjectIdentifierCodec.hpp"
#include "PrimitiveContent.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/EncodingError.hpp"
#include "../../../text/StringDecoder.hpp"

namespace erbsland::data::asn1::impl {

using namespace unit;
using namespace mem;
using namespace text;
using namespace text::literals;

void Asn1Decoder::validatePrimitive(
    const Asn1UniversalType type, const ByteBlock &bytes, const ByteIndex contentOffset) {
    const auto content = bytes.span();
    switch (type) {
    case Asn1UniversalType::Boolean:
        // ITU-T X.690 sections 8.2 and 11.1: DER BOOLEAN is one octet, FALSE=00 and TRUE=FF.
        if (content.size() != 1U || (content.front().toUInt8() != 0U && content.front().toUInt8() != 0xFFU)) {
            throwParseError("DER BOOLEAN is malformed or noncanonical."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::Integer:
    case Asn1UniversalType::Enumerated:
        if (!isMinimalInteger(content)) {
            throwParseError("Invalid ASN.1 integer content."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::BitString:
        if (!isValidBitString(content)) {
            throwParseError("Invalid ASN.1 BIT STRING padding."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::Null:
        // ITU-T X.690 section 8.8: NULL has zero content octets.
        if (!content.empty()) {
            throwParseError("DER NULL contains content octets."_el, contentOffset);
        }
        break;
    case Asn1UniversalType::ObjectIdentifier:
        Asn1ObjectIdentifierCodec{bytes, contentOffset}.validate();
        break;
    case Asn1UniversalType::Utf8String: {
        // ITU-T X.690 section 8.23.2 requires UTF8String contents to be valid UTF-8.
        try {
            StringDecoder{bytes}.validateOrThrow(StringEncoding::Utf8, StringBomMode::Reject);
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

void Asn1Decoder::validatePrintableString(const ConstByteSpan content, const ByteIndex contentOffset) {
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

}
