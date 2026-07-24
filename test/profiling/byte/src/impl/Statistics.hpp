// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Statistics_fwd.hpp"

namespace app::byte::impl {

/// Summary statistics for one byte-profiler measurement series.
/// @notest{Covered by byte profiler benchmark CTest entries.}
struct Statistics {
    double minimum{}; ///< Minimum value.
    double median{};  ///< Median value.
    double mean{};    ///< Arithmetic mean.
    double p95{};     ///< 95th percentile.
    double maximum{}; ///< Maximum value.
};

}
