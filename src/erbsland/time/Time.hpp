// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DateTimePrecision.hpp"
#include "Duration.hpp"
#include "Hour.hpp"
#include "IsoTimeFormat.hpp"
#include "Minute.hpp"
#include "Second.hpp"
#include "TimeDelta.hpp"

#include "../text/FormatAs.hpp"
#include "../text/StringConverter.hpp"
#include "../text/StringEditor.hpp"

#include <compare>

namespace erbsland::time {

struct TimeWrapResult;

/// A wall-clock time split into named parts.
/// @tested{TimeCoreTest}
struct TimeParts {
    Hour hour;                      ///< The hour component, range `0..23`.
    Minute minute;                  ///< The minute component, range `0..59`.
    Second second;                  ///< The second component, range `0..59`.
    Nanoseconds nanosecondFraction; ///< The nanosecond fraction, range `0..999999999`.

    friend auto operator==(const TimeParts &, const TimeParts &) noexcept -> bool = default;
};

/// A wall-clock time of day with nanosecond precision.
///
/// Represents a time within a day from midnight to just before the next midnight.
/// Stored internally as nanoseconds since midnight for sub-second precision.
/// @seedoc{/reference/time/date_and_time}
/// @tested{TimeTest}
class Time final {
    using Storage = math::SatInt64;

public:
    /// Create midnight (zero time).
    Time() noexcept = default;
    /// Create a time from parts.
    ///
    /// The hour, minute and second part types clamp their construction input to their valid ranges. The nanosecond
    /// fraction is additionally clamped to the range `0..999999999`.
    /// @param hour The hour (0-23).
    /// @param minute The minute (0-59).
    /// @param second The second (0-59), defaults to zero.
    /// @param nsFraction The nanosecond fraction (0-999999999), defaults to zero.
    Time(Hour hour, Minute minute, Second second = Second{}, Nanoseconds nsFraction = Nanoseconds{}) noexcept;

    // defaults
    ~Time() = default;
    Time(const Time &) noexcept = default;
    auto operator=(const Time &) noexcept -> Time & = default;
    Time(Time &&) noexcept = default;
    auto operator=(Time &&) noexcept -> Time & = default;

public: // operators
    [[nodiscard]] auto operator<=>(const Time &other) const noexcept -> std::strong_ordering = default;

public: // tests
    /// Test if this time is exactly midnight.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _nanoseconds.isZero(); }

public: // accessors
    /// Return the hour component.
    /// @return The hour (0-23).
    [[nodiscard]] auto hour() const noexcept -> Hour;
    /// Return the minute component.
    /// @return The minute (0-59).
    [[nodiscard]] auto minute() const noexcept -> Minute;
    /// Return the second component.
    /// @return The second (0-59).
    [[nodiscard]] auto second() const noexcept -> Second;
    /// Return the millisecond fraction of the second.
    /// @return The millisecond component (0-999).
    [[nodiscard]] auto millisecondFraction() const noexcept -> Milliseconds;
    /// Return the nanosecond fraction of the second.
    /// @return The nanosecond component (0-999999999).
    [[nodiscard]] auto nanosecondFraction() const noexcept -> Nanoseconds;
    /// Return all time parts.
    /// @return Named hour, minute, second, and nanosecond fraction parts.
    [[nodiscard]] auto parts() const noexcept -> TimeParts;
    /// Return the duration since midnight.
    /// @return The duration from midnight, truncated to seconds.
    [[nodiscard]] auto durationSinceMidnight() const noexcept -> Duration;
    /// Return the time delta since midnight with nanosecond precision.
    /// @return The time delta from midnight.
    [[nodiscard]] auto timeDeltaSinceMidnight() const noexcept -> TimeDelta;

public: // conversion
    /// Return the total seconds since midnight.
    /// @return The seconds since midnight.
    [[nodiscard]] auto toSecondsSinceMidnight() const noexcept -> Seconds;
    /// Return the total nanoseconds since midnight.
    /// @return The nanoseconds since midnight.
    [[nodiscard]] auto toNanosecondsSinceMidnight() const noexcept -> Nanoseconds;
    /// Convert this time to an ISO 8601 string.
    /// @param flags Formatting flags for the output.
    /// @param precision The largest precision to include.
    /// @return The ISO-formatted time string.
    [[nodiscard]] auto toIsoString(
        IsoTimeFormatFlags flags = cDefaultTimeFormat, DateTimePrecision precision = DateTimePrecision::Second) const
        -> text::String;

public: // manipulation
    /// Add a time delta, wrapping past midnight.
    ///
    /// Any positive or negative delta is accepted. If the addition crosses midnight, the time wraps and the number of
    /// days crossed is returned. Negative deltas return a negative day count.
    /// @param delta The delta to add.
    /// @return The number of days crossed (positive or negative).
    auto addWithWrap(TimeDelta delta) noexcept -> Days;
    /// Add a duration, wrapping past midnight.
    /// Any positive or negative duration is accepted. Negative durations return a negative day count.
    /// @param duration The duration to add.
    /// @return The number of days crossed (positive or negative).
    auto addWithWrap(Duration duration) noexcept -> Days;
    /// Return this time plus a delta, wrapping past midnight.
    /// @param delta The delta to add.
    /// @return The wrapped time and the number of days crossed.
    [[nodiscard]] auto addedWithWrap(TimeDelta delta) const noexcept -> TimeWrapResult;
    /// Return this time plus a duration, wrapping past midnight.
    /// @param duration The duration to add.
    /// @return The wrapped time and the number of days crossed.
    [[nodiscard]] auto addedWithWrap(Duration duration) const noexcept -> TimeWrapResult;

public:
    /// Create a time from a duration since midnight.
    /// Negative and over-day values wrap into the 24-hour range.
    /// @param duration The duration since midnight.
    /// @return The resulting time, wrapped if necessary.
    [[nodiscard]] static auto fromDurationSinceMidnight(TimeDelta duration) noexcept -> Time;
    /// Create a time from a duration since midnight.
    /// Negative and over-day values wrap into the 24-hour range.
    /// @param duration The duration since midnight.
    /// @return The resulting time, wrapped if necessary.
    [[nodiscard]] static auto fromDurationSinceMidnight(Duration duration) noexcept -> Time;
    /// Return the first possible time of day.
    /// @return 00:00:00.000000000.
    [[nodiscard]] static auto first() noexcept -> Time;
    /// Return the last possible time of day.
    /// @return 23:59:59.999999999.
    [[nodiscard]] static auto last() noexcept -> Time;

private:
    Storage _nanoseconds; ///< Nanoseconds since midnight.
};

/// The result of adding to a wall-clock time with wrapping.
/// @tested{TimeCoreTest}
struct TimeWrapResult {
    Time time; ///< The wrapped time of day.
    Days days; ///< The number of day boundaries crossed.

    friend auto operator==(const TimeWrapResult &, const TimeWrapResult &) noexcept -> bool = default;
};

}

template <>
struct erbsland::text::FormatAsText<erbsland::time::Time> : FormatAs<time::Time, String> {
    [[nodiscard]] auto format(const time::Time &value) const -> String { return value.toIsoString(); }
};

template <>
struct std::formatter<erbsland::time::Time> : std::formatter<std::string_view> {
    auto format(const erbsland::time::Time value, std::format_context &ctx) const {
        return std::formatter<std::string_view>::format(
            erbsland::text::StringConverter{value.toIsoString()}.toStdString(), ctx);
    }
};
