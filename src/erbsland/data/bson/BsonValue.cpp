// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BsonValue.hpp"

#include "impl/BsonCodec.hpp"
#include "impl/BsonValueData.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::data::bson {

using namespace text::literals;

BsonValue::BsonValue(bool value) : _data{std::make_shared<impl::BsonValueData>(value)} {
}

BsonValue::BsonValue(int32_t value) : _data{std::make_shared<impl::BsonValueData>(value)} {
}

BsonValue::BsonValue(int64_t value) : _data{std::make_shared<impl::BsonValueData>(value)} {
}

BsonValue::BsonValue(int8_t value) : BsonValue{static_cast<int32_t>(value)} {
}

BsonValue::BsonValue(uint8_t value) : BsonValue{static_cast<int32_t>(value)} {
}

BsonValue::BsonValue(int16_t value) : BsonValue{static_cast<int32_t>(value)} {
}

BsonValue::BsonValue(uint16_t value) : BsonValue{static_cast<int32_t>(value)} {
}

BsonValue::BsonValue(uint32_t value) : BsonValue{static_cast<int64_t>(value)} {
}

BsonValue::BsonValue(uint64_t value) :
    BsonValue{[value]() -> int64_t {
        if (value > INT64_MAX) {
            throw err::ParameterError{"BSON has no unsigned 64-bit integer type."_el, "value"_el};
        }
        return static_cast<int64_t>(value);
    }()} {
}

BsonValue::BsonValue(double value) : _data{std::make_shared<impl::BsonValueData>(value)} {
}

BsonValue::BsonValue(text::String value) : _data{std::make_shared<impl::BsonValueData>(std::move(value))} {
}

BsonValue::BsonValue(BsonBinary value) : _data{std::make_shared<impl::BsonValueData>(std::move(value))} {
}

BsonValue::BsonValue(mem::ByteBlock value) : BsonValue{BsonBinary{std::move(value), 0U}} {
}

BsonValue::BsonValue(time::Timestamp value) : _data{std::make_shared<impl::BsonValueData>(std::move(value))} {
}

BsonValue::BsonValue(BsonArray value) : _data{std::make_shared<impl::BsonValueData>(std::move(value))} {
}

BsonValue::BsonValue(BsonDocument value) : _data{std::make_shared<impl::BsonValueData>(std::move(value))} {
}

BsonValue::BsonValue(BsonOpaqueValue value) : _data{std::make_shared<impl::BsonValueData>(std::move(value))} {
}

auto BsonValue::type() const noexcept -> BsonType {
    if (!_data) {
        return BsonType::Null;
    }
    switch (_data->value.index()) {
    case 1:
        return BsonType::Bool;
    case 2:
        return BsonType::Int32;
    case 3:
        return BsonType::Int64;
    case 4:
        return BsonType::Double;
    case 5:
        return BsonType::Text;
    case 6:
        return BsonType::Binary;
    case 7:
        return BsonType::DateTime;
    case 8:
        return BsonType::Array;
    case 9:
        return BsonType::Document;
    case 10:
        return BsonType::Opaque;
    default:
        return BsonType::Null;
    }
}

auto BsonValue::itemCount() const noexcept -> unit::ItemCount {
    if (const auto value = getArray()) {
        return value->count();
    }
    if (const auto value = getDocument()) {
        return value->count();
    }
    return {};
}

auto BsonValue::get(unit::ItemIndex index) const -> BsonValue {
    const auto value = getArray();
    return value ? value->get(index) : BsonValue{};
}

auto BsonValue::get(const text::String &key) const -> BsonValue {
    const auto value = getDocument();
    return value ? value->get(key).value_or(BsonValue{}) : BsonValue{};
}

auto BsonValue::getOrThrow(unit::ItemIndex index) const -> BsonValue {
    const auto value = getArray();
    if (!value) {
        throw err::LogicError{"The BSON value is not an array."_el};
    }
    return value->getRefOrThrow(index);
}

auto BsonValue::getOrThrow(const text::String &key) const -> BsonValue {
    const auto value = getDocument();
    if (!value) {
        throw err::LogicError{"The BSON value is not a document."_el};
    }
    const auto result = value->get(key);
    if (!result) {
        throw err::OutOfRangeError{"The BSON document key is absent."_el};
    }
    return *result;
}

#define ERBSLAND_BSON_GETTER(NAME, TYPE)                                                                               \
    auto BsonValue::NAME() const noexcept -> std::optional<TYPE> {                                                     \
        if (!_data)                                                                                                    \
            return std::nullopt;                                                                                       \
        if (const auto *value = std::get_if<TYPE>(&_data->value))                                                      \
            return *value;                                                                                             \
        return std::nullopt;                                                                                           \
    }
ERBSLAND_BSON_GETTER(getBool, bool)
ERBSLAND_BSON_GETTER(getDouble, double)
ERBSLAND_BSON_GETTER(getText, text::String)
ERBSLAND_BSON_GETTER(getBinary, BsonBinary)
ERBSLAND_BSON_GETTER(getTimestamp, time::Timestamp)
ERBSLAND_BSON_GETTER(getArray, BsonArray)
ERBSLAND_BSON_GETTER(getDocument, BsonDocument)
ERBSLAND_BSON_GETTER(getOpaque, BsonOpaqueValue)
#undef ERBSLAND_BSON_GETTER

auto BsonValue::getInteger() const noexcept -> std::optional<int64_t> {
    if (!_data) {
        return std::nullopt;
    }
    if (const auto *value = std::get_if<int32_t>(&_data->value)) {
        return *value;
    }
    if (const auto *value = std::get_if<int64_t>(&_data->value)) {
        return *value;
    }
    return std::nullopt;
}

void BsonValue::detach() {
    if (_data.use_count() > 1) {
        _data = std::make_shared<impl::BsonValueData>(*_data);
    }
}

auto BsonValue::set(unit::ItemIndex index, BsonValue value) -> BsonValue & {
    if (!is(BsonType::Array)) {
        throw err::LogicError{"The BSON value is not an array."_el};
    }
    detach();
    auto &array = std::get<BsonArray>(_data->value);
    if (index.toRawValue() == array.count().toRawValue()) {
        array.append(std::move(value));
    } else if (index.isWithin(array.count())) {
        array.set(index, std::move(value));
    } else {
        throw err::OutOfRangeError{"The BSON array index exceeds the append position."_el};
    }
    return *this;
}

auto BsonValue::set(const text::String &key, BsonValue value) -> BsonValue & {
    if (!is(BsonType::Document)) {
        throw err::LogicError{"The BSON value is not a document."_el};
    }
    detach();
    std::get<BsonDocument>(_data->value).set(key, std::move(value));
    return *this;
}

auto BsonValue::append(BsonValue value) -> BsonValue & {
    return set(unit::ItemIndex{itemCount().toRawValue()}, std::move(value));
}

auto BsonValue::toByteBlock(BsonFormatOptions options) const -> mem::ByteBlock {
    return impl::BsonCodec::encode(*this, options);
}

auto BsonValue::fromByteBlock(const mem::ByteBlock &bytes, BsonParseOptions options) noexcept
    -> std::optional<BsonValue> {
    try {
        return fromByteBlockOrThrow(bytes, options);
    } catch (...) {
        return std::nullopt;
    }
}

auto BsonValue::fromByteBlockOrThrow(const mem::ByteBlock &bytes, BsonParseOptions options) -> BsonValue {
    return impl::BsonCodec::decode(bytes, options);
}

}
