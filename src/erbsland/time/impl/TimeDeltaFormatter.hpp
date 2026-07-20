// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TimeAmounts.hpp"
#include "../TimeDeltaFormat.hpp"

#include "../../text/String.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::time::impl {

/// Format a fixed nanosecond amount according to time-delta options.
/// @param value The total fixed amount.
/// @param format The format options.
/// @param includeZero Whether a zero component shall be produced.
/// @return The formatted components.
[[nodiscard]] auto formatTimeDelta(Nanoseconds value, const TimeDeltaFormat &format, bool includeZero = true)
    -> text::String;

/// Normalize and format independently stored fixed calendar-delta amounts without an overflow-prone total.
/// @return The formatted fixed components.
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
void appendCalendarDeltaPart(
    text::StringEditor &result,
    int64_t value,
    const text::String &singular,
    const text::String &plural,
    const text::String &shortName,
    const text::String &elclName,
    const TimeDeltaFormat &format);

}
