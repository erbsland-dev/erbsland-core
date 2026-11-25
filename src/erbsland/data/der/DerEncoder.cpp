// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DerEncoder.hpp"

#include "DerParser.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../time/Timestamp.hpp"

#include <algorithm>

namespace erbsland::data::der {

using namespace asn1;
using err::ParameterFailure;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

auto DerEncoder::beginSequence() -> Scope {
    return _writer.begin(Asn1UniversalType::Sequence, true);
}

auto DerEncoder::beginSetOf() -> Scope {
    return _writer.begin(Asn1UniversalType::Set, true, asn1::impl::Asn1Writer::Order::Encoding);
}

auto DerEncoder::beginSet() -> Scope {
    return _writer.begin(Asn1UniversalType::Set, true, asn1::impl::Asn1Writer::Order::Tag);
}

auto DerEncoder::beginExplicit(const uint32_t number) -> Scope {
    return _writer.begin({Asn1TagClass::Context, number}, true);
}

auto DerEncoder::beginOctetString() -> Scope {
    return _writer.begin(Asn1UniversalType::OctetString, false);
}

auto DerEncoder::beginBitString() -> Scope {
    return _writer.begin(Asn1UniversalType::BitString, false, asn1::impl::Asn1Writer::Order::Insertion, Byte{});
}

void DerEncoder::appendEncoded(const Asn1Node &node) {
    _writer.appendDer(node);
}

void DerEncoder::appendEncoded(const ByteBlock &bytes) {
    appendEncoded(DerParser::parseOrThrow(bytes, _writer.limits()));
}

void DerEncoder::appendSetOf(util::List<ByteBlock> children) {
    auto values = children.toStdVector();
    std::sort(values.begin(), values.end());
    const auto scope = beginSetOf();
    try {
        for (const auto &child : values) {
            appendEncoded(child);
        }
    } catch (...) {
        cancel(scope);
        throw;
    }
    end(scope);
}

void DerEncoder::appendBoolean(bool value) {
    auto byte = Byte{};
    if (value) {
        byte = Byte{0xffU};
    }
    _writer.primitive(Asn1UniversalType::Boolean, {&byte, 1U});
}

void DerEncoder::appendInteger(int64_t value) {
    _writer.integer(Asn1UniversalType::Integer, value);
}

void DerEncoder::appendEnumerated(int64_t value) {
    _writer.integer(Asn1UniversalType::Enumerated, value);
}

void DerEncoder::appendNull() {
    _writer.primitive(Asn1UniversalType::Null, {});
}

void DerEncoder::appendOctetString(mem::ConstByteSpan value) {
    _writer.primitive(Asn1UniversalType::OctetString, value);
}

void DerEncoder::appendPositiveInteger(const uint64_t value) {
    auto magnitude = ByteArray<sizeof(value)>{};
    for (auto index = ByteIndex::zero(); index.isWithin(magnitude.length()); index.uncheckedIncrement()) {
        const auto shift = static_cast<unsigned>((sizeof(value) - index.toSizeT() - 1U) * 8U);
        magnitude.set(index, Byte{static_cast<uint8_t>(value >> shift)});
    }
    appendPositiveInteger(magnitude.span());
}

void DerEncoder::appendPositiveInteger(const ConstByteSpan magnitude) {
    auto first = std::size_t{};
    while (first < magnitude.size() && magnitude[first].toUInt8() == 0U) {
        ++first;
    }
    std::optional<Byte> prefix;
    if (first == magnitude.size() || (magnitude[first].toUInt8() & 0x80U) != 0U) {
        prefix = Byte{};
    }
    _writer.primitive(Asn1UniversalType::Integer, magnitude.subspan(first), prefix);
}

void DerEncoder::appendObjectIdentifier(const Asn1ObjectIdentifier &value) {
    _writer.objectIdentifier(value);
}

void DerEncoder::appendUtf8String(const String &value) {
    if (!value.isEncodingValid()) {
        throw ParameterFailure{"A UTF8String must contain valid UTF-8."_el, "value"_el};
    }
    _writer.primitive(
        Asn1UniversalType::Utf8String, toConstByteSpan(text::impl::UnsafeU8StringAccess{value}.dataSpan()));
}

void DerEncoder::appendPrintableString(const String &value) {
    static const auto cPrintableCharacters = CharSet::fromPattern("- A-Za-z0-9'()+,./:=?"_el);
    if (!value.containsOnly(cPrintableCharacters)) {
        throw ParameterFailure{"A PrintableString contains an unsupported character."_el, "value"_el};
    }
    _writer.primitive(
        Asn1UniversalType::PrintableString, toConstByteSpan(text::impl::UnsafeU8StringAccess{value}.dataSpan()));
}

void DerEncoder::appendIa5String(const String &value) {
    static const auto cIa5Characters = CharSet::fromRange(Char{U'\0'}, Char{U'\x7f'});
    if (!value.containsOnly(cIa5Characters)) {
        throw ParameterFailure{"An IA5String can only contain ASCII characters."_el, "value"_el};
    }
    _writer.primitive(
        Asn1UniversalType::Ia5String, toConstByteSpan(text::impl::UnsafeU8StringAccess{value}.dataSpan()));
}

void DerEncoder::appendBitString(const ConstByteSpan content, const uint8_t unusedBits) {
    if (unusedBits > 7U || (content.empty() && unusedBits != 0U) ||
        (!content.empty() && unusedBits != 0U &&
            (content.back().toUInt8() &
                static_cast<uint8_t>((uint16_t{1U} << static_cast<unsigned>(unusedBits)) - uint16_t{1U})) != 0U)) {
        throw ParameterFailure{"Invalid unused-bit count or nonzero padding in DER BIT STRING."_el, "unusedBits"_el};
    }
    _writer.primitive(Asn1UniversalType::BitString, content, Byte{unusedBits});
}

void DerEncoder::appendTime(const time::Timestamp &value) {
    if (!value.isValid()) {
        throw ParameterFailure{"A DER time value must be valid."_el, "value"_el};
    }
    const auto parts = value.toDateTimeOrThrow().parts();
    const auto year = static_cast<uint16_t>(parts.year.toRawValue());
    auto bytes = ByteArray<15U>{};
    auto offset = ByteIndex::zero();
    const auto appendTwoDigits = [&bytes](const ByteIndex index, const uint16_t number) {
        bytes.set(index, Byte{static_cast<uint8_t>('0' + number / 10U)});
        bytes.set(index.incremented(), Byte{static_cast<uint8_t>('0' + number % 10U)});
    };
    if (year >= 1950U && year <= 2049U) {
        appendTwoDigits(offset, year % 100U);
        offset += ByteCount{2U};
    } else {
        appendTwoDigits(offset, year / 100U);
        appendTwoDigits(offset + ByteCount{2U}, year % 100U);
        offset += ByteCount{4U};
    }
    appendTwoDigits(offset, static_cast<uint16_t>(parts.month.toRawValue()));
    appendTwoDigits(offset + ByteCount{2U}, static_cast<uint16_t>(parts.day.toRawValue()));
    appendTwoDigits(offset + ByteCount{4U}, static_cast<uint16_t>(parts.hour.toRawValue()));
    appendTwoDigits(offset + ByteCount{6U}, static_cast<uint16_t>(parts.minute.toRawValue()));
    appendTwoDigits(offset + ByteCount{8U}, static_cast<uint16_t>(parts.second.toRawValue()));
    bytes.set(offset + ByteCount{10U}, Byte{static_cast<uint8_t>('Z')});
    auto length = ByteCount{15U};
    auto tag = Asn1UniversalType::GeneralizedTime;
    if (year >= 1950U && year <= 2049U) {
        length = ByteCount{13U};
        tag = Asn1UniversalType::UtcTime;
    }
    _writer.primitive(tag, bytes.span(ByteIndex::zero(), length));
}

void DerEncoder::appendImplicitPrimitive(const uint32_t number, const ConstByteSpan content) {
    _writer.primitive({Asn1TagClass::Context, number}, content);
}

}
