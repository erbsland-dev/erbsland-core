// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithConvertibleType.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the `time::CalendarDelta` type.
class CalendarDeltaValue final : public ValueWithConvertibleType<time::CalendarDelta, ValueType::TimeDelta> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto toTextRepresentation() const noexcept -> text::String override {
        return _value.toString(time::TimeDeltaFormat::elcl());
    }
    [[nodiscard]] auto asCalendarDelta() const noexcept -> time::CalendarDelta override { return _value; }
    [[nodiscard]] auto asCalendarDeltaOrThrow() const -> time::CalendarDelta override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<CalendarDeltaValue>(_value); }
};

}
