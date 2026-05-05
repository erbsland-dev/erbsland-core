// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Month.hpp"

#include "CalendarParts.hpp"
#include "Day.hpp"
#include "DayOfYear.hpp"
#include "Year.hpp"

#include <array>

namespace erbsland::time {

auto Month::dayCount(const Year year) const noexcept -> Days {
    constexpr auto days = std::array<int, 13>{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const auto monthIndex = toValue().toSizeT();
    return Days{days[monthIndex]} + (*this == february() && year.isLeapYear() ? Days::one() : Days::zero());
}

auto Month::lastDay(const Year year) const noexcept -> Day {
    return Day::fromAmount(dayCount(year) - Days::one());
}

auto Month::firstDayOfYear(const Year year) const noexcept -> DayOfYear {
    return DayOfYear::fromAmount(year.daysBeforeMonth(*this));
}

auto Month::lastDayOfYear(const Year year) const noexcept -> DayOfYear {
    return DayOfYear::fromAmount(year.daysBeforeMonth(*this) + dayCount(year) - Days::one());
}

auto Month::minimumDayCount() const noexcept -> Days {
    if (*this == february()) {
        return Days{28};
    }
    if (*this == april() || *this == june() || *this == september() || *this == november()) {
        return Days{30};
    }
    return Days{31};
}

auto Month::maximumDayCount() const noexcept -> Days {
    return hasFixedLength() ? minimumDayCount() : Days{29};
}

auto Month::next(Year year) const noexcept -> YearMonthParts {
    if (!isLast()) {
        return {.year = year, .month = incremented()};
    }
    if (year.hasNext()) {
        return {.year = year.next(), .month = first()};
    }
    return {.year = year, .month = *this};
}

auto Month::previous(const Year year) const noexcept -> YearMonthParts {
    if (!isFirst()) {
        return {.year = year, .month = decremented()};
    }
    if (year.hasPrevious()) {
        return {.year = year.previous(), .month = last()};
    }
    return {.year = year, .month = *this};
}

auto Month::hasNext(const Year year) const noexcept -> bool {
    return !isLast() || year.hasNext();
}

auto Month::hasPrevious(const Year year) const noexcept -> bool {
    return !isFirst() || year.hasPrevious();
}

auto Month::extractMonthAndDay(const Year year, const Days days) noexcept -> MonthDayParts {
    const auto month = year.monthOfDay(DayOfYear::fromAmount(days));
    const auto daysBeforeMonth = year.daysBeforeMonth(month);
    const auto day = Day::fromAmount(days - daysBeforeMonth);
    return {.month = month, .day = day};
}

auto Month::extractMonthAndDay(Year year, DayOfYear dayOfYear) noexcept -> MonthDayParts {
    return extractMonthAndDay(year, dayOfYear.toAmount());
}

}
