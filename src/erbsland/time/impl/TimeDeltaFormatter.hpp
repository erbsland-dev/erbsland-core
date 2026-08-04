// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TimeAmounts.hpp"
#include "../TimeDeltaFormat.hpp"
#include "../TimeDeltaUnit.hpp"

#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringLiteral.hpp"

#include <cstdint>

namespace erbsland::time::impl {

/// Names and conversion factor for a time-delta unit.
/// @tested{CalendarDeltaTest}
struct TimeDeltaUnitDefinition final {
    TimeDeltaUnit unit;            ///< The represented unit.
    int64_t factor;                ///< The unit size in nanoseconds.
    text::StringLiteral shortName; ///< The regular unit abbreviation.
    text::StringLiteral elclName;  ///< The ELCL unit abbreviation.
    text::StringLiteral singular;  ///< The singular long unit name.
    text::StringLiteral plural;    ///< The plural long unit name.
};

/// Get the formatting definition for a time-delta unit.
/// @param unit The unit to look up.
/// @return The matching formatting definition.
/// @tested{CalendarDeltaTest}
[[nodiscard]] auto timeDeltaUnitDefinition(TimeDeltaUnit unit) noexcept -> const TimeDeltaUnitDefinition &;

/// Append a separator when the result already contains a unit.
/// @param result The destination.
/// @param format The format options.
/// @tested{CalendarDeltaTest}
void appendTimeDeltaUnitSeparator(text::StringEditor &result, const TimeDeltaFormat &format);

/// Append a formatted time-delta unit name.
/// @param result The destination.
/// @param value The signed integral unit value.
/// @param hasFraction Whether a fractional component follows the integral value.
/// @param definition The unit definition.
/// @param format The format options.
/// @tested{CalendarDeltaTest}
void appendTimeDeltaUnitName(
    text::StringEditor &result,
    int64_t value,
    bool hasFraction,
    const TimeDeltaUnitDefinition &definition,
    const TimeDeltaFormat &format);

/// Append a trimmed decimal fractional component.
/// @param result The destination.
/// @param remainder The fractional remainder in nanoseconds.
/// @param factor The containing unit size in nanoseconds.
/// @param maximumDigits The maximum number of fractional digits.
/// @tested{CalendarDeltaTest}
void appendTimeDeltaFraction(text::StringEditor &result, int64_t remainder, int64_t factor, uint8_t maximumDigits);

/// Format a fixed nanosecond amount according to time-delta options.
/// @param value The total fixed amount.
/// @param format The format options.
/// @param includeZero Whether a zero component shall be produced.
/// @return The formatted components.
/// @tested{CalendarDeltaTest}
[[nodiscard]] auto formatTimeDelta(Nanoseconds value, const TimeDeltaFormat &format, bool includeZero = true)
    -> text::String;

/// Normalize and format independently stored fixed calendar-delta amounts without an overflow-prone total.
/// @param nanoseconds The nanosecond amount.
/// @param microseconds The microsecond amount.
/// @param milliseconds The millisecond amount.
/// @param seconds The second amount.
/// @param minutes The minute amount.
/// @param hours The hour amount.
/// @param days The day amount.
/// @param weeks The week amount.
/// @param format The format options.
/// @param includeZero Whether a zero component shall be produced.
/// @return The formatted fixed components.
/// @tested{CalendarDeltaTest}
[[nodiscard]] auto formatCalendarFixedDelta(
    Nanoseconds nanoseconds,
    Microseconds microseconds,
    Milliseconds milliseconds,
    Seconds seconds,
    Minutes minutes,
    Hours hours,
    Days days,
    Weeks weeks,
    const TimeDeltaFormat &format,
    bool includeZero = true) -> text::String;

/// Append an individual calendar unit to an existing result.
/// @param result The destination.
/// @param value The signed amount.
/// @param singular The long singular name.
/// @param plural The long plural name.
/// @param shortName The regular short name.
/// @param elclName The ELCL unit name.
/// @param format The format options.
/// @tested{CalendarDeltaTest}
void appendCalendarDeltaPart(
    text::StringEditor &result,
    int64_t value,
    const text::String &singular,
    const text::String &plural,
    const text::String &shortName,
    const text::String &elclName,
    const TimeDeltaFormat &format);

}
