// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CborCodec.hpp"

#include "../../../err/Exception.hpp"
#include "../../../err/ParseError.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>

namespace erbsland::data::cbor::impl {

using namespace text::literals;

CborCodec::CborCodec(mem::ByteBlock bytes, CborParseOptions options) :
    _input{std::move(bytes)}, _reader{_input}, _parseOptions{options} {
    _reader.setEndianness(mem::Endianness::Big);
    _writer.setEndianness(mem::Endianness::Big);
}

CborCodec::CborCodec(CborFormatOptions options) : _formatOptions{options} {
    _writer.setEndianness(mem::Endianness::Big);
}

auto CborCodec::decode(const mem::ByteBlock &bytes, CborParseOptions options) -> CborValue {
    try {
        return CborCodec{bytes, options}.parse();
    } catch (const err::ParseError &) {
        throw;
    } catch (const err::Exception &) {
        fail();
    }
}

auto CborCodec::encode(const CborValue &value, CborFormatOptions options) -> mem::ByteBlock {
    auto codec = CborCodec{options};
    codec.writeValue(value);
    return codec._writer.toByteBlock();
}

auto CborCodec::parse() -> CborValue {
    if (_input.length() > _parseOptions.maximumInputLength()) {
        fail();
    }
    auto value = readValue({});
    if (!_reader.isAtEnd()) {
        fail();
    }
    return value;
}

auto CborCodec::readArgument(uint8_t additional, bool allowIndefinite) -> uint64_t {
    if (additional < 24U) {
        return additional;
    }
    if (additional == 31U && allowIndefinite && !_parseOptions.isDagCbor()) {
        return std::numeric_limits<uint64_t>::max();
    }
    uint64_t value{};
    if (additional == 24U) {
        value = _reader.readUInt8OrThrow();
    } else if (additional == 25U) {
        value = _reader.readUInt16OrThrow();
    } else if (additional == 26U) {
        value = _reader.readUInt32OrThrow();
    } else if (additional == 27U) {
        value = _reader.readUInt64OrThrow();
    } else {
        fail();
    }
    if (_parseOptions.isDagCbor()) {
        if ((additional == 24U && value < 24U) || (additional == 25U && value <= UINT8_MAX) ||
            (additional == 26U && value <= UINT16_MAX) || (additional == 27U && value <= UINT32_MAX)) {
            fail();
        }
    }
    return value;
}

auto CborCodec::readBytes(uint8_t additional, uint8_t major) -> mem::ByteBlock {
    const auto length = readArgument(additional, true);
    if (length != UINT64_MAX) {
        if (length > _parseOptions.maximumInputLength().toRawValue() ||
            !_reader.canRead(static_cast<std::size_t>(length))) {
            fail();
        }
        return _reader.readBytesOrThrow(unit::ByteLength{length});
    }
    const auto start = _writer.length();
    while (true) {
        if (_reader.isAtEnd()) {
            fail();
        }
        if (_reader.peekByte().toUInt8() == 0xffU) {
            _reader.advance(1U);
            break;
        }
        const auto head = _reader.readUInt8OrThrow();
        if ((head >> 5U) != major || (head & 31U) == 31U) {
            fail();
        }
        const auto partLength = readArgument(static_cast<uint8_t>(head & 31U));
        if (partLength > _parseOptions.maximumInputLength().toRawValue() ||
            !_reader.canRead(static_cast<std::size_t>(partLength))) {
            fail();
        }
        _writer.writeBytes(_reader.readBytesOrThrow(unit::ByteLength{partLength}));
        if (_writer.length() > _parseOptions.maximumInputLength()) {
            fail();
        }
    }
    return _writer.toByteBlock().slice(unit::ByteIndex{start.toRawValue()}, _writer.length() - start);
}

auto CborCodec::bytesToText(const mem::ByteBlock &bytes) -> text::String {
    const auto span = bytes.span();
    return text::String{std::string_view{reinterpret_cast<const char *>(span.data()), span.size()}};
}

auto CborCodec::readText(uint8_t additional) -> text::String {
    const auto bytes = readBytes(additional, 3U);
    if (bytes.length() > _parseOptions.maximumStringLength()) {
        fail();
    }
    auto value = bytesToText(bytes);
    if (_parseOptions.isDagCbor() && !value.isValidUtf8()) {
        fail();
    }
    return value;
}

auto CborCodec::readArray(uint8_t additional, unit::ItemCount depth) -> CborValue {
    if (depth >= _parseOptions.maximumNesting()) {
        fail();
    }
    const auto count = readArgument(additional, true);
    if (count != UINT64_MAX && count > _parseOptions.maximumValueCount().toRawValue()) {
        fail();
    }
    auto array = CborArray{};
    for (uint64_t i = 0; count == UINT64_MAX || i < count; ++i) {
        if (count == UINT64_MAX && _reader.peekByte().toUInt8() == 0xffU) {
            _reader.advance(1U);
            break;
        }
        if (_reader.isAtEnd()) {
            fail();
        }
        array.append(readValue(depth + unit::ItemCount::one()));
    }
    return CborValue{std::move(array)};
}

auto CborCodec::readMap(uint8_t additional, unit::ItemCount depth) -> CborValue {
    if (depth >= _parseOptions.maximumNesting()) {
        fail();
    }
    const auto count = readArgument(additional, true);
    if (count != UINT64_MAX && count > _parseOptions.maximumValueCount().toRawValue()) {
        fail();
    }
    auto map = CborMap{};
    auto previous = mem::ByteBlock{};
    for (uint64_t i = 0; count == UINT64_MAX || i < count; ++i) {
        if (count == UINT64_MAX && _reader.peekByte().toUInt8() == 0xffU) {
            _reader.advance(1U);
            break;
        }
        if (_reader.isAtEnd()) {
            fail();
        }
        const auto start = _reader.position();
        const auto keyValue = readValue(depth + unit::ItemCount::one());
        if (!keyValue.is(CborType::Text)) {
            fail();
        }
        const auto key = keyValue.getText().value();
        if (map.contains(key)) {
            fail();
        }
        if (_parseOptions.isDagCbor()) {
            const auto encoded = _input.slice(start, _reader.position());
            if (!previous.isEmpty()) {
                const auto left = previous.span();
                const auto right = encoded.span();
                if (!std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end())) {
                    fail();
                }
            }
            previous = encoded;
        }
        map.set(key, readValue(depth + unit::ItemCount::one()));
    }
    return CborValue{std::move(map)};
}

