// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::time {

/// Precision levels for ISO date/time parsing and formatting.
///
/// Controls how much detail is required when parsing or how much is emitted when formatting.
enum class DateTimePrecision : uint8_t {
    /// Year only (e.g. `2025`).
    Year,
    /// Year and month (e.g. `2025-06`).
    Month,
    /// Full date (e.g. `2025-06-15`).
    Day,
    /// Full date and hour (e.g. `2025-06-15T14`).
    Hour,
    /// Full date and minute (e.g. `2025-06-15T14:30`).
    Minute,
    /// Full date and second (e.g. `2025-06-15T14:30:45`).
    Second,
    /// Full date and time with a millisecond fraction (e.g. `2025-06-15T14:30:45.123`).
    Millisecond,
    /// Full date and time with a microsecond fraction (e.g. `2025-06-15T14:30:45.123456`).
    Microsecond,
    /// Full date and time with a nanosecond fraction (e.g. `2025-06-15T14:30:45.123456789`).
    Nanosecond,
};

}
