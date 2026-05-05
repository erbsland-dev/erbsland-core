// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::time {

/// Flags for ISO date/time formatting.
/// @tested{DateTimeTest}
enum class IsoTimeFormat : uint8_t {
    Extended = 1U << 0U,                ///< Use separators such as `-`, `:`, and extended offsets.
    TimePrefix = 1U << 1U,              ///< Prefix standalone time output with `T`.
    TimeShift = 1U << 2U,               ///< Append the UTC offset for date/time output.
    TimeShiftAlwaysComplete = 1U << 3U, ///< Format zero offset numerically instead of `Z`.
    TimeShiftUpToSeconds = 1U << 4U,    ///< Include offset seconds when needed.
    UseDotFraction = 1U << 5U,          ///< Use `.` instead of `,` before fractional seconds.
    All = (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U) | (1U << 4U) | (1U << 5U),
};

/// A set of ISO date/time formatting flags.
using IsoTimeFormatFlags = util::EnumFlags<IsoTimeFormat>;

/// Default standalone time format.
inline constexpr auto cDefaultTimeFormat = IsoTimeFormatFlags{IsoTimeFormat::Extended};
/// Default date format.
inline constexpr auto cDefaultDateFormat = IsoTimeFormatFlags{IsoTimeFormat::Extended};
/// Default combined date/time format.
inline constexpr auto cDefaultDateTimeFormat = IsoTimeFormatFlags{IsoTimeFormat::Extended};

}
