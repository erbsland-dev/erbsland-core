// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value.hpp"

#include "../../../text/StringFormat.hpp"

/// A generic template to implement value for native types.
namespace erbsland::conf::impl {

using namespace text::literals;

template <typename StorageType, ValueType::Enum tValueType>
class ValueWithNativeType : public Value {
public:
    template <typename Fwd>
    explicit ValueWithNativeType(Fwd value) : _value{std::forward<Fwd>(value)} {}

public:
    [[nodiscard]] auto type() const noexcept -> ValueType override { return tValueType; }
    [[nodiscard]] auto toTextRepresentation() const noexcept -> text::String override {
        if constexpr (std::is_same_v<StorageType, text::String>) {
            return _value;
        } else {
            return text::StringFormat{"{}"_el}.build(_value);
        }
    }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override {
        return std::make_shared<ValueWithNativeType<StorageType, tValueType>>(_value);
    }
    [[nodiscard]] auto rawStorage() const noexcept -> const StorageType & { return _value; }

protected:
    StorageType _value;
};

/// The value implementation for the `Boolean` type.
class BooleanValue final : public ValueWithNativeType<bool, ValueType::Boolean> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asBoolean() const noexcept -> bool override { return _value; }
    [[nodiscard]] auto asBooleanOrThrow() const -> bool override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<BooleanValue>(_value); }
};

/// The value implementation for the `Float` type.
class FloatValue final : public ValueWithNativeType<Float, ValueType::Float> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asFloat() const noexcept -> Float override { return _value; }
    [[nodiscard]] auto asFloatOrThrow() const -> Float override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<FloatValue>(_value); }
};

/// The value implementation for the `Integer` type.
class IntegerValue final : public ValueWithNativeType<Integer, ValueType::Integer> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asInteger() const noexcept -> Integer override { return _value; }
    [[nodiscard]] auto asIntegerOrThrow() const -> Integer override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<IntegerValue>(_value); }
};

/// The value implementation for the `Text` type.
class TextValue final : public ValueWithNativeType<text::String, ValueType::Text> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asText() const noexcept -> text::String override { return _value; }
    [[nodiscard]] auto asTextOrThrow() const -> text::String override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<TextValue>(_value); }
};

}
