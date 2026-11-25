// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/FastRandom.hpp>
#include <erbsland/String.hpp>

#include <compare>
#include <cstdint>

namespace tool::test_data::impl {

/// Independent proleptic Gregorian model for fixture expectations, including leap year zero.
struct CalendarDate {
    int year{};   ///< Year in 0..9999.
    int month{1}; ///< Month in 1..12.
    int day{1};   ///< Day in the selected month.

    auto operator<=>(const CalendarDate &) const = default;
    /// Get the zero-based day since year zero.
    [[nodiscard]] auto days() const -> int64_t;
    /// Get the one-based day of the year.
    [[nodiscard]] auto dayOfYear() const -> int;
    /// Add months, clamping the date and the day of the target month.
    /// @param amount Signed number of months.
    [[nodiscard]] auto addedMonths(int64_t amount) const -> CalendarDate;
    /// Format year, month, and day as fixture fields.
    [[nodiscard]] auto fields() const -> el::String;
    /// Count days in the given Gregorian month.
    /// @param year Year in 0..9999.
    /// @param month Month in 1..12.
    [[nodiscard]] static auto monthDays(int year, int month) -> int;
    /// Count days before the given year.
    /// @param year Year in 0..10000.
    [[nodiscard]] static auto yearStart(int year) -> int64_t;
    /// Convert a day index, clamped to the supported range.
    /// @param days Day index since year zero.
    [[nodiscard]] static auto fromDays(int64_t days) -> CalendarDate;
    /// Sample a supported date with rejection of invalid month days.
    /// @param random Local seeded random source.
    [[nodiscard]] static auto random(el::FastRandom &random) -> CalendarDate;
};

}
