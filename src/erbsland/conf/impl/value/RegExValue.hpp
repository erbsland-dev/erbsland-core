// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithConvertibleType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `re::RegExPtr` type.
class RegExValue final : public ValueWithConvertibleType<re::RegExPtr, ValueType::RegEx> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asRegEx() const noexcept -> re::RegExPtr override { return _value; }
    [[nodiscard]] auto asRegExOrThrow() const -> re::RegExPtr override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<RegExValue>(_value); }
};

}
