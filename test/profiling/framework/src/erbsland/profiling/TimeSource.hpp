// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeSource_fwd.hpp"

#include <erbsland/time/TimePoint.hpp>

namespace erbsland::profiling {

/// Injectable monotonic clock used for runner deadlines and progress.
/// @tested{WorkloadRunnerTest}
class TimeSource {
public:
    /// Destroy this time source.
    virtual ~TimeSource();

public:
    /// Return the current monotonic time point.
    [[nodiscard]] virtual auto now() noexcept -> time::TimePoint = 0;
};

}
