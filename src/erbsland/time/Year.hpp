// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DayOfYear_fwd.hpp"
#include "Month_fwd.hpp"

#include "impl/TimePartBases.hpp"

#include <array>

namespace erbsland::time {

struct YearDayOfYearParts;

/// A Gregorian calendar year in the supported range `0...9999`.
/// @tested{YearTest}
class Year final : public impl::YearBase {
    using Base = impl::YearBase;

public: // importing the constructors from the base class.
    using Base::Base;

public: // tests
    /// Test if this is a leap year in the proleptic Gregorian calendar.
    [[nodiscard]] auto isLeapYear() const noexcept -> bool;

public: // algorithms
    /// Return the number of days in this year.
    /// @return 366 for leap years, 365 otherwise.
    [[nodiscard]] auto dayCount() const noexcept -> Days;
    /// Return the number of days from epoch to the first day of this year.
    /// @return The number of days since the epoch.
    [[nodiscard]] auto daysSinceEpoch() const noexcept -> Days;
    /// Return the number of days before a given month in this year.
    /// @param month The month (1-12).
    /// @return The cumulative day count before the month starts.
    [[nodiscard]] auto daysBeforeMonth(Month month) const noexcept -> Days;
    /// Return the month of the day of year
    /// @param dayOfYear The day of the year.
    /// @return The month of that day.
    [[nodiscard]] auto monthOfDay(DayOfYear dayOfYear) const noexcept -> Month;

public: // iteration
    /// Return the next year, clamped at the supported range.
    /// @return The following year, or the current year if already at maximum.
    [[nodiscard]] auto next() const noexcept -> Year;
    /// Return the previous year, clamped at the supported range.
    /// @return The preceding year, or the current year if already at minimum.
    [[nodiscard]] auto previous() const noexcept -> Year;
    /// Test if this year has a following year in the supported range.
    /// @return `true` if this year is less than 9999.
    [[nodiscard]] constexpr auto hasNext() const noexcept -> bool { return !isLast(); }
    /// Test if this year has a previous year in the supported range.
    /// @return `true` if this year is greater than 0.
    [[nodiscard]] constexpr auto hasPrevious() const noexcept -> bool { return !isFirst(); }

public: // factory
    /// Extract a year and zero-based day-of-year from days since epoch.
    /// Values before the epoch are clamped to `0000-01-01`; values after the supported range are clamped to
    /// `9999-12-31`.
    /// @param days The number of days since the epoch.
    /// @return The extracted year and zero-based day-of-year.
    [[nodiscard]] static auto extractFromEpoch(Days days) noexcept -> YearDayOfYearParts;

private:
    using StartDayArray = std::array<Days, 13>;

    [[nodiscard]] auto startDayArray() const noexcept -> const StartDayArray &;
};

}
