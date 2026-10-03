// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BsonCodec.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../err/ParseError.hpp"
#include "../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <span>

namespace erbsland::data::bson::impl {

using namespace err;
using namespace mem;
using namespace text;
using namespace unit;
using namespace text::literals;

BsonCodec::BsonCodec(ByteBlock bytes, BsonParseOptions options) :
    _input{std::move(bytes)}, _reader{_input}, _parseOptions{options} {
}

BsonCodec::BsonCodec([[maybe_unused]] BsonFormatOptions options) {
}

auto BsonCodec::decode(const ByteBlock &bytes, BsonParseOptions options) -> BsonValue {
    return BsonCodec{bytes, options}.parse();
}

auto BsonCodec::encode(const BsonValue &value, BsonFormatOptions options) -> ByteBlock {
    if (!value.is(BsonType::Document)) {
        throw ParameterFailure{"A BSON root must be a document."_el, "value"_el};
    }
    auto codec = BsonCodec{options};
    codec.writeContainer(value, false);
    return codec._writer.toByteBlock();
}

auto BsonCodec::parse() -> BsonValue {
    if (_input.length() > _parseOptions.maximumInputLength()) {
        fail();
    }
    auto value = readContainer(false, {});
    if (!_reader.isAtEnd()) {
        fail();
    }
    return value;
}

auto BsonCodec::readPayloadBytes(const ByteCount length) -> ByteBlock {
    const auto value = _reader.readBytes(length);
    if (!value.has_value()) {
        fail();
    }
    return *value;
}

auto BsonCodec::readCString() -> String {
    const auto start = _reader.position();
    while (!_reader.isAtEnd()) {
        if (readInteger<uint8_t>() == 0U) {
            return text::impl::UnsafeU8StringBuffer{_input.slice(start, _reader.position().decremented()).span()}
                .takeString();
        }
        if (_reader.position().toRawValue() - start.toRawValue() > _parseOptions.maximumStringLength().toRawValue()) {
            fail();
        }
    }
    fail();
}

auto BsonCodec::readString() -> String {
    const auto length = readInteger<int32_t>();
    if (length < 1 || static_cast<uint64_t>(length) - 1U > _parseOptions.maximumStringLength().toRawValue() ||
        !_reader.canRead(static_cast<std::size_t>(length))) {
        fail();
    }
    const auto bytes = readPayloadBytes(ByteCount::fromInteger(static_cast<uint64_t>(length - 1)));
    const auto value = text::impl::UnsafeU8StringBuffer{bytes.span()}.takeString();
    if (readInteger<uint8_t>() != 0U) {
        fail();
    }
    return value;
}

auto BsonCodec::readDateTime() -> time::Timestamp {
    const auto milliseconds = readInteger<int64_t>();
    const auto epoch = time::Timestamp::epoch(time::TimeEpoch::Posix).toTicksOrThrow<time::Milliseconds>();
    const auto core = epoch.toRawValue();
    if ((milliseconds > 0 && core > INT64_MAX - milliseconds) ||
        (milliseconds < 0 && core < INT64_MIN - milliseconds)) {
        fail();
    }
    const auto result = time::Timestamp::fromTicks(time::Milliseconds{core + milliseconds});
    if (!result) {
        fail();
    }
    return *result;
}

auto BsonCodec::readContainer(bool array, ItemCount depth) -> BsonValue {
    if (depth >= _parseOptions.maximumNesting()) {
        fail();
    }
    const auto start = _reader.position();
    const auto length = readInteger<int32_t>();
    if (length < 5 || static_cast<uint64_t>(length) > _parseOptions.maximumInputLength().toRawValue() ||
        static_cast<uint64_t>(length) > _input.length().toRawValue() - start.toRawValue()) {
        fail();
    }
    const auto end = start.toRawValue() + static_cast<uint64_t>(length);
    auto document = BsonDocument{};
    auto values = BsonArray{};
    while (_reader.position().toRawValue() < end - 1U) {
        if (_valueCount >= _parseOptions.maximumValueCount()) {
            fail();
        }
        ++_valueCount;
        const auto typeCode = readInteger<uint8_t>();
        if (typeCode == 0U) {
            fail();
        }
        const auto key = readCString();
        if (array) {
            if (key != String::fromInteger(values.count().toRawValue())) {
                fail();
            }
            values.append(readValue(typeCode, depth + ItemCount::one()));
        } else {
            if (document.contains(key)) {
                fail();
            }
            document.set(key, readValue(typeCode, depth + ItemCount::one()));
        }
        if (_reader.position().toRawValue() > end - 1U) {
            fail();
        }
    }
    if (_reader.position().toRawValue() != end - 1U || readInteger<uint8_t>() != 0U) {
        fail();
    }
    return array ? BsonValue{std::move(values)} : BsonValue{std::move(document)};
}

auto BsonCodec::readValue(uint8_t code, ItemCount depth) -> BsonValue {
    switch (code) {
    case 0x01U:
        return BsonValue{std::bit_cast<double>(readInteger<uint64_t>())};
    case 0x02U:
        return BsonValue{readString()};
    case 0x03U:
        return readContainer(false, depth);
    case 0x04U:
        return readContainer(true, depth);
    case 0x05U: {
        const auto length = readInteger<int32_t>();
        if (length < 0 || static_cast<uint64_t>(length) > _parseOptions.maximumInputLength().toRawValue()) {
            fail();
        }
        const auto subtype = readInteger<uint8_t>();
        if (subtype == 2U) {
            if (length < 4) {
                fail();
            }
            const auto actual = readInteger<int32_t>();
            if (actual != length - 4) {
                fail();
            }
            return BsonValue{
                BsonBinary{readPayloadBytes(ByteCount::fromInteger(static_cast<uint64_t>(actual))), subtype}};
        }
        return BsonValue{BsonBinary{readPayloadBytes(ByteCount::fromInteger(static_cast<uint64_t>(length))), subtype}};
    }
    case 0x08U: {
        const auto value = readInteger<uint8_t>();
        if (value > 1U) {
            fail();
        }
        return BsonValue{value == 1U};
    }
    case 0x09U:
        return BsonValue{readDateTime()};
    case 0x0aU:
        return {};
    case 0x10U:
        return BsonValue{readInteger<int32_t>()};
    case 0x12U:
        return BsonValue{readInteger<int64_t>()};
    default:
        break;
    }
    const auto start = _reader.position();
    switch (code) {
    case 0x06U:
    case 0x7fU:
    case 0xffU:
        break;
    case 0x07U:
        if (!_reader.canRead(12U)) {
            fail();
        }
        _reader.advance(12U);
        break;
    case 0x0bU:
        (void)readCString();
        (void)readCString();
        break;
    case 0x0cU:
        (void)readString();
        if (!_reader.canRead(12U)) {
            fail();
        }
        _reader.advance(12U);
        break;
    case 0x0dU:
    case 0x0eU:
        (void)readString();
        break;
    case 0x0fU: {
        const auto total = readInteger<int32_t>();
        if (total < 14 || static_cast<uint64_t>(total) > _parseOptions.maximumInputLength().toRawValue()) {
            fail();
        }
        (void)readString();
        (void)readContainer(false, depth);
        if (_reader.position().toRawValue() - start.toRawValue() != static_cast<uint64_t>(total)) {
            fail();
        }
        break;
    }
    case 0x11U:
        if (!_reader.canRead(8U)) {
            fail();
        }
        _reader.advance(8U);
        break;
    case 0x13U:
        if (!_reader.canRead(16U)) {
            fail();
        }
        _reader.advance(16U);
        break;
    default:
        fail();
    }
    if (_reader.position().toRawValue() > _input.length().toRawValue()) {
        fail();
    }
    return BsonValue{BsonOpaqueValue{code, _input.slice(start, _reader.position())}};
}

auto BsonCodec::typeCode(const BsonValue &value) -> uint8_t {
    switch (value.type()) {
    case BsonType::Double:
        return 0x01U;
    case BsonType::Text:
        return 0x02U;
    case BsonType::Document:
        return 0x03U;
    case BsonType::Array:
        return 0x04U;
    case BsonType::Binary:
        return 0x05U;
    case BsonType::Bool:
        return 0x08U;
    case BsonType::DateTime:
        return 0x09U;
    case BsonType::Null:
        return 0x0aU;
    case BsonType::Int32:
        return 0x10U;
    case BsonType::Int64:
        return 0x12U;
    case BsonType::Opaque:
        return value.getOpaque()->typeCode();
    }
    return 0U;
}

void BsonCodec::writeCString(const String &value) {
    const auto bytes = text::impl::UnsafeU8StringAccess{value}.dataSpan();
    if (std::find(bytes.begin(), bytes.end(), '\0') != bytes.end()) {
        throw ParameterFailure{"BSON keys cannot contain NUL."_el, "key"_el};
    }
    _writer.writeBytes(std::as_bytes(bytes));
    _writer.writeUInt8(0U);
}

void BsonCodec::writeString(const String &value) {
    const auto bytes = StringConverter{value}.toStdString();
    if (bytes.size() >= static_cast<std::size_t>(INT32_MAX)) {
        throw ParameterFailure{"BSON string is too long."_el, "value"_el};
    }
    _writer.writeInt32(static_cast<int32_t>(bytes.size() + 1U));
    _writer.writeBytes(std::span<const std::byte>{reinterpret_cast<const std::byte *>(bytes.data()), bytes.size()});
    _writer.writeUInt8(0U);
}

void BsonCodec::writeDateTime(const time::Timestamp &value) {
    if (!value.isValid() || value.time().nanosecondFraction().toRawValue() % 1'000'000 != 0) {
        throw ParameterFailure{"BSON datetime requires millisecond precision."_el, "value"_el};
    }
    const auto millis = value.toTicksOrThrow<time::Milliseconds>();
    const auto epoch = time::Timestamp::epoch(time::TimeEpoch::Posix).toTicksOrThrow<time::Milliseconds>();
    _writer.writeInt64(millis.toRawValue() - epoch.toRawValue());
}

void BsonCodec::writeContainer(const BsonValue &value, bool array) {
    const auto start = _writer.position();
    _writer.writeInt32(0);
    if (array) {
        const auto values = value.getArray().value();
        auto index = uint64_t{};
        for (const auto &entry : values) {
            _writer.writeUInt8(typeCode(entry));
            writeCString(String::fromInteger(index));
            writeValue(entry);
            ++index;
        }
    } else {
        const auto values = value.getDocument().value();
        for (const auto &[key, entry] : values) {
            _writer.writeUInt8(typeCode(entry));
            writeCString(key);
            writeValue(entry);
        }
    }
    _writer.writeUInt8(0U);
    const auto end = _writer.position();
    const auto length = end.toRawValue() - start.toRawValue();
    if (length > INT32_MAX) {
        throw ParameterFailure{"BSON document is too long."_el, "value"_el};
    }
    _writer.setPosition(start);
    _writer.writeInt32(static_cast<int32_t>(length));
    _writer.setPosition(end);
}

void BsonCodec::writeValue(const BsonValue &value) {
    switch (value.type()) {
    case BsonType::Null:
        break;
    case BsonType::Bool:
        _writer.writeUInt8(value.getBool().value() ? 1U : 0U);
        break;
    case BsonType::Int32:
        _writer.writeInt32(static_cast<int32_t>(value.getInteger().value()));
        break;
    case BsonType::Int64:
        _writer.writeInt64(value.getInteger().value());
        break;
    case BsonType::Double:
        _writer.writeUInt64(std::bit_cast<uint64_t>(value.getDouble().value()));
        break;
    case BsonType::Text:
        writeString(value.getText().value());
        break;
    case BsonType::Binary: {
        const auto binary = value.getBinary().value();
        const auto size = binary.bytes.length().toRawValue();
        if (size > INT32_MAX - 4U) {
            throw ParameterFailure{"BSON binary payload is too long."_el, "value"_el};
        }
        _writer.writeInt32(static_cast<int32_t>(size + (binary.subtype == 2U ? 4U : 0U)));
        _writer.writeUInt8(binary.subtype);
        if (binary.subtype == 2U) {
            _writer.writeInt32(static_cast<int32_t>(size));
        }
        _writer.writeBytes(binary.bytes);
        break;
    }
    case BsonType::DateTime:
        writeDateTime(value.getTimestamp().value());
        break;
    case BsonType::Array:
        writeContainer(value, true);
        break;
    case BsonType::Document:
        writeContainer(value, false);
        break;
    case BsonType::Opaque:
        _writer.writeBytes(value.getOpaque()->payload());
        break;
    }
}

[[noreturn]] void BsonCodec::fail() {
    throw ParseError{"Malformed or unsupported BSON data."_el};
}

}
