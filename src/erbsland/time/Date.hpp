// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CalendarParts.hpp"
#include "DateTimePrecision.hpp"
#include "Day.hpp"
#include "DayOfWeek.hpp"
#include "DayOfYear.hpp"
#include "IsoTimeFormat.hpp"
#include "Month.hpp"
#include "Time.hpp"
#include "TimeAmounts.hpp"
#include "Year.hpp"

#include "../text/FormatAs.hpp"
#include "../text/String.hpp"
#include "../text/StringConverter.hpp"
#include "../text/StringEditor.hpp"

#include <compare>
#include <cstdint>

namespace erbsland::time {

/// A date in the proleptic Gregorian calendar using `0000-01-01` as epoch.
///
/// Supports years 0-9999. Invalid dates are represented by a special internal value
/// and sort before all valid dates in comparisons.
/// @seedoc{/reference/time/date_and_time}
/// @tested{DateTest}
class Date final {
    struct PrivateTag {};
    using Storage = math::SatInt32;

public:
    /// Create an invalid date.
    Date() noexcept = default;
    /// Create a date from parts, or an invalid date if the parts do not exist.
    ///
    /// The year, month and day part types clamp their raw construction input first. The resulting combination is then
    /// checked as a real calendar date; for example, February 31st becomes an invalid date.
    /// @param year The year.
    /// @param month The month.
    /// @param day The day.
    Date(Year year, Month month, Day day) noexcept;
    /// @overload
    Date(Day day, Month month, Year year) noexcept;
    /// @overload
    Date(Year year, Day day, Month month) noexcept;

