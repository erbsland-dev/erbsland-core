// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithConvertibleType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for a `time::TimeWithZone`.
class TimeWithZoneValue final : public ValueWithConvertibleType<time::TimeWithZone, ValueType::Time> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asTime() const noexcept -> time::Time override { return _value.time(); }
    [[nodiscard]] auto asTimeOrThrow() const -> time::Time override { return _value.time(); }
    [[nodiscard]] auto asTimeWithZone() const noexcept -> time::TimeWithZone override { return _value; }
    [[nodiscard]] auto asTimeWithZoneOrThrow() const -> time::TimeWithZone override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<TimeWithZoneValue>(_value); }
};

}
