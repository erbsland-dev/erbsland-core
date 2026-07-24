// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WorkerFairness_fwd.hpp"

namespace app::stream::impl {

/// Internal WorkerFairness data for stream profiling.
/// @notest{Covered by stream profiler CTest entries.}
struct WorkerFairness {
    double minimum{};                ///< Minimum worker rate.
    double maximum{};                ///< Maximum worker rate.
    double coefficientOfVariation{}; ///< Coefficient of variation across workers.
};

}
