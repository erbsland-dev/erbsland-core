// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Definitions.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/TimePoint.hpp>

#include <optional>

namespace app::constant_time {

/// Shared bounded-run parameters.
/// @tested{ConstantTimeRunnerTest}
struct RunOptions final {
    el::TimeDelta duration{el::Seconds{60}}; ///< Per-experiment wall-time budget.
    uint64_t maximumSamples{};               ///< Zero means no extra sample limit.
    uint64_t seed{};                         ///< Reported reproducible seed.
    std::optional<el::TimePoint> start{};    ///< Optional start before factory setup.
};

}
