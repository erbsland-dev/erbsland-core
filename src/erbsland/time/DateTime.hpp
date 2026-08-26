// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CalendarDelta.hpp"
#include "Date.hpp"
#include "DateTime_fwd.hpp"
#include "DateTimeParts.hpp"
#include "Duration.hpp"
#include "Time.hpp"
#include "TimeOccurrenceInFold.hpp"
#include "TimeWithZone.hpp"
#include "TimeZone.hpp"

#include "impl/DateTimeEpochs.hpp"
#include "impl/DateTimeTraits.hpp"
#include "tz/TimeOffset.hpp"

#include "../text/String_fwd.hpp"

#include <compare>
#include <cstdint>
#include <ctime>
#include <optional>
#include <utility>

namespace erbsland::time {

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
    /// Create a local date/time from a time with a time zone.
    /// Invalid local dates result in an invalid date/time.
    /// @param localDate The local date.
    /// @param localTime The local time and its time zone.
    /// @param occurrence Which occurrence to use during fold periods.
    DateTime(
        Date localDate, TimeWithZone localTime, TimeOccurrenceInFold occurrence = TimeOccurrenceInFold::First) noexcept;
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
    /// Compare two date/time values by their instant and display properties.
    [[nodiscard]] auto operator<=>(const DateTime &other) const noexcept -> std::strong_ordering;
    [[nodiscard]] auto operator==(const DateTime &other) const noexcept -> bool = default;
    /// Return this date/time with a duration added.
    [[nodiscard]] auto operator+(const Duration duration) const noexcept -> DateTime { return added(duration); }
    /// Add a duration to this date/time.
    auto operator+=(const Duration duration) noexcept -> DateTime & {
        add(duration);
        return *this;
    }
    /// Return this date/time with a duration subtracted.
    [[nodiscard]] auto operator-(const Duration duration) const noexcept -> DateTime { return subtracted(duration); }
    /// Subtract a duration from this date/time.
    auto operator-=(const Duration duration) noexcept -> DateTime & {
        subtract(duration);
        return *this;
    }
    /// Return the signed duration from `other` to this date/time.
    /// @param other The other date/time.
    /// @return The duration from `other` to this.
    [[nodiscard]] auto operator-(const DateTime &other) const noexcept -> Duration { return other.durationTo(*this); }
    /// Return this date/time with a calendar delta added.
    [[nodiscard]] auto operator+(const CalendarDelta &delta) const noexcept -> DateTime { return added(delta); }
    /// Add a calendar delta to this date/time.
    auto operator+=(const CalendarDelta &delta) noexcept -> DateTime & {
        add(delta);
        return *this;
    }
    /// Return this date/time with a calendar delta subtracted.
    [[nodiscard]] auto operator-(const CalendarDelta &delta) const noexcept -> DateTime { return subtracted(delta); }
    /// Subtract a calendar delta from this date/time.
    auto operator-=(const CalendarDelta &delta) noexcept -> DateTime & {
        subtract(delta);
        return *this;
    }

public: // tests
    /// Test if this date/time represents a valid instant.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _date.isValid(); }
    /// Test if this date/time is displayed in UTC.
    [[nodiscard]] constexpr auto isUtc() const noexcept -> bool { return _offset.isUtc(); }
    /// Test if the display zone originated from the system-local setting.
    [[nodiscard]] constexpr auto isLocalTime() const noexcept -> bool { return _offset.isLocalTime(); }

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
    [[nodiscard]] auto timeZone() const noexcept -> TimeZone;
    /// Return the display time-zone abbreviation, or an empty string when none is available.
    [[nodiscard]] auto timeZoneAbbreviation() const -> text::String;

public: // manipulation
    /// Test if applying a calendar delta would saturate the supported range.
    [[nodiscard]] auto wouldAddSaturate(const CalendarDelta &delta) const noexcept -> bool;
    /// Test if subtracting a calendar delta would saturate the supported range.
    [[nodiscard]] auto wouldSubtractSaturate(const CalendarDelta &delta) const noexcept -> bool;
    /// Apply a calendar delta in ascending unit order and clamp at supported bounds.
    [[nodiscard]] auto added(const CalendarDelta &delta) const noexcept -> DateTime;
    /// Apply a calendar delta in ascending unit order.
    /// @throws err::OverflowError if an intermediate result exceeds supported bounds.
    [[nodiscard]] auto addedOrThrow(const CalendarDelta &delta) const -> DateTime;
    /// Apply a calendar delta in place and clamp at supported bounds.
    void add(const CalendarDelta &delta) noexcept { *this = added(delta); }
    /// Apply a calendar delta in place.
    /// @throws err::OverflowError if an intermediate result exceeds supported bounds.
    void addOrThrow(const CalendarDelta &delta) { *this = addedOrThrow(delta); }
    /// Subtract a calendar delta in ascending unit order and clamp at supported bounds.
    [[nodiscard]] auto subtracted(const CalendarDelta &delta) const noexcept -> DateTime { return added(-delta); }
    /// Subtract a calendar delta in ascending unit order.
    /// @throws err::OverflowError if an intermediate result exceeds supported bounds.
    [[nodiscard]] auto subtractedOrThrow(const CalendarDelta &delta) const -> DateTime { return addedOrThrow(-delta); }
    /// Subtract a calendar delta in place and clamp at supported bounds.
    void subtract(const CalendarDelta &delta) noexcept { *this = subtracted(delta); }
    /// Subtract a calendar delta in place.
    /// @throws err::OverflowError if an intermediate result exceeds supported bounds.
    void subtractOrThrow(const CalendarDelta &delta) { *this = subtractedOrThrow(delta); }

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
    /// Convert this date/time to a compact human-readable representation.
    /// Invalid date/times return an empty string.
    /// @return The displayed date/time followed by its resolved offset.
    [[nodiscard]] auto toString() const -> text::String;
    /// Convert to UTC, or return an invalid date/time if this date/time is invalid.
    [[nodiscard]] auto toUtc() const noexcept -> DateTime;
    /// Convert the display time zone, or return an invalid date/time if this date/time is invalid.
    [[nodiscard]] auto toTimeZone(TimeZone timeZone) const noexcept -> DateTime;
    /// Convert to `std::time_t` using the POSIX epoch.
    /// Fractions of seconds are discarded.
    /// Invalid date/times convert from the internal `-1` seconds sentinel.
    [[nodiscard]] auto toTimeT() const noexcept -> std::time_t;
    /// Convert to complete seconds and a nanosecond fraction from an epoch.
    /// This is the only method that returns the full precision of a date/time value.
    /// @param epoch The epoch.
    /// @return The non-negative seconds and fraction, or no value if this date/time precedes the epoch or is invalid.
    [[nodiscard]] auto toSecondsAndFractions(TimeEpoch epoch = TimeEpoch::Core) const noexcept
        -> std::optional<std::pair<Seconds, Nanoseconds>>;
    /// Convert to complete seconds and a nanosecond fraction from an epoch.
    /// This is the only method that returns the full precision of a date/time value.
    /// @param epoch The epoch.
    /// @return The non-negative seconds and fraction.
    /// @throws err::OutOfRangeError If this date/time precedes the epoch or is invalid.
    [[nodiscard]] auto toSecondsAndFractionsOrThrow(TimeEpoch epoch = TimeEpoch::Core) const
        -> std::pair<Seconds, Nanoseconds>;
    /// Convert to exact ticks in seconds, milliseconds, microseconds or nanoseconds from an epoch.
    /// Fractions outside the precision of the tick unit are discarded.
    /// @tparam tUnit The tick unit: Nanoseconds, Microseconds, Milliseconds, or Seconds.
    /// @param epoch The epoch.
    /// @return Exact non-negative ticks, or no value if they cannot represent this date/time.
    template <typename tUnit>
        requires impl::DateTimeTickUnit<tUnit>
    [[nodiscard]] auto toTicks(TimeEpoch epoch = TimeEpoch::Core) const noexcept -> std::optional<tUnit>;
    /// @overload
    [[nodiscard]] auto toSeconds(const TimeEpoch epoch = TimeEpoch::Core) const noexcept -> std::optional<Seconds> {
        return toTicks<Seconds>(epoch);
    }
    /// @overload
    [[nodiscard]] auto toNanoseconds(const TimeEpoch epoch = TimeEpoch::Core) const noexcept
        -> std::optional<Nanoseconds> {
        return toTicks<Nanoseconds>(epoch);
    }
    /// Convert to exact ticks in seconds, milliseconds, microseconds or nanoseconds from an epoch.
    /// Fractions outside the precision of the tick unit are discarded.
    /// @tparam tUnit The tick unit: Nanoseconds, Microseconds, Milliseconds, or Seconds.
    /// @param epoch The epoch.
    /// @return Exact non-negative ticks.
    /// @throws err::OutOfRangeError If the ticks cannot represent this date/time.
    template <typename tUnit>
        requires impl::DateTimeTickUnit<tUnit>
    [[nodiscard]] auto toTicksOrThrow(TimeEpoch epoch = TimeEpoch::Core) const -> tUnit;
    /// @overload
    [[nodiscard]] auto toSecondsOrThrow(const TimeEpoch epoch = TimeEpoch::Core) const -> Seconds {
        return toTicksOrThrow<Seconds>(epoch);
    }
    /// @overload
    [[nodiscard]] auto toNanosecondsOrThrow(const TimeEpoch epoch = TimeEpoch::Core) const -> Nanoseconds {
        return toTicksOrThrow<Nanoseconds>(epoch);
    }
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
    /// Create a UTC date/time from a POSIX time value.
    /// Returns an invalid date/time if the value is outside the supported date/time range.
    [[nodiscard]] static auto fromTimeT(std::time_t posixTime) noexcept -> DateTime;
    /// Create a UTC date/time from exact ticks in seconds, milliseconds, microseconds or nanoseconds since an epoch.
    /// Fractions outside the precision of the tick unit are discarded.
    /// @tparam tUnit The tick unit: Nanoseconds, Microseconds, Milliseconds, or Seconds.
    /// @param ticks The non-negative ticks.
    /// @param epoch The epoch.
    /// @return A date/time, or no value if the ticks cannot be represented.
    template <typename tUnit>
        requires impl::DateTimeTickUnit<tUnit>
    [[nodiscard]] static auto fromTicks(tUnit ticks, TimeEpoch epoch = TimeEpoch::Core) noexcept
        -> std::optional<DateTime>;
    /// @overload
    [[nodiscard]] static auto fromSeconds(const Seconds ticks, const TimeEpoch epoch = TimeEpoch::Core) noexcept
        -> std::optional<DateTime> {
        return fromTicks<Seconds>(ticks, epoch);
    }
    /// @overload
    [[nodiscard]] static auto fromNanoseconds(const Nanoseconds ticks, const TimeEpoch epoch = TimeEpoch::Core) noexcept
        -> std::optional<DateTime> {
        return fromTicks<Nanoseconds>(ticks, epoch);
    }
    /// Create a UTC date/time from exact ticks in seconds, milliseconds, microseconds or nanoseconds since an epoch.
    /// Fractions outside the precision of the tick unit are discarded.
    /// @tparam tUnit The tick unit: Nanoseconds, Microseconds, Milliseconds, or Seconds.
    /// @param ticks The non-negative ticks.
    /// @param epoch The epoch.
    /// @return A date/time.
    /// @throws err::OutOfRangeError If the ticks cannot be represented.
    template <typename tUnit>
        requires impl::DateTimeTickUnit<tUnit>
    [[nodiscard]] static auto fromTicksOrThrow(tUnit ticks, TimeEpoch epoch = TimeEpoch::Core) -> DateTime;
    /// @overload
    [[nodiscard]] static auto fromSecondsOrThrow(const Seconds ticks, const TimeEpoch epoch = TimeEpoch::Core)
        -> DateTime {
        return fromTicksOrThrow<Seconds>(ticks, epoch);
    }
    /// @overload
    [[nodiscard]] static auto fromNanosecondsOrThrow(const Nanoseconds ticks, const TimeEpoch epoch = TimeEpoch::Core)
        -> DateTime {
        return fromTicksOrThrow<Nanoseconds>(ticks, epoch);
    }
    /// Create a UTC date/time from complete seconds and a nanosecond fraction since an epoch.
    /// This is the only method that allows to construct a date/time at it's full range and precision.
    /// @param seconds The non-negative complete seconds.
    /// @param fractions The nanosecond fraction in the range 0 to 999999999.
    /// @param epoch The epoch.
    /// @return A date/time, or no value if the values cannot be represented.
    [[nodiscard]] static auto fromTicks(
        Seconds seconds, Nanoseconds fractions, TimeEpoch epoch = TimeEpoch::Core) noexcept -> std::optional<DateTime>;
    /// Create a UTC date/time from complete seconds and a nanosecond fraction since an epoch.
    /// This is the only method that allows to construct a date/time at it's full range and precision.
    /// @param seconds The non-negative complete seconds.
    /// @param fractions The nanosecond fraction in the range 0 to 999999999.
    /// @param epoch The epoch.
    /// @return A date/time.
    /// @throws err::OutOfRangeError If the values cannot be represented.
    [[nodiscard]] static auto fromTicksOrThrow(
        Seconds seconds, Nanoseconds fractions, TimeEpoch epoch = TimeEpoch::Core) -> DateTime;
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
    /// Get an epoch date-time.
    /// @param epoch The epoch.
    /// @return The epoch date-time.
    [[nodiscard]] static auto epoch(TimeEpoch epoch = TimeEpoch::Core) noexcept -> DateTime;
    /// Get the first possible date-time.
    [[nodiscard]] static auto first() noexcept -> DateTime { return epoch(); }
    /// Get the last possible date-time.
    [[nodiscard]] static auto last() noexcept -> DateTime { return DateTime{Date::last(), Time::last()}; }

private:
    /// Construct a date-time with an explicitly resolved time-zone offset.
    DateTime(const Date date, const Time time, const tz::TimeOffset offset, PrivateTag) noexcept :
        _date{date}, _time{time}, _offset{offset} {}
    /// Return the local date and time before applying the stored offset.
    [[nodiscard]] auto localDateTime() const noexcept -> std::pair<Date, Time>;
    /// Subtract the stored offset from the local date and time.
    void subtractOffset() noexcept;
    /// Format an ISO UTC offset suffix.
    [[nodiscard]] static auto isoTimeShiftString(Seconds offset, IsoTimeFormatFlags flags) -> text::String;
    /// Throw when a date-time cannot be represented as ticks.
    [[noreturn]] static void throwDateTimeNotTickConvertible();
    /// Throw when a tick conversion receives a negative value.
    [[noreturn]] static void throwTicksMustNotBeNegative();

private:
    Date _date;             ///< The date part
    Time _time;             ///< The time part
    tz::TimeOffset _offset; ///< The time offset or time-zone.
};

}

#include "DateTime.tpp"
