// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CalendarDelta.hpp"

#include "impl/TimeDeltaFormatter.hpp"

#include "../err/OverflowError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::time {

using namespace text::literals;

auto CalendarDelta::operator+(CalendarDelta other) const noexcept -> CalendarDelta {
    auto result = *this;
    result += other;
    return result;
}

auto CalendarDelta::operator+=(CalendarDelta other) noexcept -> CalendarDelta & {
    _parts.nanoseconds += other._parts.nanoseconds;
    _parts.microseconds += other._parts.microseconds;
    _parts.milliseconds += other._parts.milliseconds;
    _parts.seconds += other._parts.seconds;
    _parts.minutes += other._parts.minutes;
    _parts.hours += other._parts.hours;
    _parts.days += other._parts.days;
    _parts.weeks += other._parts.weeks;
    _parts.months += other._parts.months;
    _parts.years += other._parts.years;
    return *this;
}

auto CalendarDelta::operator-(CalendarDelta other) const noexcept -> CalendarDelta {
    auto result = *this;
    result -= other;
    return result;
}

auto CalendarDelta::operator-=(CalendarDelta other) noexcept -> CalendarDelta & {
    return *this += -other;
}

auto CalendarDelta::operator-() const noexcept -> CalendarDelta {
    return CalendarDelta{Parts{
        .nanoseconds = -_parts.nanoseconds,
        .microseconds = -_parts.microseconds,
        .milliseconds = -_parts.milliseconds,
        .seconds = -_parts.seconds,
        .minutes = -_parts.minutes,
        .hours = -_parts.hours,
        .days = -_parts.days,
        .weeks = -_parts.weeks,
        .months = -_parts.months,
        .years = -_parts.years}};
}

auto CalendarDelta::isZero() const noexcept -> bool {
    return _parts.nanoseconds.isZero() && _parts.microseconds.isZero() && _parts.milliseconds.isZero() &&
        _parts.seconds.isZero() && _parts.minutes.isZero() && _parts.hours.isZero() && _parts.days.isZero() &&
        _parts.weeks.isZero() && _parts.months.isZero() && _parts.years.isZero();
}

auto CalendarDelta::toTimeDelta() const noexcept -> std::optional<TimeDelta> {
    if (!_parts.months.isZero() || !_parts.years.isZero()) {
        return {};
    }
    auto total = Nanoseconds{};
    const auto add = [&total](const auto amount) -> bool {
        if (amount.template wouldConvertSaturate<Nanoseconds>()) {
            return false;
        }
        const auto converted = amount.template converted<Nanoseconds>();
        if (total.toValue().wouldAddSaturate(converted.toValue())) {
            return false;
        }
        total += converted;
        return true;
    };
    if (!add(_parts.nanoseconds) || !add(_parts.microseconds) || !add(_parts.milliseconds) || !add(_parts.seconds) ||
        !add(_parts.minutes) || !add(_parts.hours) || !add(_parts.days) || !add(_parts.weeks)) {
        return {};
    }
    return TimeDelta{total};
}

auto CalendarDelta::isValidTimeDelta() const noexcept -> bool {
    return toTimeDelta().has_value();
}

auto CalendarDelta::toTimeDeltaOrThrow() const -> TimeDelta {
    if (const auto result = toTimeDelta(); result.has_value()) {
        return *result;
    }
    throw err::OverflowError{"CalendarDelta cannot be represented as a TimeDelta"};
}

auto CalendarDelta::toString(const TimeDeltaFormat &format) const -> text::String {
    auto result = text::StringEditor{};
    impl::appendCalendarDeltaPart(result, _parts.years.toRawValue(), "year"_el, "years"_el, "y"_el, "year"_el, format);
    impl::appendCalendarDeltaPart(
        result, _parts.months.toRawValue(), "month"_el, "months"_el, "mo"_el, "month"_el, format);
    const auto fixedText = impl::formatCalendarFixedDelta(
        _parts.nanoseconds,
        _parts.microseconds,
        _parts.milliseconds,
        _parts.seconds,
        _parts.minutes,
        _parts.hours,
        _parts.days,
        _parts.weeks,
        format,
        result.isEmpty());
    if (!fixedText.isEmpty()) {
        if (!result.isEmpty()) {
            result.append(format.unitSeparator());
        }
        result.append(fixedText);
    }
    return result;
}

auto CalendarDelta::setNanoseconds(const Nanoseconds value) noexcept -> CalendarDelta & {
    _parts.nanoseconds = value;
    return *this;
}

auto CalendarDelta::setMicroseconds(const Microseconds value) noexcept -> CalendarDelta & {
    _parts.microseconds = value;
    return *this;
}

auto CalendarDelta::setMilliseconds(const Milliseconds value) noexcept -> CalendarDelta & {
    _parts.milliseconds = value;
    return *this;
}

auto CalendarDelta::setSeconds(const Seconds value) noexcept -> CalendarDelta & {
    _parts.seconds = value;
    return *this;
}

auto CalendarDelta::setMinutes(const Minutes value) noexcept -> CalendarDelta & {
    _parts.minutes = value;
    return *this;
}

auto CalendarDelta::setHours(const Hours value) noexcept -> CalendarDelta & {
    _parts.hours = value;
    return *this;
}

auto CalendarDelta::setDays(const Days value) noexcept -> CalendarDelta & {
    _parts.days = value;
    return *this;
}

auto CalendarDelta::setWeeks(const Weeks value) noexcept -> CalendarDelta & {
    _parts.weeks = value;
    return *this;
}

auto CalendarDelta::setMonths(const Months value) noexcept -> CalendarDelta & {
    _parts.months = value;
    return *this;
}

auto CalendarDelta::setYears(const Years value) noexcept -> CalendarDelta & {
    _parts.years = value;
    return *this;
}

}
