// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Date.hpp"
#include "Duration.hpp"
#include "Time.hpp"
#include "TimeOccurrenceInFold.hpp"
#include "TimeZone.hpp"

#include "tz/TimeOffset.hpp"

#include "../text/String.hpp"
#include "../text/StringConverter.hpp"
#include "../text/StringEditor.hpp"

#include <compare>
#include <cstdint>
#include <ctime>
#include <optional>
#include <utility>

namespace erbsland::time {

/// A local date/time split into named parts.
/// @tested{TimeCoreTest}
struct DateTimeParts {
    Year year;                      ///< The local year component.
    Month month;                    ///< The local month component.
    Day day;                        ///< The local day component.
    Hour hour;                      ///< The local hour component.
    Minute minute;                  ///< The local minute component.
    Second second;                  ///< The local second component.
    Nanoseconds nanosecondFraction; ///< The local nanosecond fraction.

    friend auto operator==(const DateTimeParts &, const DateTimeParts &) noexcept -> bool = default;
};

/// A point in time represented as UTC date/time plus display offset information.
/// @seedoc{/reference/time/date_and_time}
/// @tested{DateTimeTest}
class DateTime final {
    struct PrivateTag {};

public:
    /// Create an invalid date/time.
    DateTime() noexcept = default;
    /// Create a UTC date/time.
    /// If `utcDate` is invalid, the resulting date/time is invalid and its time and offset are reset.
    /// @param utcDate The date.
    /// @param utcTime The time in the UTC timezone.
    DateTime(const Date utcDate, const Time utcTime) noexcept : _date{utcDate}, _time{utcTime} { subtractOffset(); }
    /// Create a fixed-offset local date/time.
    /// Invalid local dates result in an invalid date/time. Offsets are normalized into the supported UTC offset range.
    /// @param localDate The local date.
    /// @param localTime The local time.
    /// @param offset The UTC offset.
    DateTime(Date localDate, Time localTime, Seconds offset) noexcept;
    /// Create a fixed-offset local date/time.
    /// @param localDate The local date.
    /// @param localTime The local time.
    /// @param offset The UTC offset.
    DateTime(const Date localDate, const Time localTime, const Duration offset) noexcept :
        DateTime{localDate, localTime, offset.toSeconds()} {}
    /// Create a named-zone local date/time.
    /// Invalid local dates result in an invalid date/time. During folds, `occurrence` selects the first or second
    /// possible instant; local times in gaps are resolved using the zone database transition rule.
    /// @param localDate The local date.
    /// @param localTime The local time.
    /// @param timeZone The time zone.
    /// @param occurrence Which occurrence to use during fold periods.
    DateTime(
        Date localDate,
        Time localTime,
        TimeZone timeZone,
        TimeOccurrenceInFold occurrence = TimeOccurrenceInFold::First) noexcept;

