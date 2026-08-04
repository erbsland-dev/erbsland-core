// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeAmounts.hpp"

namespace erbsland::time {

/// Store all independently configurable calendar-delta parts.
/// @tested{CalendarDeltaTest}
struct CalendarDeltaParts {
    Nanoseconds nanoseconds;   ///< The nanosecond part.
    Microseconds microseconds; ///< The microsecond part.
    Milliseconds milliseconds; ///< The millisecond part.
    Seconds seconds;           ///< The second part.
    Minutes minutes;           ///< The minute part.
    Hours hours;               ///< The hour part.
    Days days;                 ///< The day part.
    Weeks weeks;               ///< The week part.
    Months months;             ///< The month part.
    Years years;               ///< The year part.

    friend auto operator==(const CalendarDeltaParts &, const CalendarDeltaParts &) noexcept -> bool = default;
};

}
