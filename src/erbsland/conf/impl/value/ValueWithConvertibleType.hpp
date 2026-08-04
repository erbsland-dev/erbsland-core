// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value.hpp"

#include "../../../re/RegEx.hpp"

namespace erbsland::conf::impl {

/// A generic base class for Core scalar types with configuration text rendering.
template <typename StorageType, ValueType::Enum tValueType>
class ValueWithConvertibleType : public Value {
public:
    /// Store a convertible value using perfect forwarding.
    /// @tparam FwdValue The forwarded value type.
    /// @param value The value to retain.
    template <typename FwdValue>
    explicit ValueWithConvertibleType(FwdValue value) : _value{std::forward<FwdValue>(value)} {}

public:
    [[nodiscard]] auto type() const noexcept -> ValueType override { return tValueType; }
    [[nodiscard]] auto toTextRepresentation() const noexcept -> text::String override {
        if constexpr (std::is_same_v<StorageType, re::RegExPtr>) {
            return _value->pattern().toString();
        } else {
            return _value.toString();
        }
    }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override {
        return std::make_shared<ValueWithConvertibleType<StorageType, tValueType>>(_value);
    }
    /// Access the stored value without conversion or copying.
    [[nodiscard]] auto rawStorage() const noexcept -> const StorageType & { return _value; }

protected:
    StorageType _value;
};

}
