// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeAmounts.hpp"
#include "TimeDelta.hpp"
#include "TimeDeltaFormat.hpp"

#include "../text/FormatAs.hpp"
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
    /// All independently stored delta parts.
    struct Parts {
        Nanoseconds nanoseconds;
        Microseconds microseconds;
        Milliseconds milliseconds;
        Seconds seconds;
        Minutes minutes;
        Hours hours;
        Days days;
        Weeks weeks;
        Months months;
        Years years;

        friend auto operator==(const Parts &, const Parts &) noexcept -> bool = default;
    };

public:
    /// Create a zero calendar delta.
    CalendarDelta() noexcept = default;
    /// Create a calendar delta from all parts.
    explicit CalendarDelta(Parts parts) noexcept : _parts{parts} {}
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
    ~CalendarDelta() = default;
    CalendarDelta(const CalendarDelta &) noexcept = default;
    auto operator=(const CalendarDelta &) noexcept -> CalendarDelta & = default;
    CalendarDelta(CalendarDelta &&) noexcept = default;
    auto operator=(CalendarDelta &&) noexcept -> CalendarDelta & = default;

public: // operators
    [[nodiscard]] auto operator==(const CalendarDelta &other) const noexcept -> bool = default;
    [[nodiscard]] auto operator+(CalendarDelta other) const noexcept -> CalendarDelta;
    auto operator+=(CalendarDelta other) noexcept -> CalendarDelta &;
    [[nodiscard]] auto operator-(CalendarDelta other) const noexcept -> CalendarDelta;
    auto operator-=(CalendarDelta other) noexcept -> CalendarDelta &;
    [[nodiscard]] auto operator-() const noexcept -> CalendarDelta;

public: // tests
    [[nodiscard]] auto isZero() const noexcept -> bool;
    [[nodiscard]] auto isValidTimeDelta() const noexcept -> bool;

public: // accessors
    [[nodiscard]] constexpr auto parts() const noexcept -> Parts { return _parts; }
    [[nodiscard]] constexpr auto nanoseconds() const noexcept -> Nanoseconds { return _parts.nanoseconds; }
    auto setNanoseconds(Nanoseconds value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto microseconds() const noexcept -> Microseconds { return _parts.microseconds; }
    auto setMicroseconds(Microseconds value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto milliseconds() const noexcept -> Milliseconds { return _parts.milliseconds; }
    auto setMilliseconds(Milliseconds value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto seconds() const noexcept -> Seconds { return _parts.seconds; }
    auto setSeconds(Seconds value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto minutes() const noexcept -> Minutes { return _parts.minutes; }
    auto setMinutes(Minutes value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto hours() const noexcept -> Hours { return _parts.hours; }
    auto setHours(Hours value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto days() const noexcept -> Days { return _parts.days; }
    auto setDays(Days value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto weeks() const noexcept -> Weeks { return _parts.weeks; }
    auto setWeeks(Weeks value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto months() const noexcept -> Months { return _parts.months; }
    auto setMonths(Months value) noexcept -> CalendarDelta &;
    [[nodiscard]] constexpr auto years() const noexcept -> Years { return _parts.years; }
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

template <>
struct erbsland::text::FormatAsText<erbsland::time::CalendarDelta> : FormatAs<time::CalendarDelta, String> {
    [[nodiscard]] auto format(const time::CalendarDelta &value) const -> String { return value.toString(); }
};
