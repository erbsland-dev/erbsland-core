// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Month_fwd.hpp"
#include "Year_fwd.hpp"

#include "impl/TimePartBases.hpp"

namespace erbsland::time {

struct DateParts;

/// A day within a month, range `1..31`.
///
/// Provides clamped arithmetic and month-aware navigation methods like `next()` and `previous()`.
/// @tested{DayTest}
class Day final : public impl::DayBase {
    using Base = impl::DayBase;

public:
    using Base::Base;

public: // tests
    /// Test if this day exists in the given month.
    /// @param year The year to check.
    /// @param month The month to check.
    /// @return `true` if this day is valid in the given month and year.
    [[nodiscard]] auto exists(Year year, Month month) const noexcept -> bool;
    /// Test if this is the last day in the given month.
    /// @param year The year to check.
    /// @param month The month to check.
    /// @return `true` if this day equals the last day of the month.
    [[nodiscard]] auto isLast(Year year, Month month) const noexcept -> bool;
    /// Test if this day has a following date in the supported date range.
    /// @param year The year to check.
    /// @param month The month to check.
    /// @return `true` if advancing this day does not exceed the supported range.
    [[nodiscard]] auto hasNext(Year year, Month month) const noexcept -> bool;
    /// Test if this day has a previous date in the supported date range.
    /// @param year The year to check.
    /// @param month The month to check.
    /// @return `true` if retreating this day does not go below the supported range.
    [[nodiscard]] auto hasPrevious(Year year, Month month) const noexcept -> bool;

public: // algorithms
    /// Return the day, clamped to the valid range of the given year and month.
    /// @param year The year.
    /// @param month The month.
    /// @return The clamped day.
    [[nodiscard]] auto clamped(Year year, Month month) const noexcept -> Day;
    /// Return the next calendar day parts, clamped at the valid date range.
    /// If this is the last supported date, the result stays at the same date.
    /// @param year The current year.
    /// @param month The current month.
    /// @return The next calendar date parts.
    [[nodiscard]] auto next(Year year, Month month) const noexcept -> DateParts;
    /// Return the previous calendar day parts, clamped at the valid date range.
    /// If this is the first supported date, the result stays at the same date.
    /// @param year The current year.
    /// @param month The current month.
    /// @return The previous calendar date parts.
    [[nodiscard]] auto previous(Year year, Month month) const noexcept -> DateParts;

public: // factory methods
    /// Return the last day for the given month.
    /// @param year The year to check.
    /// @param month The month to check.
    /// @return The last valid day of the month (28-31).
    [[nodiscard]] static auto last(Year year, Month month) noexcept -> Day;
    /// Return the shortest possible last day of a month.
    /// @return `Day{28}`, the minimum last day across all months.
    [[nodiscard]] static auto lastMinimum() noexcept -> Day { return Day{28}; }
    /// Return the longest possible last day of a month.
    /// @return `Day{31}`, the maximum last day across all months.
    [[nodiscard]] static auto lastMaximum() noexcept -> Day { return Day{31}; }
};

}