auto CborCodec::readTag(uint8_t additional, unit::ItemCount depth) -> CborValue {
    const auto tag = readArgument(additional);
    if (tag == 42U) {
        const auto value = readValue(depth);
        if (!value.is(CborType::Bytes)) {
            fail();
        }
        const auto bytes = value.getBytes().value();
        if (bytes.isEmpty() || bytes.span()[0].toUInt8() != 0U ||
            !validCid(bytes.slice(unit::ByteIndex{1U}, bytes.length() - unit::ByteLength{1U}))) {
            fail();
        }
        return CborValue{CborLink{bytes.slice(unit::ByteIndex{1U}, bytes.length() - unit::ByteLength{1U})}};
    }
    if (_parseOptions.isDagCbor()) {
        fail();
    }
    if (tag == 0U) {
        const auto value = readValue(depth);
        if (!value.is(CborType::Text)) {
            fail();
        }
        const auto date = time::DateTime::fromIsoString(value.getText().value());
        if (!date.isValid()) {
            fail();
        }
        return CborValue{date};
    }
    if (tag == 1U) {
        const auto value = readValue(depth);
        int64_t seconds{};
        auto nanoseconds = uint32_t{};
        if (const auto signedValue = value.getSigned()) {
            seconds = *signedValue;
        } else if (const auto unsignedValue = value.getUnsigned(); unsignedValue && *unsignedValue <= INT64_MAX) {
            seconds = static_cast<int64_t>(*unsignedValue);
        } else if (const auto floatValue = value.getFloat()) {
            if (!std::isfinite(*floatValue)) {
                fail();
            }
            const auto whole = std::floor(*floatValue);
            if (whole < static_cast<double>(INT64_MIN) || whole >= static_cast<double>(INT64_MAX)) {
                fail();
            }
            seconds = static_cast<int64_t>(whole);
            const auto fraction = *floatValue - whole;
            const auto nanos = std::round(fraction * 1'000'000'000.0);
            if (nanos >= 1'000'000'000.0) {
                if (seconds == INT64_MAX) {
                    fail();
                }
                ++seconds;
            } else {
                nanoseconds = static_cast<uint32_t>(nanos);
                if (static_cast<double>(nanoseconds) / 1'000'000'000.0 != fraction) {
                    fail();
                }
            }
        } else {
            fail();
        }
        const auto epoch = time::DateTime::epoch(time::TimeEpoch::Posix).toSecondsOrThrow();
        if ((seconds < 0 && seconds < -epoch.toRawValue()) ||
            (seconds > 0 && epoch.toRawValue() > INT64_MAX - seconds)) {
            fail();
        }
        const auto date = time::DateTime::fromTicks(
            time::Seconds{epoch.toRawValue() + seconds}, time::Nanoseconds{static_cast<int64_t>(nanoseconds)});
        if (!date) {
            fail();
        }
        return CborValue{*date};
    }
    fail();
}

auto CborCodec::halfToDouble(uint16_t bits) noexcept -> double {
    const auto sign = (bits & 0x8000U) != 0U ? -1.0 : 1.0;
    const auto exponent = (bits >> 10U) & 31U;
    const auto mantissa = bits & 1023U;
    if (exponent == 31U) {
        return mantissa == 0U ? sign * std::numeric_limits<double>::infinity()
                              : std::numeric_limits<double>::quiet_NaN();
    }
    if (exponent == 0U) {
        return sign * std::ldexp(static_cast<double>(mantissa), -24);
    }
    return sign * std::ldexp(static_cast<double>(mantissa + 1024U), static_cast<int>(exponent) - 25);
}

auto CborCodec::readSimple(uint8_t additional) -> CborValue {
    if (additional == 20U) {
        return CborValue{false};
    }
    if (additional == 21U) {
        return CborValue{true};
    }
    if (additional == 22U) {
        return {};
    }
    if (additional == 25U || additional == 26U || additional == 27U) {
        if (_parseOptions.isDagCbor() && additional != 27U) {
            fail();
        }
        double value{};
        if (additional == 25U) {
            value = halfToDouble(_reader.readUInt16OrThrow());
        } else if (additional == 26U) {
            value = static_cast<double>(std::bit_cast<float>(_reader.readUInt32OrThrow()));
        } else {
            value = std::bit_cast<double>(_reader.readUInt64OrThrow());
        }
        if (_parseOptions.isDagCbor() && (!std::isfinite(value) || (value == 0.0 && std::signbit(value)))) {
            fail();
        }
        return CborValue{value};
    }
    fail();
}

auto CborCodec::readValue(unit::ItemCount depth) -> CborValue {
    if (_reader.isAtEnd() || _valueCount >= _parseOptions.maximumValueCount()) {
        fail();
    }
    ++_valueCount;
    const auto head = _reader.readUInt8OrThrow();
    const auto major = static_cast<uint8_t>(head >> 5U);
    const auto additional = static_cast<uint8_t>(head & 31U);
    switch (major) {
    case 0U:
        return CborValue{readArgument(additional)};
    case 1U: {
        const auto magnitude = readArgument(additional);
        if (magnitude > INT64_MAX) {
            fail();
        }
        return CborValue{-1 - static_cast<int64_t>(magnitude)};
    }
    case 2U:
        return CborValue{readBytes(additional, major)};
    case 3U:
        return CborValue{readText(additional)};
    case 4U:
        return readArray(additional, depth);
    case 5U:
        return readMap(additional, depth);
    case 6U:
        return readTag(additional, depth);
    case 7U:
        return readSimple(additional);
    default:
        fail();
    }
}

auto CborCodec::validCid(const mem::ByteBlock &bytes) noexcept -> bool {
    const auto data = bytes.span();
    if (data.size() < 1U) {
        return false;
    }
    auto position = std::size_t{};
    // CIDv0 is exactly a SHA-256 multihash: code 0x12, length 0x20, then 32 digest bytes.
    if (data[position].toUInt8() == 0x12U) {
        return data.size() == 34U && data[position + 1U].toUInt8() == 0x20U;
    }
    const auto readVarint = [&]() noexcept -> std::optional<uint64_t> {
        auto value = uint64_t{};
        for (auto shift = unsigned{}; shift < 64U; shift += 7U) {
            if (position >= data.size()) {
                return std::nullopt;
            }
            const auto octet = data[position++].toUInt8();
            if (shift == 63U && (octet & 0x7fU) > 1U) {
                return std::nullopt;
            }
            value |= static_cast<uint64_t>(octet & 0x7fU) << shift;
            if ((octet & 0x80U) == 0U) {
                if (shift != 0U && (octet & 0x7fU) == 0U) {
                    return std::nullopt;
                }
                return value;
            }
        }
        return std::nullopt;
    };
    const auto version = readVarint();
    const auto codec = readVarint();
    const auto hashCode = readVarint();
    const auto digestLength = readVarint();
    return version == 1U && codec && *codec != 0U && hashCode && *hashCode != 0U && digestLength &&
        *digestLength == data.size() - position;
}

[[noreturn]] void CborCodec::fail() {
    throw err::ParseError{"Malformed or unsupported CBOR data."_el};
}

}
