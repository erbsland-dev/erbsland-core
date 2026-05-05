// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Year_fwd.hpp"

#include "impl/TimePartBases.hpp"

namespace erbsland::time {

/// A one-based day of year, range `1..365` (or `1..366` in leap years).
///
/// Provides clamped arithmetic.
/// @tested{DayOfYearTest}
class DayOfYear final : public impl::DayOfYearBase {
    using Base = impl::DayOfYearBase;

public:
    using Base::Base;

public: // accessors
    /// Test if this is the last day in `year`.
    /// @param year The year to check against.
    /// @return `true` if this is the last day of the year.
    [[nodiscard]] auto isLast(Year year) const noexcept -> bool;

public: // factory methods
    /// Return the last day of year for `year`.
    /// @param year The year to query.
    /// @return 365 for common years, 366 for leap years.
    [[nodiscard]] static auto last(Year year) noexcept -> DayOfYear;
    /// Return the shortest possible last day-of-year (365, for common years).
    /// @return The minimum last day-of-year.
    [[nodiscard]] static auto lastMinimum() noexcept -> DayOfYear { return DayOfYear{365}; }
    /// Return the longest possible last day-of-year (366, for leap years).
    /// @return The maximum last day-of-year.
    [[nodiscard]] static auto lastMaximum() noexcept -> DayOfYear { return DayOfYear{366}; }
};

}
