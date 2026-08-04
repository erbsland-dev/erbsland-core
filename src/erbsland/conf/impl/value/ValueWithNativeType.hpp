// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value.hpp"

#include "../../../text/StringFormat.hpp"

/// A generic template to implement value for native types.
namespace erbsland::conf::impl {

using namespace text::literals;

/// Value implementation that stores a native value type.
template <typename StorageType, ValueType::Enum tValueType>
class ValueWithNativeType : public Value {
public:
    /// Store a native value using perfect forwarding.
    /// @tparam Fwd The forwarded value type.
    /// @param value The value to retain.
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
    /// Get the native stored value.
    [[nodiscard]] auto rawStorage() const noexcept -> const StorageType & { return _value; }

protected:
    StorageType _value;
};

}
