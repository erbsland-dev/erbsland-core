// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SteadyTimeSource_fwd.hpp"
#include "TimeSource.hpp"

namespace erbsland::profiling {

/// Production monotonic runner clock.
/// @notest{Thin TimePoint::now adapter; injection is tested by WorkloadRunnerTest.}
class SteadyTimeSource final : public TimeSource {
public: // implement TimeSource
    [[nodiscard]] auto now() noexcept -> time::TimePoint override;
};

}
