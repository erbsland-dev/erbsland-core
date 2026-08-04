// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithNativeType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `Boolean` type.
class BooleanValue final : public ValueWithNativeType<bool, ValueType::Boolean> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asBoolean() const noexcept -> bool override { return _value; }
    [[nodiscard]] auto asBooleanOrThrow() const -> bool override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<BooleanValue>(_value); }
};

}
