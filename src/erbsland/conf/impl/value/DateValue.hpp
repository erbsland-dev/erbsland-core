// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithConvertibleType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `time::Date` type.
class DateValue final : public ValueWithConvertibleType<time::Date, ValueType::Date> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asDate() const noexcept -> time::Date override { return _value; }
    [[nodiscard]] auto asDateOrThrow() const -> time::Date override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<DateValue>(_value); }
};

}
