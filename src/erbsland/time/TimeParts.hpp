// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Hour.hpp"
#include "Minute.hpp"
#include "Second.hpp"
#include "TimeAmounts.hpp"

namespace erbsland::time {

/// A wall-clock time split into named parts.
/// @tested{TimeCoreTest}
struct TimeParts {
    Hour hour;                      ///< The hour component, range `0..23`.
    Minute minute;                  ///< The minute component, range `0..59`.
    Second second;                  ///< The second component, range `0..59`.
    Nanoseconds nanosecondFraction; ///< The nanosecond fraction, range `0..999999999`.

    friend auto operator==(const TimeParts &, const TimeParts &) noexcept -> bool = default;
};

}
