// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CborValue.hpp"

#include "impl/CborCodec.hpp"
#include "impl/CborValueData.hpp"

#include "../../err/LogicFailure.hpp"
#include "../../err/NotFoundError.hpp"
#include "../../err/OutOfRangeFailure.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::data::cbor {

using namespace err;
using namespace mem;
using namespace text;
using namespace unit;
using namespace text::literals;

CborValue::CborValue(bool value) : _data{std::make_shared<impl::CborValueData>(value)} {
}

CborValue::CborValue(int64_t value) : _data{std::make_shared<impl::CborValueData>(value)} {
}

CborValue::CborValue(uint64_t value) : _data{std::make_shared<impl::CborValueData>(value)} {
}

CborValue::CborValue(double value) : _data{std::make_shared<impl::CborValueData>(value)} {
}

CborValue::CborValue(String value) : _data{std::make_shared<impl::CborValueData>(std::move(value))} {
}

CborValue::CborValue(ByteBlock value) : _data{std::make_shared<impl::CborValueData>(std::move(value))} {
}

CborValue::CborValue(CborArray value) : _data{std::make_shared<impl::CborValueData>(std::move(value))} {
}

CborValue::CborValue(CborMap value) : _data{std::make_shared<impl::CborValueData>(std::move(value))} {
}

CborValue::CborValue(time::Timestamp value) : _data{std::make_shared<impl::CborValueData>(std::move(value))} {
}

CborValue::CborValue(CborLink value) : _data{std::make_shared<impl::CborValueData>(std::move(value))} {
}

auto CborValue::type() const noexcept -> CborType {
    if (!_data) {
        return CborType::Null;
    }
    switch (_data->value.index()) {
    case 1:
        return CborType::Bool;
    case 2:
        return CborType::Signed;
    case 3:
        return CborType::Unsigned;
    case 4:
        return CborType::Float;
    case 5:
        return CborType::Text;
    case 6:
        return CborType::Bytes;
    case 7:
        return CborType::Array;
    case 8:
        return CborType::Map;
    case 9:
        return CborType::DateTime;
    case 10:
        return CborType::Link;
    default:
        return CborType::Null;
    }
}

auto CborValue::itemCount() const noexcept -> ItemCount {
    if (const auto value = getArray()) {
        return value->count();
    }
    if (const auto value = getMap()) {
        return value->count();
    }
    return {};
}

auto CborValue::get(ItemIndex index) const -> CborValue {
    const auto value = getArray();
    return value ? value->get(index) : CborValue{};
}

auto CborValue::get(const String &key) const -> CborValue {
    const auto value = getMap();
    return value ? value->get(key).value_or(CborValue{}) : CborValue{};
}

auto CborValue::getOrThrow(ItemIndex index) const -> CborValue {
    const auto value = getArray();
    if (!value) {
        throw LogicFailure{"The CBOR value is not an array."_el};
    }
    return value->getRefOrThrow(index);
}

auto CborValue::getOrThrow(const String &key) const -> CborValue {
    const auto value = getMap();
    if (!value) {
        throw LogicFailure{"The CBOR value is not a map."_el};
    }
    const auto result = value->get(key);
    if (!result) {
        throw NotFoundError{"The CBOR map key is absent."_el};
    }
    return *result;
}

#define ERBSLAND_CBOR_GETTER(NAME, TYPE)                                                                               \
    auto CborValue::NAME() const noexcept -> std::optional<TYPE> {                                                     \
        if (!_data)                                                                                                    \
            return std::nullopt;                                                                                       \
        if (const auto *value = std::get_if<TYPE>(&_data->value))                                                      \
            return *value;                                                                                             \
        return std::nullopt;                                                                                           \
    }
ERBSLAND_CBOR_GETTER(getBool, bool)
ERBSLAND_CBOR_GETTER(getFloat, double)
ERBSLAND_CBOR_GETTER(getText, String)
ERBSLAND_CBOR_GETTER(getBytes, ByteBlock)
ERBSLAND_CBOR_GETTER(getTimestamp, time::Timestamp)
ERBSLAND_CBOR_GETTER(getArray, CborArray)
ERBSLAND_CBOR_GETTER(getMap, CborMap)
#undef ERBSLAND_CBOR_GETTER

auto CborValue::getSigned() const noexcept -> std::optional<int64_t> {
    if (!_data) {
        return std::nullopt;
    }
    if (const auto *value = std::get_if<int64_t>(&_data->value)) {
        return *value;
    }
    if (const auto *value = std::get_if<uint64_t>(&_data->value); value && *value <= INT64_MAX) {
        return static_cast<int64_t>(*value);
    }
    return std::nullopt;
}

auto CborValue::getUnsigned() const noexcept -> std::optional<uint64_t> {
    if (!_data) {
        return std::nullopt;
    }
    if (const auto *value = std::get_if<uint64_t>(&_data->value)) {
        return *value;
    }
    if (const auto *value = std::get_if<int64_t>(&_data->value); value && *value >= 0) {
        return static_cast<uint64_t>(*value);
    }
    return std::nullopt;
}

auto CborValue::getLinkBytes() const noexcept -> std::optional<ByteBlock> {
    if (!_data) {
        return std::nullopt;
    }
    if (const auto *value = std::get_if<CborLink>(&_data->value)) {
        return value->cid;
    }
    return std::nullopt;
}

void CborValue::detach() {
    if (_data.use_count() > 1) {
        _data = std::make_shared<impl::CborValueData>(*_data);
    }
}

auto CborValue::set(ItemIndex index, CborValue value) -> CborValue & {
    if (!is(CborType::Array)) {
        throw LogicFailure{"The CBOR value is not an array."_el};
    }
    detach();
    auto &array = std::get<CborArray>(_data->value);
    if (index.toRawValue() == array.count().toRawValue()) {
        array.append(std::move(value));
    } else if (index.isWithin(array.count())) {
        array.set(index, std::move(value));
    } else {
        throw OutOfRangeFailure{"The CBOR array index exceeds the append position."_el};
    }
    return *this;
}

auto CborValue::set(const String &key, CborValue value) -> CborValue & {
    if (!is(CborType::Map)) {
        throw LogicFailure{"The CBOR value is not a map."_el};
    }
    detach();
    std::get<CborMap>(_data->value).set(key, std::move(value));
    return *this;
}

auto CborValue::append(CborValue value) -> CborValue & {
    if (!is(CborType::Array)) {
        throw LogicFailure{"The CBOR value is not an array."_el};
    }
    detach();
    std::get<CborArray>(_data->value).append(std::move(value));
    return *this;
}

auto CborValue::toByteBlock(CborFormatOptions options) const -> ByteBlock {
    return impl::CborCodec::encode(*this, options);
}

auto CborValue::fromByteBlock(const ByteBlock &bytes, CborParseOptions options) noexcept -> std::optional<CborValue> {
    try {
        return fromByteBlockOrThrow(bytes, options);
    } catch (const ParseError &) {
        return std::nullopt;
    }
}

auto CborValue::fromByteBlockOrThrow(const ByteBlock &bytes, CborParseOptions options) -> CborValue {
    return impl::CborCodec::decode(bytes, options);
}

}
