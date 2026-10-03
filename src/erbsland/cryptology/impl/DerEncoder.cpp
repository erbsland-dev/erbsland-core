// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DerEncoder.hpp"

#include "Asn1ObjectIdentifierCodec.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/OutOfRangeFailure.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../time/Timestamp.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../util/List.hpp"

#include <algorithm>

namespace erbsland::cryptology::impl {

using namespace err;
using namespace mem;
using namespace text;
using namespace text::literals;
using namespace unit;

constexpr auto cMaximumDerLength = ByteCount{16U * 1024U * 1024U};
constexpr auto cReservedLengthBytes = ByteCount{5U};

void DerEncoder::markAsSensitive() noexcept {
    _data.markAsSensitive();
}

void DerEncoder::appendEncoded(const ConstByteSpan value) {
    if (ByteCount::fromSizeT(value.size()) > cMaximumDerLength ||
        _data.length() > cMaximumDerLength - ByteCount::fromSizeT(value.size())) {
        throw OutOfRangeFailure{"DER content exceeds the fixed writer limit."_el};
    }
    _data.append(value);
}

auto DerEncoder::encoded() const -> ByteBlock {
    if (_depth != 0U) {
        throw LogicFailure{"Cannot access DER output with unclosed values."_el};
    }
    return ByteBlock{_data};
}

auto DerEncoder::beginSequence() -> Scope {
    return beginValue(0x30U);
}

auto DerEncoder::beginSet() -> Scope {
    return beginValue(0x31U);
}

auto DerEncoder::beginExplicit(const uint8_t tagNumber) -> Scope {
    if (tagNumber > 30U) {
        throw ParameterFailure{"Only low-tag-number DER context tags are supported."_el, "tagNumber"_el};
    }
    return beginValue(static_cast<uint8_t>(0xa0U | tagNumber));
}

auto DerEncoder::beginOctetString() -> Scope {
    return beginValue(0x04U);
}

auto DerEncoder::beginBitString() -> Scope {
    auto result = beginValue(0x03U);
    _data.append(Byte{});
    return result;
}

void DerEncoder::end(const Scope scope) {
    if (scope._depth != _depth || scope._contentIndex > _data.endIndex()) {
        throw LogicFailure{"DER scopes must be closed in reverse order."_el};
    }
    const auto contentLength = _data.length() - scope._contentIndex.distanceFromZero();
    if (contentLength > cMaximumDerLength) {
        throw OutOfRangeFailure{"DER content exceeds the fixed writer limit."_el};
    }
    auto lengthBytes = ByteArray<cReservedLengthBytes.toSizeT()>{};
    auto encodedLength = ByteCount::one();
    if (contentLength < ByteCount{128U}) {
        lengthBytes.set(ByteIndex::zero(), Byte{static_cast<uint8_t>(contentLength.toRawValue())});
    } else {
        auto remaining = contentLength.toRawValue();
        auto octetCount = ByteCount{};
        while (remaining != 0U) {
            lengthBytes.set(
                ByteIndex::end(cReservedLengthBytes - ByteCount::one() - octetCount),
                Byte{static_cast<uint8_t>(remaining)});
            remaining >>= 8U;
            ++octetCount;
        }
        encodedLength = octetCount + ByteCount::one();
        const auto first = ByteIndex::end(cReservedLengthBytes - encodedLength);
        lengthBytes.set(first, Byte{static_cast<uint8_t>(0x80U | octetCount.toRawValue())});
        if (first != ByteIndex::zero()) {
            for (auto index = ByteIndex::zero(); index.isWithin(encodedLength); index.uncheckedIncrement()) {
                lengthBytes.set(index, lengthBytes.get(first + index.distanceFromZero()));
            }
        }
    }
    _data.replace(
        ByteRange{scope._lengthIndex, cReservedLengthBytes}, lengthBytes.span(ByteIndex::zero(), encodedLength));
    --_depth;
}

void DerEncoder::appendSet(util::List<ByteBlock> children) {
    auto values = children.toStdVector();
    std::sort(values.begin(), values.end());
    const auto scope = beginSet();
    for (const auto &value : values) {
        appendEncoded(value.span());
    }
    end(scope);
}

void DerEncoder::appendBoolean(const bool value) {
    const auto content = ByteArray<1U>{Byte{value ? uint8_t{0xffU} : uint8_t{0U}}};
    appendValue(0x01U, content.span());
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
    const auto scope = beginValue(0x02U);
    if (first == magnitude.size()) {
        _data.append(Byte{});
    } else {
        if ((magnitude[first].toUInt8() & 0x80U) != 0U) {
            _data.append(Byte{});
        }
        _data.append(magnitude.subspan(first));
    }
    end(scope);
}

void DerEncoder::appendNull() {
    appendValue(0x05U, {});
}

void DerEncoder::appendObjectIdentifier(const Asn1ObjectIdentifier &value) {
    const auto scope = beginValue(0x06U);
    Asn1ObjectIdentifierCodec::encode(value, _data);
    end(scope);
}

void DerEncoder::appendUtf8String(const String &value) {
    if (!value.isEncodingValid()) {
        throw ParameterFailure{"A UTF8String must contain valid UTF-8."_el, "value"_el};
    }
    appendValue(0x0cU, toConstByteSpan(text::impl::UnsafeU8StringAccess{value}.dataSpan()));
}

void DerEncoder::appendPrintableString(const String &value) {
    static const auto cPrintableCharacters = CharSet::fromPattern("- A-Za-z0-9'()+,./:=?"_el);
    if (!value.containsOnly(cPrintableCharacters)) {
        throw ParameterFailure{"A PrintableString contains an unsupported character."_el, "value"_el};
    }
    appendValue(0x13U, toConstByteSpan(text::impl::UnsafeU8StringAccess{value}.dataSpan()));
}

void DerEncoder::appendIa5String(const String &value) {
    static const auto cIa5Characters = CharSet::fromRange(Char{U'\0'}, Char{U'\x7f'});
    if (!value.containsOnly(cIa5Characters)) {
        throw ParameterFailure{"An IA5String can only contain ASCII characters."_el, "value"_el};
    }
    appendValue(0x16U, toConstByteSpan(text::impl::UnsafeU8StringAccess{value}.dataSpan()));
}

void DerEncoder::appendOctetString(const ConstByteSpan content) {
    appendValue(0x04U, content);
}

void DerEncoder::appendBitString(const ConstByteSpan content, const uint8_t unusedBits) {
    validateBitString(content, unusedBits);
    const auto scope = beginValue(0x03U);
    _data.append(Byte{unusedBits});
    _data.append(content);
    end(scope);
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
    auto tag = uint8_t{0x18U};
    if (year >= 1950U && year <= 2049U) {
        length = ByteCount{13U};
        tag = 0x17U;
    }
    appendValue(tag, bytes.span(ByteIndex::zero(), length));
}

void DerEncoder::appendImplicitPrimitive(const uint8_t tagNumber, const ConstByteSpan content) {
    if (tagNumber > 30U) {
        throw ParameterFailure{"Only low-tag-number DER context tags are supported."_el, "tagNumber"_el};
    }
    appendValue(static_cast<uint8_t>(0x80U | tagNumber), content);
}

auto DerEncoder::beginValue(const uint8_t tag) -> Scope {
    if (_data.length() > cMaximumDerLength - cReservedLengthBytes - ByteCount::one()) {
        throw OutOfRangeFailure{"DER content exceeds the fixed writer limit."_el};
    }
    _data.append(Byte{tag});
    const auto lengthIndex = _data.endIndex();
    _data.append(Byte{}, cReservedLengthBytes);
    ++_depth;
    return Scope{lengthIndex, _data.endIndex(), _depth};
}

void DerEncoder::appendValue(const uint8_t tag, const ConstByteSpan content) {
    const auto scope = beginValue(tag);
    appendEncoded(content);
    end(scope);
}

void DerEncoder::validateBitString(const ConstByteSpan content, const uint8_t unusedBits) const {
    if (unusedBits > 7U || (content.empty() && unusedBits != 0U) ||
        (!content.empty() && unusedBits != 0U &&
            (content.back().toUInt8() &
                static_cast<uint8_t>((uint16_t{1U} << static_cast<unsigned>(unusedBits)) - uint16_t{1U})) != 0U)) {
        throw ParameterFailure{"Invalid unused-bit count or nonzero padding in DER BIT STRING."_el, "unusedBits"_el};
    }
}

}
