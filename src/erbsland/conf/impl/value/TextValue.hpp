// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithNativeType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `Text` type.
class TextValue final : public ValueWithNativeType<text::String, ValueType::Text> {
public:
    using ValueWithNativeType::ValueWithNativeType;
    [[nodiscard]] auto asText() const noexcept -> text::String override { return _value; }
    [[nodiscard]] auto asTextOrThrow() const -> text::String override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<TextValue>(_value); }
};

}
