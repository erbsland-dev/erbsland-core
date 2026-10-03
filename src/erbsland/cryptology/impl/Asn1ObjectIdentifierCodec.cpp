// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1ObjectIdentifierCodec.hpp"

#include "../asn1/Asn1ObjectIdentifier.hpp"

#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../mem/ByteIntegerFormat.hpp"
#include "../../mem/ByteReader.hpp"
#include "../../text/IntegerBase.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"
#include "../../unit/ByteIndex.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

Asn1ObjectIdentifierCodec::Asn1ObjectIdentifierCodec(const ByteBlock &content, const ByteIndex contentOffset) noexcept :
    _content{content}, _contentOffset{contentOffset} {
}

auto Asn1ObjectIdentifierCodec::decode() const -> String {
    auto result = StringEditor{};
    parse(&result);
    return result;
}

void Asn1ObjectIdentifierCodec::validate() const {
    parse(nullptr);
}

void Asn1ObjectIdentifierCodec::encode(const Asn1ObjectIdentifier &value, ByteBlockEditor &output) {
    auto reader = StringCharReader{value.toString()};
    auto parseOptions = IntegerParseOptions{};
    parseOptions.setFixedBase(IntegerBase::Decimal);
    const auto first = reader.readIntegerOrThrow<uint64_t>(parseOptions);
    if (!reader.readIf(U'.')) {
        throw ParseError{"Malformed ASN.1 object identifier."};
    }
    const auto second = reader.readIntegerOrThrow<uint64_t>(parseOptions);
    appendEncodedArc(output, first * 40U + second);
    while (!reader.isAtEnd()) {
        if (!reader.readIf(U'.')) {
            throw ParseError{"Malformed ASN.1 object identifier."};
        }
        appendEncodedArc(output, reader.readIntegerOrThrow<uint64_t>(parseOptions));
    }
}

void Asn1ObjectIdentifierCodec::parse(StringEditor *const output) const {
    // ITU-T X.690 section 8.19: OID subidentifiers are minimal unsigned base-128 integers.
    if (_content.isEmpty()) {
        throw ParseError{"DER OBJECT IDENTIFIER is empty."_el, _contentOffset};
    }
    auto reader = ByteReader{_content};
    auto firstSubIdentifier = true;
    while (!reader.isAtEnd()) {
        const auto relativeOffset = reader.position();
        const auto arcOffset = _contentOffset + relativeOffset.distanceFromZero();
        if ((reader.peekByte().toUInt8() & 0x7fU) == 0U && reader.peekByte().toUInt8() != 0U) {
            throw ParseError{"DER OBJECT IDENTIFIER arc is not minimally encoded."_el, arcOffset};
        }
        const auto value = reader.readInteger<uint64_t>(ByteIntegerFormat::UnsignedBase128);
        if (!value.has_value()) {
            throw ParseError{"DER OBJECT IDENTIFIER arc is truncated, invalid, or too large."_el, arcOffset};
        }
        appendArc(output, *value, firstSubIdentifier);
        firstSubIdentifier = false;
    }
}

void Asn1ObjectIdentifierCodec::appendEncodedArc(ByteBlockEditor &output, const uint64_t value) {
    constexpr auto cArraySize = std::size_t{10U};
    auto bytes = ByteArray<cArraySize>{};
    auto count = ByteCount{};
    auto remaining = value;
    do {
        bytes.set(bytes.lastIndex() - count, Byte{static_cast<uint8_t>(remaining & 0x7fU)});
        remaining >>= 7U;
        ++count;
    } while (remaining != 0U);
    for (auto index = bytes.endIndex() - count; index.isWithin(bytes.length()); index.uncheckedIncrement()) {
        auto continuation = Byte{};
        if (index != bytes.lastIndex()) {
            continuation = Byte{0x80U};
        }
        bytes.set(index, bytes.get(index) | continuation);
    }
    output.append(bytes.span(bytes.endIndex() - count, count));
}

void Asn1ObjectIdentifierCodec::appendArc(
    StringEditor *const output, const uint64_t value, const bool firstSubIdentifier) {
    if (output == nullptr) {
        return;
    }
    if (firstSubIdentifier) {
        const auto first = value < 40U ? uint64_t{} : (value < 80U ? uint64_t{1U} : uint64_t{2U});
        output->append(String::fromInteger(first));
        output->append(U'.');
        output->append(String::fromInteger(value - first * 40U));
        return;
    }
    output->append(U'.');
    output->append(String::fromInteger(value));
}

}
