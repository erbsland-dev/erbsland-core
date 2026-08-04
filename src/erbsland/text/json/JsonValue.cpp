// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "JsonValue.hpp"

#include "impl/JsonParser.hpp"
#include "impl/JsonValueData.hpp"
#include "impl/JsonWriter.hpp"

#include "../Literals.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParseError.hpp"

#include <cmath>
#include <limits>

namespace erbsland::text::json {

using namespace text::literals;

JsonValue::JsonValue(const bool value) : _data{std::make_shared<impl::JsonValueData>(value)} {
}

JsonValue::JsonValue(const int64_t value) : _data{std::make_shared<impl::JsonValueData>(value)} {
}

JsonValue::JsonValue(const double value) {
    if (!std::isfinite(value)) {
        throw err::ParameterError{"A JSON number must be finite."_el, "value"_el};
    }
    _data = std::make_shared<impl::JsonValueData>(value);
}

JsonValue::JsonValue(String value) : _data{std::make_shared<impl::JsonValueData>(std::move(value))} {
}

JsonValue::JsonValue(JsonArray value) : _data{std::make_shared<impl::JsonValueData>(std::move(value))} {
}

JsonValue::JsonValue(JsonObject value) : _data{std::make_shared<impl::JsonValueData>(std::move(value))} {
}

auto JsonValue::type() const noexcept -> JsonType {
    if (_data == nullptr || std::holds_alternative<std::monostate>(_data->value)) {
        return JsonType::Null;
    }
    if (std::holds_alternative<bool>(_data->value)) {
        return JsonType::Bool;
    }
    if (std::holds_alternative<int64_t>(_data->value) || std::holds_alternative<double>(_data->value)) {
        return JsonType::Number;
    }
    if (std::holds_alternative<String>(_data->value)) {
        return JsonType::Text;
    }
    if (std::holds_alternative<JsonArray>(_data->value)) {
        return JsonType::Array;
    }
    return JsonType::Object;
}

auto JsonValue::is(const JsonType expected) const noexcept -> bool {
    return type() == expected;
}

auto JsonValue::isPrimitive() const noexcept -> bool {
    return type() != JsonType::Array && type() != JsonType::Object;
}

auto JsonValue::itemCount() const noexcept -> unit::ItemCount {
    if (const auto value = getArray(); value.has_value()) {
        return value->count();
    }
    if (const auto value = getObject(); value.has_value()) {
        return value->count();
    }
    return {};
}

auto JsonValue::get(const unit::ItemIndex index) const -> JsonValue {
    const auto array = getArray();
    return array.has_value() ? array->get(index) : JsonValue{};
}

auto JsonValue::getOrThrow(const unit::ItemIndex index) const -> JsonValue {
    const auto array = getArray();
    if (!array.has_value()) {
        throw err::LogicError{"The JSON value is not an array."_el};
    }
    return array->getRefOrThrow(index);
}

auto JsonValue::get(const String &key) const -> JsonValue {
    const auto object = getObject();
    if (!object.has_value()) {
        return {};
    }
    return object->get(key).value_or(JsonValue{});
}

auto JsonValue::getOrThrow(const String &key) const -> JsonValue {
    const auto object = getObject();
    if (!object.has_value()) {
        throw err::LogicError{"The JSON value is not an object."_el};
    }
    const auto result = object->get(key);
    if (!result.has_value()) {
        throw err::OutOfRangeError{"The JSON object key does not exist."_el};
    }
    return *result;
}

auto JsonValue::getBool() const noexcept -> std::optional<bool> {
    if (_data == nullptr) {
        return {};
    }
    if (const auto value = std::get_if<bool>(&_data->value)) {
        return *value;
    }
    return {};
}

auto JsonValue::getBool(const bool fallback) const noexcept -> bool {
    return getBool().value_or(fallback);
}

auto JsonValue::getBoolOrThrow() const -> bool {
    const auto result = getBool();
    if (!result.has_value()) {
        throwTypeError();
    }
    return *result;
}

auto JsonValue::getNumber() const noexcept -> std::optional<double> {
    if (_data == nullptr) {
        return {};
    }
    if (const auto value = std::get_if<double>(&_data->value)) {
        return *value;
    }
    if (const auto value = std::get_if<int64_t>(&_data->value)) {
        return static_cast<double>(*value);
    }
    return {};
}

auto JsonValue::getNumber(const double fallback) const noexcept -> double {
    return getNumber().value_or(fallback);
}

auto JsonValue::getNumberOrThrow() const -> double {
    const auto result = getNumber();
    if (!result.has_value()) {
        throwTypeError();
    }
    return *result;
}

auto JsonValue::getText() const noexcept -> std::optional<String> {
    if (_data == nullptr) {
        return {};
    }
    if (const auto value = std::get_if<String>(&_data->value)) {
        return *value;
    }
    return {};
}

auto JsonValue::getText(String fallback) const noexcept -> String {
    return getText().value_or(std::move(fallback));
}

auto JsonValue::getTextOrThrow() const -> String {
    const auto result = getText();
    if (!result.has_value()) {
        throwTypeError();
    }
    return *result;
}

auto JsonValue::set(const unit::ItemIndex index, JsonValue value) -> JsonValue & {
    if (type() != JsonType::Array) {
        throw err::LogicError{"The JSON value is not an array."_el};
    }
    detach();
    auto &array = std::get<JsonArray>(_data->value);
    if (index.toRawValue() == array.count().toRawValue()) {
        array.append(std::move(value));
    } else if (index.isWithin(array.count())) {
        array.set(index, std::move(value));
    } else {
        throw err::OutOfRangeError{"The JSON array index exceeds the append position."_el};
    }
    return *this;
}

auto JsonValue::set(const String &key, JsonValue value) -> JsonValue & {
    if (type() != JsonType::Object) {
        throw err::LogicError{"The JSON value is not an object."_el};
    }
    detach();
    std::get<JsonObject>(_data->value).set(key, std::move(value));
    return *this;
}

auto JsonValue::append(JsonValue value) -> JsonValue & {
    return set(unit::ItemIndex{itemCount().toRawValue()}, std::move(value));
}

auto JsonValue::toString(const JsonFormatOptions options) const -> String {
    return impl::JsonWriter{*this, options}.write();
}

auto JsonValue::fromString(const String &text, const JsonParseOptions options) noexcept -> std::optional<JsonValue> {
    try {
        return fromStringOrThrow(text, options);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto JsonValue::fromStringOrThrow(const String &text, const JsonParseOptions options) -> JsonValue {
    return impl::JsonParser{text, options}.parse();
}

auto JsonValue::getInteger() const noexcept -> std::optional<int64_t> {
    if (_data == nullptr) {
        return {};
    }
    if (const auto value = std::get_if<int64_t>(&_data->value)) {
        return *value;
    }
    if (const auto value = std::get_if<double>(&_data->value)) {
        if (std::isfinite(*value) && std::trunc(*value) == *value &&
            *value >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
            *value < -static_cast<double>(std::numeric_limits<int64_t>::min())) {
            return static_cast<int64_t>(*value);
        }
    }
    return {};
}

auto JsonValue::getArray() const noexcept -> std::optional<JsonArray> {
    if (_data != nullptr) {
        if (const auto value = std::get_if<JsonArray>(&_data->value)) {
            return *value;
        }
    }
    return {};
}

auto JsonValue::getObject() const noexcept -> std::optional<JsonObject> {
    if (_data != nullptr) {
        if (const auto value = std::get_if<JsonObject>(&_data->value)) {
            return *value;
        }
    }
    return {};
}

void JsonValue::throwTypeError() {
    throw err::LogicError{"The JSON value has an incompatible type."_el};
}

void JsonValue::detach() {
    if (_data.use_count() > 1) {
        _data = std::make_shared<impl::JsonValueData>(*_data);
    }
}

}
