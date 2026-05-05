// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DayOfWeekFormat.hpp"
#include "TimeAmounts.hpp"

#include "impl/TimePartBases.hpp"

#include "../text/String.hpp"

namespace erbsland::time {

/// A day of week, Monday (`0`) through Sunday (`6`).
///
/// Provides clamped arithmetic.
/// @tested{DayOfWeekTest}
class DayOfWeek final : public impl::DayOfWeekBase {
    using Base = impl::DayOfWeekBase;

public:
    using Base::Base;
    /// Days to the next occurrence, excluding today unless equal.
    /// @param dayOfWeek The target day of week.
    /// @return The signed number of days to the next occurrence.
    [[nodiscard]] auto daysToNext(DayOfWeek dayOfWeek) const noexcept -> Days;
    /// Days to the previous occurrence, excluding today unless equal.
    /// @param dayOfWeek The target day of week.
    /// @return The signed number of days to the previous occurrence.
    [[nodiscard]] auto daysToPrevious(DayOfWeek dayOfWeek) const noexcept -> Days;
    /// Convert this day to a display string.
    /// @param format The formatting style (short or long name).
    /// @return A string view of the day name.
    [[nodiscard]] auto toString(DayOfWeekFormat format = DayOfWeekFormat::Long) const -> text::StringView;

public: // factory methods
    /// Monday
    [[nodiscard]] static auto monday() noexcept -> DayOfWeek { return DayOfWeek{0}; }
    /// Tuesday
    [[nodiscard]] static auto tuesday() noexcept -> DayOfWeek { return DayOfWeek{1}; }
    /// Wednesday
    [[nodiscard]] static auto wednesday() noexcept -> DayOfWeek { return DayOfWeek{2}; }
    /// Thursday
    [[nodiscard]] static auto thursday() noexcept -> DayOfWeek { return DayOfWeek{3}; }
    /// Friday
    [[nodiscard]] static auto friday() noexcept -> DayOfWeek { return DayOfWeek{4}; }
    /// Saturday
    [[nodiscard]] static auto saturday() noexcept -> DayOfWeek { return DayOfWeek{5}; }
    /// Sunday
    [[nodiscard]] static auto sunday() noexcept -> DayOfWeek { return DayOfWeek{6}; }
    /// Epoch day (Saturday)
    [[nodiscard]] static auto atEpoch() noexcept -> DayOfWeek { return saturday(); }
};

}
