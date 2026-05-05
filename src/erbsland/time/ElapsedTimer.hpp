// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimePoint.hpp"

namespace erbsland::time {

/// A small helper for measuring elapsed monotonic time.
///
/// Starts timing on construction and provides elapsed time queries.
/// @tested{ElapsedTimerTest}
class ElapsedTimer final {
public:
    /// Start the timer at the current time.
    ElapsedTimer() noexcept : _start{TimePoint::now()} {}

    // defaults
    ~ElapsedTimer() = default;
    ElapsedTimer(const ElapsedTimer &) noexcept = default;
    auto operator=(const ElapsedTimer &) noexcept -> ElapsedTimer & = default;

public:
    /// Restart the timer, resetting the elapsed time to zero.
    void restart() noexcept { _start = TimePoint::now(); }
    /// Return elapsed time since construction or last restart.
    /// @return The elapsed duration.
    [[nodiscard]] auto elapsed() const noexcept -> TimeDelta { return _start.timeDeltaToNow(); }

private:
    TimePoint _start;
};

}
