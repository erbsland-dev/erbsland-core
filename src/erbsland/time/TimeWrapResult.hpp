// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Time.hpp"
#include "TimeWrapResult_fwd.hpp"

namespace erbsland::time {

/// The result of adding to a wall-clock time with wrapping.
/// @tested{TimeCoreTest}
struct TimeWrapResult {
    Time time; ///< The wrapped time of day.
    Days days; ///< The number of day boundaries crossed.

    friend auto operator==(const TimeWrapResult &, const TimeWrapResult &) noexcept -> bool = default;
};

}
