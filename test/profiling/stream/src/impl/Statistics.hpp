// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Statistics_fwd.hpp"

namespace app::stream::impl {

/// Internal Statistics data for stream profiling.
/// @notest{Covered by stream profiler CTest entries.}
struct Statistics {
    std::int64_t minimum{}; ///< Minimum value.
    std::int64_t median{};  ///< Median value.
    std::int64_t p95{};     ///< 95th percentile.
    std::int64_t maximum{}; ///< Maximum value.
    double mean{};          ///< Arithmetic mean.
};

}
