// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithConvertibleType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `time::DateTime` type.
class DateTimeValue final : public ValueWithConvertibleType<time::DateTime, ValueType::DateTime> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asDateTime() const noexcept -> time::DateTime override { return _value; }
    [[nodiscard]] auto asDateTimeOrThrow() const -> time::DateTime override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<DateTimeValue>(_value); }
};

}
