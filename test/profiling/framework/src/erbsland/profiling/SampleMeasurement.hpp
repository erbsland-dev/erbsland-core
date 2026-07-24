// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SampleMeasurement_fwd.hpp"
#include "WorkerMeasurement.hpp"

#include <erbsland/all.hpp>

#include <cstdint>

namespace erbsland::profiling {

/// One synchronized multi-worker sample.
/// @tested{WorkloadRunnerTest}
struct SampleMeasurement {
    List<WorkerMeasurement> workers{}; ///< Per-worker measurements.
    List<std::uint64_t> metrics{};     ///< Aggregate counters.
    std::uint64_t operations{};        ///< Aggregate logical operations.
    TimeDelta wallTime{};              ///< Synchronized sample wall time.
};

}
