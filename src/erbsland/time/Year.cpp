// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Year.hpp"

#include "CalendarParts.hpp"
#include "Day.hpp"
#include "DayOfYear.hpp"
#include "Month.hpp"

#include <algorithm>

namespace erbsland::time {

namespace {
constexpr auto cDaysPer400Years = Days{146097};
constexpr auto cDaysPer100Years = Days{36524};
constexpr auto cDaysPer4Years = Days{1461};
constexpr auto cLastValidDay = Days{3652424};
}

auto Year::isLeapYear() const noexcept -> bool {
    const auto year = toValue();
    return ((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0;
}

auto Year::dayCount() const noexcept -> Days {
    return isLeapYear() ? Days{366} : Days{365};
}

auto Year::daysSinceEpoch() const noexcept -> Days {
    auto days = Days{0};
    auto year = toValue();
    const auto cycles400 = year / 400;
    year %= 400;
    days += cDaysPer400Years * cycles400;
    if (year > 0) {
        days += Days::one();
    }
    const auto cycles100 = year / 100;
    year %= 100;
    days += cDaysPer100Years * cycles100;
    if (year > 0) {
        days -= Days::one();
    }
    const auto cycles4 = year / 4;
    year %= 4;
    days += cDaysPer4Years * cycles4;
    if (year > 0) {
        days += Days::one();
    }
    days += Days{365} * year;
    return days;
}

auto Year::next() const noexcept -> Year {
    return hasNext() ? incremented() : *this;
}

auto Year::previous() const noexcept -> Year {
    return hasPrevious() ? decremented() : *this;
}

auto Year::extractFromEpoch(Days days) noexcept -> YearDayOfYearParts {
    const static auto maximumValue = YearDayOfYearParts{.year = Year{9999}, .dayOfYear = Days{365}};
    if (days <= Days::zero()) {
        return {.year = Year{0}, .dayOfYear = Days{0}};
    }
    if (days >= cLastValidDay) {
        return maximumValue;
    }
    const auto years400 = Years{days.toValue() / cDaysPer400Years.toValue()};
    days %= cDaysPer400Years.toValue();
    if (days < Days{366}) {
        return {.year = Year::fromAmount(years400 * 400), .dayOfYear = days};
    }
    if (years400 >= Years{25}) {
        return maximumValue;
    }
    days -= Days::one();
    const auto years100 = Years{days.toValue() / cDaysPer100Years.toValue()};
    days %= cDaysPer100Years.toValue();
    if (days < Days{365}) {
        return {.year = Year::fromAmount(years400 * 400 + years100 * 100), .dayOfYear = days};
    }
    days += Days::one();
    const auto years4 = Years{days.toValue() / cDaysPer4Years.toValue()};
    days %= cDaysPer4Years.toValue();
    auto year = years400 * 400 + years100 * 100 + years4 * 4;
    if (days >= Days{366}) {
        days -= Days::one();
        year += Years{days.toValue() / 365};
        days %= 365;
    }
    return {.year = Year::fromAmount(year), .dayOfYear = days};
}

auto Year::startDayArray() const noexcept -> const StartDayArray & {
    static constexpr auto regular = StartDayArray{
        Days{0},
        Days{31},
        Days{59},
        Days{90},
        Days{120},
        Days{151},
        Days{181},
        Days{212},
        Days{243},
        Days{273},
        Days{304},
        Days{334},
        Days{365}};
    static constexpr auto leap = StartDayArray{
        Days{0},
        Days{31},
        Days{60},
        Days{91},
        Days{121},
        Days{152},
        Days{182},
        Days{213},
        Days{244},
        Days{274},
        Days{305},
        Days{335},
        Days{366}};
    return isLeapYear() ? leap : regular;
}

auto Year::daysBeforeMonth(const Month month) const noexcept -> Days {
    return startDayArray()[month.toAmount().toValue().toSizeT()];
}

auto Year::monthOfDay(const DayOfYear dayOfYear) const noexcept -> Month {
    const auto &dayArray = startDayArray();
    const auto dayValue = dayOfYear.toAmount();
    const auto foundMonth = std::ranges::upper_bound(dayArray, dayValue);
    const auto months = Months{std::ranges::distance(dayArray.begin(), foundMonth) - 1};
    return Month::fromAmount(months);
}

}