    // defaults
    ~DateTime() = default;
    DateTime(const DateTime &) noexcept = default;
    auto operator=(const DateTime &) noexcept -> DateTime & = default;
    DateTime(DateTime &&) noexcept = default;
    auto operator=(DateTime &&) noexcept -> DateTime & = default;

public: // operators
    [[nodiscard]] auto operator<=>(const DateTime &other) const noexcept -> std::strong_ordering;
    [[nodiscard]] auto operator==(const DateTime &other) const noexcept -> bool = default;
    [[nodiscard]] auto operator+(const Duration duration) const noexcept -> DateTime { return added(duration); }
    auto operator+=(const Duration duration) noexcept -> DateTime & {
        add(duration);
        return *this;
    }
    [[nodiscard]] auto operator-(const Duration duration) const noexcept -> DateTime { return subtracted(duration); }
    auto operator-=(const Duration duration) noexcept -> DateTime & {
        subtract(duration);
        return *this;
    }
    /// Return the signed duration from `other` to this date/time.
    /// @param other The other date/time.
    /// @return The duration from `other` to this.
    [[nodiscard]] auto operator-(const DateTime &other) const noexcept -> Duration { return other.durationTo(*this); }

public: // tests
    /// Test if this date/time represents a valid instant.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _date.isValid(); }
    /// Test if this date/time is displayed in UTC.
    [[nodiscard]] constexpr auto isUtc() const noexcept -> bool { return _offset.isUtc(); }

public: // accessors
    /// Return the stored UTC date, or an invalid date for invalid date/times.
    [[nodiscard]] constexpr auto utcDate() const noexcept -> Date { return _date; }
    /// Return the stored UTC time, or midnight for invalid date/times.
    [[nodiscard]] constexpr auto utcTime() const noexcept -> Time { return _time; }
    /// Return the local display date, or an invalid date for invalid date/times.
    [[nodiscard]] auto date() const noexcept -> Date { return localDateTime().first; }
    /// Return the local display time, or midnight for invalid date/times.
    [[nodiscard]] auto time() const noexcept -> Time { return localDateTime().second; }
    /// Return the local year, or zero for invalid date/times.
    [[nodiscard]] auto year() const noexcept -> Year { return date().year(); }
    /// Return the local month, or January for invalid date/times.
    [[nodiscard]] auto month() const noexcept -> Month { return date().month(); }
    /// Return the local day, or one for invalid date/times.
    [[nodiscard]] auto day() const noexcept -> Day { return date().day(); }
    /// Return the local day of year, or one for invalid date/times.
    [[nodiscard]] auto dayOfYear() const noexcept -> DayOfYear { return date().dayOfYear(); }
    /// Return the local day of week, or the epoch weekday for invalid date/times.
    [[nodiscard]] auto dayOfWeek() const noexcept -> DayOfWeek { return date().dayOfWeek(); }
    /// Return the local hour.
    [[nodiscard]] auto hour() const noexcept -> Hour { return time().hour(); }
    /// Return the local minute.
    [[nodiscard]] auto minute() const noexcept -> Minute { return time().minute(); }
    /// Return the local second.
    [[nodiscard]] auto second() const noexcept -> Second { return time().second(); }
    /// Return the local millisecond fraction.
    [[nodiscard]] auto millisecondFraction() const noexcept -> Milliseconds { return time().millisecondFraction(); }
    /// Return the local nanosecond fraction.
    [[nodiscard]] auto nanosecondFraction() const noexcept -> Nanoseconds { return time().nanosecondFraction(); }
    /// Return all local date/time parts.
    /// For invalid date/times, returns the same fallback parts as the individual accessors.
    [[nodiscard]] auto parts() const noexcept -> DateTimeParts;
    /// Return the display offset from UTC.
    [[nodiscard]] auto timeOffset() const noexcept -> Duration { return Duration{_offset.offset()}; }
    /// Return the display time zone, or UTC for invalid date/times and fixed offsets.
    [[nodiscard]] auto timeZone() const noexcept -> TimeZone { return TimeZone{_offset.zoneId()}; }
    /// Return the display time-zone abbreviation, or an empty string when none is available.
    [[nodiscard]] auto timeZoneAbbreviation() const -> text::String;

public: // manipulation
    /// Test if adding a duration would saturate to the first or last supported date/time.
    /// Returns `false` for invalid date/times.
    /// @param duration The duration to add.
    /// @return `true` if the result would be clamped to the supported date/time range.
    [[nodiscard]] auto wouldAddSaturate(Duration duration) const noexcept -> bool;
    /// Test if subtracting a duration would saturate to the first or last supported date/time.
    /// Returns `false` for invalid date/times.
    /// @param duration The duration to subtract.
    /// @return `true` if the result would be clamped to the supported date/time range.
    [[nodiscard]] auto wouldSubtractSaturate(Duration duration) const noexcept -> bool;
    /// Return this date/time plus a duration, clamped to the supported date/time range.
    /// Invalid date/times stay invalid.
    [[nodiscard]] auto added(Duration duration) const noexcept -> DateTime;
    /// Return this date/time plus a duration.
    /// Invalid date/times stay invalid.
    /// @param duration The duration to add.
    /// @return The resulting date/time.
    /// @throws err::OverflowError if the result would exceed the supported date/time range.
    [[nodiscard]] auto addedOrThrow(Duration duration) const -> DateTime;
    /// Add a duration in place, clamped to the supported date/time range.
    /// Invalid date/times stay invalid.
    void add(const Duration duration) noexcept { *this = added(duration); }
    /// Add a duration in place.
    /// Invalid date/times stay invalid.
    /// @param duration The duration to add.
    /// @throws err::OverflowError if the result would exceed the supported date/time range.
    void addOrThrow(const Duration duration) { *this = addedOrThrow(duration); }
    /// Return this date/time minus a duration, clamped to the supported date/time range.
    /// Invalid date/times stay invalid.
    [[nodiscard]] auto subtracted(const Duration duration) const noexcept -> DateTime { return added(-duration); }
    /// Return this date/time minus a duration.
    /// Invalid date/times stay invalid.
    /// @param duration The duration to subtract.
    /// @return The resulting date/time.
    /// @throws err::OverflowError if the result would exceed the supported date/time range.
    [[nodiscard]] auto subtractedOrThrow(Duration duration) const -> DateTime;
    /// Subtract a duration in place, clamped to the supported date/time range.
    /// Invalid date/times stay invalid.
    void subtract(const Duration duration) noexcept { *this = subtracted(duration); }
    /// Subtract a duration in place.
    /// Invalid date/times stay invalid.
    /// @param duration The duration to subtract.
    /// @throws err::OverflowError if the result would exceed the supported date/time range.
    void subtractOrThrow(const Duration duration) { *this = subtractedOrThrow(duration); }
    /// Calculate the signed duration from this date/time to `other`.
    /// Invalid date/times participate as `-1` seconds since epoch.
    /// @param other The other date/time.
    /// @return The duration from this to `other`.
    [[nodiscard]] auto durationTo(const DateTime &other) const noexcept -> Duration;
    /// Calculate the signed nanosecond time delta from this date/time to `other`.
    /// Invalid date/times participate as `-1` seconds since epoch.
    [[nodiscard]] auto timeDeltaTo(const DateTime &other) const noexcept -> TimeDelta;

public: // conversion
    /// Convert to UTC, or return an invalid date/time if this date/time is invalid.
    [[nodiscard]] auto toUtc() const noexcept -> DateTime;
    /// Convert the display time zone, or return an invalid date/time if this date/time is invalid.
    [[nodiscard]] auto toTimeZone(TimeZone timeZone) const noexcept -> DateTime;
    /// Convert to seconds since the internal epoch, or `-1` for invalid date/times.
    [[nodiscard]] auto toSecondsSinceEpoch() const noexcept -> Seconds;
    /// Convert to `std::time_t` using the POSIX epoch.
    /// Invalid date/times convert from the internal `-1` seconds sentinel.
    [[nodiscard]] auto toTimeT() const noexcept -> std::time_t;
    /// Convert to Windows FILETIME 100-nanosecond ticks since 1601-01-01 UTC.
    /// Returns no value if this date/time is invalid or before the Windows FILETIME epoch.
    [[nodiscard]] auto toWindowsFileTimeTicks() const noexcept -> std::optional<std::uint64_t>;
    /// Convert this date/time to an ISO 8601 string.
    /// Invalid date/times return an empty string.
    /// @param flags Formatting flags for date, time, and offset output.
    /// @param precision The largest precision to include in the text.
    /// @return The ISO-formatted string.
    [[nodiscard]] auto toIsoString(
        IsoTimeFormatFlags flags = cDefaultDateTimeFormat,
        DateTimePrecision precision = DateTimePrecision::Second) const -> text::String;

public:
    /// Return the current UTC date/time with nanosecond precision when supported by the platform clock.
    [[nodiscard]] static auto now() noexcept -> DateTime;
    /// Create a UTC date/time from seconds since the internal epoch.
    /// Returns an invalid date/time if `seconds` is outside the supported date/time range.
    [[nodiscard]] static auto fromSecondsSinceEpoch(Seconds seconds, Nanoseconds fractions = Nanoseconds{}) noexcept
        -> DateTime;
    /// Create a UTC date/time from a duration since the internal epoch.
    /// Returns an invalid date/time if the duration is outside the supported date/time range.
    [[nodiscard]] static auto fromDurationSinceEpoch(const Duration duration) noexcept -> DateTime {
        return fromSecondsSinceEpoch(duration.toSeconds());
    }
    /// Create a UTC date/time from a POSIX time value.
    /// Returns an invalid date/time if the value is outside the supported date/time range.
    [[nodiscard]] static auto fromTimeT(std::time_t posixTime) noexcept -> DateTime;
    /// Create a UTC date/time from POSIX seconds and an optional nanosecond fraction.
    /// Returns an invalid date/time if the value is outside the supported date/time range or if the fraction is
    /// invalid.
    [[nodiscard]] static auto fromPosixTime(Seconds seconds, Nanoseconds fractions = Nanoseconds{}) noexcept
        -> DateTime;
    /// Create a UTC date/time from Windows FILETIME 100-nanosecond ticks since 1601-01-01 UTC.
    /// Returns an invalid date/time if the value is outside the supported date/time range.
    [[nodiscard]] static auto fromWindowsFileTimeTicks(std::uint64_t ticks) noexcept -> DateTime;
    /// Parse an ISO date/time string.
    /// @param text The text to parse.
    /// @param requiredPrecision The minimum precision required.
    /// @return The date-time or an invalid date/time if parsing fails.
    [[nodiscard]] static auto fromIsoString(
        const text::String &text, DateTimePrecision requiredPrecision = DateTimePrecision::Second) noexcept -> DateTime;
    /// Parse an ISO date/time string or throw on errors.
    /// @param text The text to parse.
    /// @param requiredPrecision The minimum precision required.
    /// @return A valid date/time.
    /// @throws err::ParseError If parsing fails.
    [[nodiscard]] static auto fromIsoStringOrThrow(
        const text::String &text, DateTimePrecision requiredPrecision = DateTimePrecision::Second) -> DateTime;
    /// Parse an ISO local date/time string in the given time zone.
    /// @param text The text to parse.
    /// @param timeZone The time zone to interpret the local time.
    /// @param requiredPrecision The minimum precision required.
    /// @return The date-time or an invalid date/time if parsing fails.
    [[nodiscard]] static auto fromIsoString(
        const text::String &text,
        TimeZone timeZone,
        DateTimePrecision requiredPrecision = DateTimePrecision::Second) noexcept -> DateTime;
    /// Parse an ISO local date/time string in the given time zone or throw on errors.
    /// @param text The text to parse.
    /// @param timeZone The time zone to interpret the local time.
    /// @param requiredPrecision The minimum precision required.
    /// @return A valid date/time.
    /// @throws err::ParseError If parsing fails.
    [[nodiscard]] static auto fromIsoStringOrThrow(
        const text::String &text, TimeZone timeZone, DateTimePrecision requiredPrecision = DateTimePrecision::Second)
        -> DateTime;
    /// Get the epoch date-time.
    [[nodiscard]] static auto epoch() noexcept -> DateTime { return DateTime{Date::epoch(), Time{}}; }
    /// Get the first possible date-time.
    [[nodiscard]] static auto first() noexcept -> DateTime { return epoch(); }
    /// Get the last possible date-time.
    [[nodiscard]] static auto last() noexcept -> DateTime { return DateTime{Date::last(), Time::last()}; }
    /// Get the POSIX epoch date-time.
    [[nodiscard]] static auto posixEpoch() noexcept -> DateTime;

private:
    DateTime(const Date date, const Time time, const tz::TimeOffset offset, PrivateTag) noexcept :
        _date{date}, _time{time}, _offset{offset} {}
    [[nodiscard]] auto localDateTime() const noexcept -> std::pair<Date, Time>;
    void subtractOffset() noexcept;
    [[nodiscard]] static auto isoTimeShiftString(Seconds offset, IsoTimeFormatFlags flags) -> text::String;
    [[nodiscard]] static auto posixEpochSecondsDelta() noexcept -> Seconds;

private:
    Date _date;             ///< The date part
    Time _time;             ///< The time part
    tz::TimeOffset _offset; ///< The time offset or time-zone.
};

}

template <>
struct erbsland::text::FormatAsText<erbsland::time::DateTime> : FormatAs<time::DateTime, String> {
    [[nodiscard]] auto format(const time::DateTime &value) const -> String { return value.toIsoString(); }
};

template <>
struct std::formatter<erbsland::time::DateTime> : std::formatter<std::string_view> {
    auto format(const erbsland::time::DateTime value, std::format_context &ctx) const {
        return std::formatter<std::string_view>::format(
            erbsland::text::StringConverter{value.toIsoString()}.toStdString(), ctx);
    }
};
