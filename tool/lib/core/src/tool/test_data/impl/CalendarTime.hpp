// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/FastRandom.hpp>
#include <erbsland/String.hpp>

#include <compare>
#include <cstdint>

namespace tool::test_data::impl {

/// Independent wall-clock model, with nanosecond resolution.
struct CalendarTime {
    int hour{};       ///< Hour in 0..23.
    int minute{};     ///< Minute in 0..59.
    int second{};     ///< Second in 0..59.
    int nanosecond{}; ///< Fraction in 0..999999999.
    auto operator<=>(const CalendarTime &) const = default;
    /// Get nanoseconds since midnight.
    [[nodiscard]] auto nanoseconds() const -> int64_t;
    /// Format the four wall-clock fields.
    [[nodiscard]] auto fields() const -> el::String;
    /// Split a valid nanosecond-of-day value.
    /// @param value Nanoseconds within one day.
    [[nodiscard]] static auto fromNanoseconds(int64_t value) -> CalendarTime;
    /// Sample a time of day.
    /// @param random Local seeded random source.
    [[nodiscard]] static auto random(el::FastRandom &random) -> CalendarTime;
};

}