    // defaults
    ~Date() = default;
    Date(const Date &) noexcept = default;
    auto operator=(const Date &) noexcept -> Date & = default;
    Date(Date &&) noexcept = default;
    auto operator=(Date &&) noexcept -> Date & = default;

public: // operators
    /// Compare dates. Invalid dates sort before valid dates.
    [[nodiscard]] auto operator<=>(const Date &other) const noexcept -> std::strong_ordering = default;

public: // tests
    /// Test if this date is valid.
    /// @return `true` if the date represents a real calendar date.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _days >= 0; }
    /// Test if this is the first valid date (epoch).
    /// @return `true` if this is `0000-01-01`.
    [[nodiscard]] auto isFirst() const noexcept -> bool;
    /// Test if this is the last valid date (9999-12-31).
    /// @return `true` if this is the maximum supported date.
    [[nodiscard]] auto isLast() const noexcept -> bool;
    /// Test if adding the given number of days would saturate the date to the first/last valid date.
    /// The method only returns `true` if the date is clamped to the first or last valid date,
    /// not if it ends naturally on the first or last day of the year.
    /// Returns `false` for invalid dates.
    /// @param days The number of days to add.
    /// @return `true` if the date would saturate to the first or last valid date.
    [[nodiscard]] auto wouldAddSaturate(Days days) const -> bool;
    /// Test if adding months would saturate the date to the first/last valid date.
    /// Returns `false` for invalid dates.
    /// @param months The number of months to add.
    /// @return `true` if the date would saturate to the first or last valid date.
    [[nodiscard]] auto wouldAddSaturate(Months months) const -> bool;
    /// Test if adding years would saturate the date to the first/last valid date.
    /// Returns `false` for invalid dates.
    /// @param years The number of years to add.
    /// @return `true` if the date would saturate to the first or last valid date.
    [[nodiscard]] auto wouldAddSaturate(Years years) const -> bool;

public: // accessors
    /// Get the year, or zero for invalid dates.
    /// @return The year component.
    [[nodiscard]] auto year() const noexcept -> Year;
    /// Get the month, or January for invalid dates.
    /// @return The month component.
    [[nodiscard]] auto month() const noexcept -> Month;
    /// Get the day, or one for invalid dates.
    /// @return The day component.
    [[nodiscard]] auto day() const noexcept -> Day;
    /// Get the one-based day of year.
    /// @return The day of year (1-366).
    [[nodiscard]] auto dayOfYear() const noexcept -> DayOfYear;
    /// Get the day of week.
    /// @return The day of week (Monday - Sunday).
    [[nodiscard]] auto dayOfWeek() const noexcept -> DayOfWeek;
    /// Get all date parts.
    /// For invalid dates, returns `0000-01-01`, matching the individual accessors.
    /// @return Named year, month, and day parts.
    [[nodiscard]] auto parts() const noexcept -> DateParts;

public: // manipulation
    /// Return this date plus `amount` days, clamped to valid bounds.
    /// Invalid dates stay invalid.
    /// @param amount The number of days to add.
    /// @return The resulting date.
    [[nodiscard]] auto added(Days amount) const noexcept -> Date;
    /// Return this date plus `amount` days.
    /// Invalid dates stay invalid.
    /// @param amount The number of days to add.
    /// @return The resulting date.
    /// @throws err::OverflowError if the result would be outside the supported date range.
    [[nodiscard]] auto addedOrThrow(Days amount) const -> Date;
    /// Return this date plus `amount` months, clamped to valid bounds.
    /// Invalid dates stay invalid.
    /// A day is clamped to the largest possible day of the resulting month.
    /// @param amount The number of months to add.
    /// @return The resulting date.
    [[nodiscard]] auto added(Months amount) const noexcept -> Date;
    /// Return this date plus `amount` months.
    /// Invalid dates stay invalid.
    /// A day is clamped to the largest possible day of the resulting month.
    /// @param amount The number of months to add.
    /// @return The resulting date.
    /// @throws err::OverflowError if the result would be outside the supported date range.
    [[nodiscard]] auto addedOrThrow(Months amount) const -> Date;
    /// Return this date plus `amount` years, clamped to valid bounds.
    /// Invalid dates stay invalid.
    /// @param amount The number of years to add.
    /// @return The resulting date.
    [[nodiscard]] auto added(Years amount) const noexcept -> Date;
    /// Return this date plus `amount` years.
    /// Invalid dates stay invalid.
    /// @param amount The number of years to add.
    /// @return The resulting date.
    /// @throws err::OverflowError if the result would be outside the supported date range.
    [[nodiscard]] auto addedOrThrow(Years amount) const -> Date;
    /// Add days in place.
    /// @param amount The number of days to add.
    void add(Days amount) noexcept;
    /// Add days in place.
    /// @param amount The number of days to add.
    /// @throws err::OverflowError if the result would be outside the supported date range.
    void addOrThrow(Days amount);
    /// Add months in place.
    /// A day is clamped to the largest possible day of the resulting month.
    /// @param amount The number of months to add.
    void add(Months amount) noexcept;
    /// Add months in place.
    /// A day is clamped to the largest possible day of the resulting month.
    /// @param amount The number of months to add.
    /// @throws err::OverflowError if the result would be outside the supported date range.
    void addOrThrow(Months amount);
    /// Add years in place.
    /// @param amount The number of years to add.
    void add(Years amount) noexcept;
    /// Add years in place.
    /// @param amount The number of years to add.
    /// @throws err::OverflowError if the result would be outside the supported date range.
    void addOrThrow(Years amount);
    /// Return the following date, or invalid past the supported range.
    /// @return The next calendar day.
    [[nodiscard]] auto next() const noexcept -> Date;
    /// Return the previous date, or invalid before the supported range.
    /// @return The previous calendar day.
    [[nodiscard]] auto previous() const noexcept -> Date;
    /// Return the next requested day of week, or an invalid date if this date is invalid.
    [[nodiscard]] auto next(DayOfWeek dayOfWeek) const noexcept -> Date;
    /// Return the previous requested day of week, or an invalid date if this date is invalid.
    [[nodiscard]] auto previous(DayOfWeek dayOfWeek) const noexcept -> Date;

public: // conversion
    /// Convert to days since epoch, or `-1` for invalid dates.
    [[nodiscard]] auto toDaysSinceEpoch() const noexcept -> Days { return Days{_days}; }
    /// Calculate the number of days to another date.
    /// Returns zero if either date is invalid.
    [[nodiscard]] auto daysTo(Date other) const noexcept -> Days;
    /// Convert this date to an ISO 8601 string.
    /// Invalid dates return an empty string.
    /// @param flags Formatting flags for the output.
    /// @param precision The largest precision to include.
    /// @return The ISO-formatted date string.
    [[nodiscard]] auto toIsoString(
        IsoTimeFormatFlags flags = cDefaultDateFormat, DateTimePrecision precision = DateTimePrecision::Day) const
        -> text::String;

public:
    /// Create a date from raw integer parts.
    /// @param year The year (0-9999).
    /// @param month The month (1-12).
    /// @param day The day (1-31).
    /// @return A valid date if the parts form a real calendar date, otherwise invalid.
    [[nodiscard]] static auto fromYearMonthDay(int year, int month, int day) noexcept -> Date;
    /// Create a date from raw integer parts or throw.
    /// @param year The year (0-9999).
    /// @param month The month (1-12).
    /// @param day The day (1-31).
    /// @return A valid date.
    /// @throws err::OutOfRangeError if the parts do not form a valid date.
    [[nodiscard]] static auto fromYearMonthDayOrThrow(int year, int month, int day) -> Date;
    /// Create a date from typed parts.
    /// @param year The year.
    /// @param month The month, defaults to January.
    /// @param day The day, defaults to the first.
    /// @return A valid date if the parts form a real calendar date, otherwise invalid.
    [[nodiscard]] static auto fromParts(Year year, Month month = Month{}, Day day = Day{}) noexcept -> Date;
    /// Create a date from typed parts or throw.
    /// @param year The year.
    /// @param month The month, defaults to January.
    /// @param day The day, defaults to the first.
    /// @return A valid date.
    /// @throws err::OutOfRangeError if the parts do not form a valid date.
    [[nodiscard]] static auto fromPartsOrThrow(Year year, Month month = Month{}, Day day = Day{}) -> Date;
    /// Create a date from days since epoch.
    /// @param days The number of days since epoch.
    /// @return A valid date if within range, otherwise invalid.
    [[nodiscard]] static auto fromDaysSinceEpoch(Days days) noexcept -> Date;
    /// Test if date parts form an existing date.
    /// @param year The year.
    /// @param month The month.
    /// @param day The day.
    /// @return `true` if the parts form a valid calendar date.
    [[nodiscard]] static auto exists(Year year, Month month, Day day) noexcept -> bool;
    /// Return first day in a year.
    /// @param year The year.
    /// @return January 1st of the given year.
    [[nodiscard]] static auto firstDay(Year year) noexcept -> Date;
    /// Return first day in a month.
    /// @param year The year.
    /// @param month The month.
    /// @return The first day of the given month.
    [[nodiscard]] static auto firstDay(Year year, Month month) noexcept -> Date;
    /// Return last day in a year.
    /// @param year The year.
    /// @return December 31st of the given year.
    [[nodiscard]] static auto lastDay(Year year) noexcept -> Date;
    /// Return last day in a month.
    /// @param year The year.
    /// @param month The month.
    /// @return The last day of the given month.
    [[nodiscard]] static auto lastDay(Year year, Month month) noexcept -> Date;
    /// Return the epoch date.
    /// @return `0000-01-01`.
    [[nodiscard]] static auto epoch() noexcept -> Date;
    /// Return first supported date.
    [[nodiscard]] static auto first() noexcept -> Date;
    /// Return the last supported date (9999-12-31).
    /// @return The maximum date.
    [[nodiscard]] static auto last() noexcept -> Date;

private:
    /// Create a date from raw days since epoch.
    Date(const math::SatInt32 rawDays, PrivateTag) noexcept : _days{rawDays} {}
    /// Test if the given parts represent a valid date.
    [[nodiscard]] static auto containsParts(int year, int month, int day) noexcept -> bool;
    /// Get the raw number of days since epoch.
    [[nodiscard]] static auto daysFromParts(Year year, Month month, Day day) noexcept -> Days;

private:
    Storage _days{-1}; ///< Days since epoch; `-1` marks invalid.
};

}

template <>
struct erbsland::text::FormatAsText<erbsland::time::Date> : FormatAs<time::Date, String> {
    [[nodiscard]] auto format(const time::Date &value) const -> String { return value.toIsoString(); }
};

template <>
struct std::formatter<erbsland::time::Date> : std::formatter<std::string_view> {
    auto format(const erbsland::time::Date value, std::format_context &ctx) const {
        return std::formatter<std::string_view>::format(
            erbsland::text::StringConverter{value.toIsoString()}.toStdString(), ctx);
    }
};
