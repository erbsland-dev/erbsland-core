// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Time.hpp"
#include "TimeZone.hpp"

#include "../text/FormatAs.hpp"
#include "../text/String.hpp"
#include "../text/StringConverter.hpp"

namespace erbsland::time {

/// A wall-clock time accompanied by a time zone.
///
/// Without a date, a named zone cannot be resolved to a unique UTC offset. Combine this value with a `Date` to
/// construct a `DateTime` when an exact instant is required.
/// @seedoc{/reference/time/date_and_time}
/// @tested{TimeWithZoneTest}
class TimeWithZone final {
public:
    /// Create midnight UTC.
    TimeWithZone() noexcept = default;
    /// Create a time with a zone.
    /// @param time The wall-clock time.
    /// @param timeZone The accompanying time zone.
    TimeWithZone(Time time, TimeZone timeZone = {}) noexcept : _time{time}, _timeZone{timeZone} {}

    // defaults
    ~TimeWithZone() = default;
    TimeWithZone(const TimeWithZone &) noexcept = default;
    auto operator=(const TimeWithZone &) noexcept -> TimeWithZone & = default;
    TimeWithZone(TimeWithZone &&) noexcept = default;
    auto operator=(TimeWithZone &&) noexcept -> TimeWithZone & = default;

public: // operators
    [[nodiscard]] auto operator==(const TimeWithZone &other) const noexcept -> bool = default;

public: // accessors
    /// Return the wall-clock time.
    [[nodiscard]] constexpr auto time() const noexcept -> Time { return _time; }
    /// Return the accompanying time zone.
    [[nodiscard]] constexpr auto timeZone() const noexcept -> TimeZone { return _timeZone; }
    /// Return the hour component.
    [[nodiscard]] auto hour() const noexcept -> Hour { return _time.hour(); }
    /// Return the minute component.
    [[nodiscard]] auto minute() const noexcept -> Minute { return _time.minute(); }
    /// Return the second component.
    [[nodiscard]] auto second() const noexcept -> Second { return _time.second(); }
    /// Return the millisecond fraction.
    [[nodiscard]] auto millisecondFraction() const noexcept -> Milliseconds { return _time.millisecondFraction(); }
    /// Return the nanosecond fraction.
    [[nodiscard]] auto nanosecondFraction() const noexcept -> Nanoseconds { return _time.nanosecondFraction(); }

public: // conversion
    /// Convert this value to a compact human-readable representation.
    /// Local-origin zones are omitted. UTC and fixed zones use ISO suffixes; other named zones use brackets.
    /// @return The formatted wall-clock time and zone.
    [[nodiscard]] auto toString() const -> text::String;

private:
    Time _time;         ///< The wall-clock time.
    TimeZone _timeZone; ///< The accompanying zone.
};

}

template <>
struct erbsland::text::FormatAsText<erbsland::time::TimeWithZone> : FormatAs<time::TimeWithZone, String> {
    [[nodiscard]] auto format(const time::TimeWithZone &value) const -> String { return value.toString(); }
};
