// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time {

/// Which local occurrence to choose when a wall clock time appears twice
/// (during the fall-back gap when daylight saving time ends).
///
/// When clocks are set back, the same local time occurs twice. This enum disambiguates which occurrence to use.
/// @tested{DateTimeTest}
enum class TimeOccurrenceInFold : uint8_t {
    First,
    Second,
};

}
