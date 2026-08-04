// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CalendarDeltaParts.hpp"
#include "TimeDelta.hpp"
#include "TimeDeltaFormat.hpp"

#include "../text/String.hpp"
#include "../text/StringConverter.hpp"

#include <optional>
#include <type_traits>

namespace erbsland::time {

/// A non-normalized delta composed from independent fixed and calendar amounts.
/// @seedoc{/reference/time/duration_and_time_amounts}
/// @tested{CalendarDeltaTest}
class CalendarDelta final {
public:
    /// Alias for all independently stored delta parts.
    using Parts = CalendarDeltaParts;

public:
    /// Create a calendar delta from all parts.
    explicit CalendarDelta(const Parts &parts) noexcept : _parts{parts} {}
    /// Create a calendar delta containing one amount.
    template <typename tAmount>
        requires(
            std::is_same_v<tAmount, Nanoseconds> || std::is_same_v<tAmount, Microseconds> ||
            std::is_same_v<tAmount, Milliseconds> || std::is_same_v<tAmount, Seconds> ||
            std::is_same_v<tAmount, Minutes> || std::is_same_v<tAmount, Hours> || std::is_same_v<tAmount, Days> ||
            std::is_same_v<tAmount, Weeks> || std::is_same_v<tAmount, Months> || std::is_same_v<tAmount, Years>)
    CalendarDelta(tAmount amount) noexcept { // NOLINT(*-explicit-constructor)
        setAmount(amount);
    }

    // defaults
    CalendarDelta() noexcept = default;
    ~CalendarDelta() = default;
    CalendarDelta(const CalendarDelta &) noexcept = default;
    auto operator=(const CalendarDelta &) noexcept -> CalendarDelta & = default;
    CalendarDelta(CalendarDelta &&) noexcept = default;
    auto operator=(CalendarDelta &&) noexcept -> CalendarDelta & = default;

public: // operators
    [[nodiscard]] auto operator==(const CalendarDelta &other) const noexcept -> bool = default;
    /// Add `other` to this delta.
    [[nodiscard]] auto operator+(CalendarDelta other) const noexcept -> CalendarDelta;
    /// Add `other` to this delta in place.
    auto operator+=(CalendarDelta other) noexcept -> CalendarDelta &;
    /// Subtract `other` from this delta.
    [[nodiscard]] auto operator-(CalendarDelta other) const noexcept -> CalendarDelta;
    /// Subtract `other` from this delta in place.
    auto operator-=(CalendarDelta other) noexcept -> CalendarDelta &;
    /// Negate every part of this delta.
    [[nodiscard]] auto operator-() const noexcept -> CalendarDelta;

public: // tests
    /// Test whether every part is zero.
    [[nodiscard]] auto isZero() const noexcept -> bool;
    /// Test whether this delta contains no calendar-dependent units.
    [[nodiscard]] auto isValidTimeDelta() const noexcept -> bool;

public: // accessors
    /// Access all independently stored parts.
    [[nodiscard]] constexpr auto parts() const noexcept -> Parts { return _parts; }
    /// Access the nanosecond part.
    [[nodiscard]] constexpr auto nanoseconds() const noexcept -> Nanoseconds { return _parts.nanoseconds; }
    /// Set the nanosecond part.
    auto setNanoseconds(Nanoseconds value) noexcept -> CalendarDelta &;
    /// Access the microsecond part.
    [[nodiscard]] constexpr auto microseconds() const noexcept -> Microseconds { return _parts.microseconds; }
    /// Set the microsecond part.
    auto setMicroseconds(Microseconds value) noexcept -> CalendarDelta &;
    /// Access the millisecond part.
    [[nodiscard]] constexpr auto milliseconds() const noexcept -> Milliseconds { return _parts.milliseconds; }
    /// Set the millisecond part.
    auto setMilliseconds(Milliseconds value) noexcept -> CalendarDelta &;
    /// Access the second part.
    [[nodiscard]] constexpr auto seconds() const noexcept -> Seconds { return _parts.seconds; }
    /// Set the second part.
    auto setSeconds(Seconds value) noexcept -> CalendarDelta &;
    /// Access the minute part.
    [[nodiscard]] constexpr auto minutes() const noexcept -> Minutes { return _parts.minutes; }
    /// Set the minute part.
    auto setMinutes(Minutes value) noexcept -> CalendarDelta &;
    /// Access the hour part.
    [[nodiscard]] constexpr auto hours() const noexcept -> Hours { return _parts.hours; }
    /// Set the hour part.
    auto setHours(Hours value) noexcept -> CalendarDelta &;
    /// Access the day part.
    [[nodiscard]] constexpr auto days() const noexcept -> Days { return _parts.days; }
    /// Set the day part.
    auto setDays(Days value) noexcept -> CalendarDelta &;
    /// Access the week part.
    [[nodiscard]] constexpr auto weeks() const noexcept -> Weeks { return _parts.weeks; }
    /// Set the week part.
    auto setWeeks(Weeks value) noexcept -> CalendarDelta &;
    /// Access the month part.
    [[nodiscard]] constexpr auto months() const noexcept -> Months { return _parts.months; }
    /// Set the month part.
    auto setMonths(Months value) noexcept -> CalendarDelta &;
    /// Access the year part.
    [[nodiscard]] constexpr auto years() const noexcept -> Years { return _parts.years; }
    /// Set the year part.
    auto setYears(Years value) noexcept -> CalendarDelta &;

public: // conversion
    /// Convert to a fixed TimeDelta if all parts can be represented exactly.
    [[nodiscard]] auto toTimeDelta() const noexcept -> std::optional<TimeDelta>;
    /// Convert to a fixed TimeDelta.
    /// @throws err::OverflowError if calendar units are present or the fixed sum exceeds TimeDelta bounds.
    [[nodiscard]] auto toTimeDeltaOrThrow() const -> TimeDelta;
    /// Convert this value to text.
    [[nodiscard]] auto toString(const TimeDeltaFormat &format = {}) const -> text::String;

private:
    template <typename tAmount>
    /// Store `amount` in its matching part.
    void setAmount(tAmount amount) noexcept {
        if constexpr (std::is_same_v<tAmount, Nanoseconds>) {
            _parts.nanoseconds = amount;
        } else if constexpr (std::is_same_v<tAmount, Microseconds>) {
            _parts.microseconds = amount;
        } else if constexpr (std::is_same_v<tAmount, Milliseconds>) {
            _parts.milliseconds = amount;
        } else if constexpr (std::is_same_v<tAmount, Seconds>) {
            _parts.seconds = amount;
        } else if constexpr (std::is_same_v<tAmount, Minutes>) {
            _parts.minutes = amount;
        } else if constexpr (std::is_same_v<tAmount, Hours>) {
            _parts.hours = amount;
        } else if constexpr (std::is_same_v<tAmount, Days>) {
            _parts.days = amount;
        } else if constexpr (std::is_same_v<tAmount, Weeks>) {
            _parts.weeks = amount;
        } else if constexpr (std::is_same_v<tAmount, Months>) {
            _parts.months = amount;
        } else if constexpr (std::is_same_v<tAmount, Years>) {
            _parts.years = amount;
        }
    }

    Parts _parts; ///< Independently stored parts.
};

}
