// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FakeTimeSource_fwd.hpp"

#include <erbsland/profiling/TimeSource.hpp>

/// A deterministic monotonic clock for profiling framework tests.
/// @tested{WorkloadRunnerTest}
class FakeTimeSource final : public erbsland::profiling::TimeSource {
public:
    /// Create a clock that advances by `step` on each read.
    explicit FakeTimeSource(erbsland::time::TimeDelta step);

public: // implement TimeSource
    [[nodiscard]] auto now() noexcept -> erbsland::time::TimePoint override;

private:
    erbsland::time::TimePoint _current;
    erbsland::time::TimeDelta _step;
};
