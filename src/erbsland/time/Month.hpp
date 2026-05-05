// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeAmounts.hpp"

#include "impl/TimePartBases.hpp"

namespace erbsland::time {

class Day;
class DayOfYear;
class Year;
struct MonthDayParts;
struct YearMonthParts;

/// A month in the Gregorian calendar, range `1..12`.
///
/// Provides clamped arithmetic, month-aware navigation and calendar computation methods.
/// @tested{MonthTest}
class Month final : public impl::MonthBase {
    using Base = impl::MonthBase;

public:
    using Base::Base;
    /// Return the number of days in this month of `year`.
    /// @param year The year to check.
    /// @return The day count (28-31).
    [[nodiscard]] auto dayCount(Year year) const noexcept -> Days;
    /// Return the last day in this month of `year`.
    /// @param year The year to check.
    /// @return The last valid day.
    [[nodiscard]] auto lastDay(Year year) const noexcept -> Day;
    /// Return the first day-of-year for this month of `year`.
    /// @param year The year to check.
    /// @return The one-based day-of-year of the first day.
    [[nodiscard]] auto firstDayOfYear(Year year) const noexcept -> DayOfYear;
    /// Return the last day-of-year for this month of `year`.
    /// @param year The year to check.
    /// @return The one-based day-of-year of the last day.
    [[nodiscard]] auto lastDayOfYear(Year year) const noexcept -> DayOfYear;
    /// Return the minimum number of days this month can have.
    /// @return 28 for February, 30 for April/June/September/November, 31 otherwise.
    [[nodiscard]] auto minimumDayCount() const noexcept -> Days;
    /// Return the maximum number of days this month can have.
    /// @return Same as `minimumDayCount()`, except February returns 29 in leap years.
    [[nodiscard]] auto maximumDayCount() const noexcept -> Days;
    /// Test if the month has a fixed length in every year.
    /// @return `true` for all months except February.
    [[nodiscard]] auto hasFixedLength() const noexcept -> bool { return *this != february(); }
    /// Return the next month parts, clamped at the supported date range.
    /// If this is December of the last supported year, the result stays at the same year and month.
    /// @param year The current year.
    /// @return The next year and month.
    [[nodiscard]] auto next(Year year) const noexcept -> YearMonthParts;
    /// Return the previous month parts, clamped at the supported date range.
    /// If this is January of the first supported year, the result stays at the same year and month.
    /// @param year The current year.
    /// @return The previous year and month.
    [[nodiscard]] auto previous(Year year) const noexcept -> YearMonthParts;
    /// Test if this month has a following month in the supported date range.
    /// @param year The current year.
    /// @return `true` if this month is less than December or the year is less than 9999.
    [[nodiscard]] auto hasNext(Year year) const noexcept -> bool;
    /// Test if this month has a previous month in the supported date range.
    /// @param year The current year.
    /// @return `true` if this month is greater than January or the year is greater than 0.
    [[nodiscard]] auto hasPrevious(Year year) const noexcept -> bool;
    /// Extract the month and day from a zero-based day-of-year amount.
    /// Values outside the year are clamped by `DayOfYear::fromAmount()` before extraction.
    /// @param year The year to use for leap year calculation.
    /// @param days The zero-based day-of-year amount.
    /// @return The extracted month and day.
    [[nodiscard]] static auto extractMonthAndDay(Year year, Days days) noexcept -> MonthDayParts;
    /// Extract the month and day from a one-based day-of-year.
    /// Out-of-range day-of-year values are already clamped by `DayOfYear`.
    /// @param year The year to use for leap year calculation.
    /// @param dayOfYear The one-based day-of-year.
    /// @return The extracted month and day.
    [[nodiscard]] static auto extractMonthAndDay(Year year, DayOfYear dayOfYear) noexcept -> MonthDayParts;

public: // factory methods
    /// January
    [[nodiscard]] static auto january() noexcept -> Month { return Month{1}; }
    /// February
    [[nodiscard]] static auto february() noexcept -> Month { return Month{2}; }
    /// March
    [[nodiscard]] static auto march() noexcept -> Month { return Month{3}; }
    /// April
    [[nodiscard]] static auto april() noexcept -> Month { return Month{4}; }
    /// May
    [[nodiscard]] static auto may() noexcept -> Month { return Month{5}; }
    /// June
    [[nodiscard]] static auto june() noexcept -> Month { return Month{6}; }
    /// July
    [[nodiscard]] static auto july() noexcept -> Month { return Month{7}; }
    /// August
    [[nodiscard]] static auto august() noexcept -> Month { return Month{8}; }
    /// September
    [[nodiscard]] static auto september() noexcept -> Month { return Month{9}; }
    /// October
    [[nodiscard]] static auto october() noexcept -> Month { return Month{10}; }
    /// November
    [[nodiscard]] static auto november() noexcept -> Month { return Month{11}; }
    /// December
    [[nodiscard]] static auto december() noexcept -> Month { return Month{12}; }
};

}
