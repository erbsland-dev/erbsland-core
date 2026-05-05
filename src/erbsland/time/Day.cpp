// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Day.hpp"

#include "CalendarParts.hpp"
#include "Month.hpp"
#include "Year.hpp"

namespace erbsland::time {

auto Day::exists(const Year year, const Month month) const noexcept -> bool {
    return toAmount() < month.dayCount(year);
}

auto Day::isLast(const Year year, const Month month) const noexcept -> bool {
    return toAmount() == month.dayCount(year) - Days::one();
}

auto Day::hasNext(const Year year, const Month month) const noexcept -> bool {
    return !(year.isLast() && month.isLast() && isLast(year, month));
}

auto Day::hasPrevious(const Year year, const Month month) const noexcept -> bool {
    return !(year.isFirst() && month.isFirst() && isFirst());
}

auto Day::clamped(const Year year, const Month month) const noexcept -> Day {
    const auto lastValidDay = last(year, month);
    if (*this <= lastValidDay) {
        return *this;
    }
    return lastValidDay;
}

auto Day::last(const Year year, const Month month) noexcept -> Day {
    return fromAmount(month.dayCount(year) - Days::one());
}

auto Day::next(Year year, Month month) const noexcept -> DateParts {
    if (!isLast(year, month)) {
        return {.year = year, .month = month, .day = incremented()};
    }
    if (month.hasNext(year)) {
        const auto nextMonthParts = month.next(year);
        return {.year = nextMonthParts.year, .month = nextMonthParts.month, .day = Day::first()};
    }
    return {.year = year, .month = month, .day = *this};
}

auto Day::previous(Year year, Month month) const noexcept -> DateParts {
    if (!isFirst()) {
        return {.year = year, .month = month, .day = decremented()};
    }
    if (month.hasPrevious(year)) {
        const auto previousMonthParts = month.previous(year);
        return {
            .year = previousMonthParts.year,
            .month = previousMonthParts.month,
            .day = last(previousMonthParts.year, previousMonthParts.month)};
    }
    return {.year = year, .month = month, .day = *this};
}

}
