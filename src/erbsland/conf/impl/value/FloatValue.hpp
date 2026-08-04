// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithNativeType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `Float` type.
class FloatValue final : public ValueWithNativeType<Float, ValueType::Float> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asFloat() const noexcept -> Float override { return _value; }
    [[nodiscard]] auto asFloatOrThrow() const -> Float override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<FloatValue>(_value); }
};

}
