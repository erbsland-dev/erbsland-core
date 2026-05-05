// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Day.hpp"
#include "Month.hpp"
#include "TimeAmounts.hpp"
#include "Year.hpp"

namespace erbsland::time {

/// A year and a zero-based day-of-year amount.
///
/// Used by calendar extraction helpers that split an epoch day count into the containing year and the remaining day
/// offset within that year.
/// @tested{TimeCalendarPartsTest::testYearCalendarHelpers}
struct YearDayOfYearParts {
    Year year;      ///< The extracted year.
    Days dayOfYear; ///< The zero-based day offset within `year`.

    friend auto operator==(const YearDayOfYearParts &, const YearDayOfYearParts &) noexcept -> bool = default;
};

/// A year and month pair.
///
/// Used by month navigation helpers that may cross a year boundary.
/// @tested{TimeCalendarPartsTest::testMonthCalendarHelpers}
struct YearMonthParts {
    Year year;   ///< The year of the month.
    Month month; ///< The month in `year`.

    friend auto operator==(const YearMonthParts &, const YearMonthParts &) noexcept -> bool = default;
};

/// A month and day pair.
///
/// Used by calendar extraction helpers that resolve a day-of-year value into month and day components.
/// @tested{TimeCalendarPartsTest::testMonthCalendarHelpers}
struct MonthDayParts {
    Month month; ///< The extracted month.
    Day day;     ///< The day within `month`.

    friend auto operator==(const MonthDayParts &, const MonthDayParts &) noexcept -> bool = default;
};

/// A calendar date split into named parts.
/// @tested{TimeCoreTest::testDateCommonHandling}
struct DateParts {
    Year year;   ///< The year component.
    Month month; ///< The month component.
    Day day;     ///< The day component.

    friend auto operator==(const DateParts &, const DateParts &) noexcept -> bool = default;
};

}
