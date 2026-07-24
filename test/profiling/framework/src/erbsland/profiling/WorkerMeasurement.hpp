// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerMeasurement_fwd.hpp"

#include <erbsland/all.hpp>

#include <cstdint>

namespace erbsland::profiling {

/// Measurements produced by one workload worker.
/// @tested{WorkloadRunnerTest}
struct WorkerMeasurement {
    std::uint64_t operations{};    ///< Completed logical operations.
    List<std::uint64_t> metrics{}; ///< Registered counters in definition order.
    TimeDelta elapsed{};           ///< Worker execution time.
    std::uint64_t sink{};          ///< Optimizer-resistant result sink.
    ByteBlock digest{};            ///< Optional validation digest.
};

}
