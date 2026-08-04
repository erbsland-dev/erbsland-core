// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithNativeType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `Integer` type.
class IntegerValue final : public ValueWithNativeType<Integer, ValueType::Integer> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asInteger() const noexcept -> Integer override { return _value; }
    [[nodiscard]] auto asIntegerOrThrow() const -> Integer override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<IntegerValue>(_value); }
};

}
