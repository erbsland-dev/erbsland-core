// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Statistics_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// Distribution statistics for profiling measurements.
/// @tested{StatisticsTest}
struct Statistics {
    double minimum{}; ///< Minimum value.
    double median{};  ///< Median value.
    double mean{};    ///< Arithmetic mean.
    double p95{};     ///< Nearest-rank 95th percentile.
    double maximum{}; ///< Maximum value.

    /// Calculate statistics for a non-empty value list.
    /// @param values Values to summarize.
    /// @return Calculated statistics.
    /// @throws ApplicationError if the list is empty.
    [[nodiscard]] static auto calculate(List<double> values) -> Statistics;
    /// Normalize a counter to a per-second rate.
    /// @param value Counter value.
    /// @param elapsed Elapsed interval.
    /// @return Counter value per second, or zero for a zero interval.
    [[nodiscard]] static auto normalizedRate(std::uint64_t value, TimeDelta elapsed) noexcept -> double;
    /// Calculate min/max fairness for positive worker rates.
    /// @param rates Per-worker rates.
    /// @return A value in the range zero to one, with one representing equal rates.
    [[nodiscard]] static auto fairness(const List<double> &rates) noexcept -> double;
};

}
