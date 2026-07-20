// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value.hpp"

#include "../../../re/RegEx.hpp"

namespace erbsland::conf::impl {

/// A generic base class for Core scalar types with configuration text rendering.
template <typename StorageType, ValueType::Enum tValueType>
class ValueWithConvertibleType : public Value {
public:
    template <typename FwdValue>
    explicit ValueWithConvertibleType(FwdValue value) : _value{std::forward<FwdValue>(value)} {}

public:
    [[nodiscard]] auto type() const noexcept -> ValueType override { return tValueType; }
    [[nodiscard]] auto toTextRepresentation() const noexcept -> text::String override {
        if constexpr (std::is_same_v<StorageType, re::RegExPtr>) {
            return _value->pattern().toString();
        } else {
            return _value.toString();
        }
    }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override {
        return std::make_shared<ValueWithConvertibleType<StorageType, tValueType>>(_value);
    }
    [[nodiscard]] auto rawStorage() const noexcept -> const StorageType & { return _value; }

protected:
    StorageType _value;
};

/// The value implementation for the `time::DateTime` type.
class DateTimeValue final : public ValueWithConvertibleType<time::DateTime, ValueType::DateTime> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asDateTime() const noexcept -> time::DateTime override { return _value; }
    [[nodiscard]] auto asDateTimeOrThrow() const -> time::DateTime override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<DateTimeValue>(_value); }
};

/// The value implementation for the `time::Date` type.
class DateValue final : public ValueWithConvertibleType<time::Date, ValueType::Date> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asDate() const noexcept -> time::Date override { return _value; }
    [[nodiscard]] auto asDateOrThrow() const -> time::Date override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<DateValue>(_value); }
};

/// The value implementation for a floating `time::Time`.
class TimeValue final : public ValueWithConvertibleType<time::Time, ValueType::Time> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asTime() const noexcept -> time::Time override { return _value; }
    [[nodiscard]] auto asTimeOrThrow() const -> time::Time override { return _value; }
    [[nodiscard]] auto asTimeWithZone() const noexcept -> time::TimeWithZone override {
        return time::TimeWithZone{_value, time::TimeZone::local()};
    }
    [[nodiscard]] auto asTimeWithZoneOrThrow() const -> time::TimeWithZone override { return asTimeWithZone(); }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<TimeValue>(_value); }
};

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

/// The value implementation for the `re::RegExPtr` type.
class RegExValue final : public ValueWithConvertibleType<re::RegExPtr, ValueType::RegEx> {
public:
    using ValueWithConvertibleType::ValueWithConvertibleType;
    [[nodiscard]] auto asRegEx() const noexcept -> re::RegExPtr override { return _value; }
    [[nodiscard]] auto asRegExOrThrow() const -> re::RegExPtr override { return _value; }
    [[nodiscard]] auto deepCopy() const -> ValuePtr override { return std::make_shared<RegExValue>(_value); }
};

}
