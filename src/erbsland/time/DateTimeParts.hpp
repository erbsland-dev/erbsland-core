// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeAmounts.hpp"

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

}
