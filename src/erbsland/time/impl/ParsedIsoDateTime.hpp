// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Date.hpp"
#include "../DateTimePrecision.hpp"
#include "../Time.hpp"
#include "../TimeAmounts.hpp"

namespace erbsland::time::impl {

/// Parsed ISO date/time fields.
///
/// Holds the results of parsing an ISO 8601 date/time string.
/// @tested{DateTimeTest}
struct ParsedIsoDateTime final {
    Date date;                   ///< The parsed date.
    Time time;                   ///< The parsed time.
    DateTimePrecision precision; ///< The parsed precision.
    bool hasOffset{false};       ///< `true` if an explicit UTC offset was parsed.
    Seconds offset;              ///< The parsed UTC offset.
};

}
