// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThroughputStatistics_fwd.hpp"

namespace app::stream::impl {

/// Internal ThroughputStatistics data for stream profiling.
/// @notest{Covered by stream profiler CTest entries.}
struct ThroughputStatistics {
    double minimum{}; ///< Minimum throughput.
    double median{};  ///< Median throughput.
    double mean{};    ///< Mean throughput.
    double p95{};     ///< 95th percentile.
    double maximum{}; ///< Maximum throughput.
};

}
