// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SampleMeasurement_fwd.hpp"
#include "Statistics_fwd.hpp"
#include "WorkerMeasurement_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// Calculated distribution and aggregate statistics for profiling measurements.
/// @tested{StatisticsTest}
class Statistics final {
public:
    /// Calculate distribution statistics for a non-empty value list.
    /// @param values Values to summarize and sort in place.
    /// @throws ApplicationError if the list is empty.
    explicit Statistics(List<double> values);
    /// Calculate benchmark distribution and aggregate statistics in one pass.
    /// @param measurements Non-empty benchmark sample list.
    /// @throws ApplicationError if the list is empty or metric counts differ.
    explicit Statistics(const List<SampleMeasurement> &measurements);

public:
    /// Normalize a counter to a per-second rate.
    /// @param value Counter value.
    /// @param elapsed Elapsed interval.
    /// @return Counter value per second, or zero for a zero interval.
    [[nodiscard]] static auto normalizedRate(std::uint64_t value, TimeDelta elapsed) noexcept -> double;
    /// Calculate min/max fairness for positive rates.
    /// @param rates Rates to compare.
    /// @return A value from zero to one, with one representing equal rates.
    [[nodiscard]] static auto fairness(const List<double> &rates) noexcept -> double;
    /// Calculate min/max fairness for a set of worker measurements.
    /// @param workers Workers to compare by their operation rates.
    /// @return A value from zero to one, with one representing equal rates.
    [[nodiscard]] static auto workerFairness(const List<WorkerMeasurement> &workers) noexcept -> double;

public: // attributes
    /// Minimum value.
    [[nodiscard]] auto minimum() const noexcept -> double { return _minimum; }
    /// Median value.
    [[nodiscard]] auto median() const noexcept -> double { return _median; }
    /// Arithmetic mean.
    [[nodiscard]] auto mean() const noexcept -> double { return _mean; }
    /// Nearest-rank 95th percentile.
    [[nodiscard]] auto p95() const noexcept -> double { return _p95; }
    /// Maximum value.
    [[nodiscard]] auto maximum() const noexcept -> double { return _maximum; }
    /// Fairness across all workers in the benchmark samples.
    [[nodiscard]] auto workerFairness() const noexcept -> double { return _workerFairness; }
    /// Total elapsed wall time across all benchmark samples.
    [[nodiscard]] auto totalElapsed() const noexcept -> TimeDelta { return _totalElapsed; }
    /// Return the aggregate value for a registered metric.
    /// @param index Zero-based metric index.
    [[nodiscard]] auto metricTotal(std::size_t index) const noexcept -> std::uint64_t;

private:
    /// Calculate the distribution from measurement values.
    void calculateDistribution(List<double> values);
    /// Calculate fairness from the observed minimum and maximum.
    [[nodiscard]] static auto calculateFairness(double minimum, double maximum, bool hasRates) noexcept -> double;

private:
    double _minimum{};                 ///< Minimum value.
    double _median{};                  ///< Median value.
    double _mean{};                    ///< Arithmetic mean.
    double _p95{};                     ///< Nearest-rank 95th percentile.
    double _maximum{};                 ///< Maximum value.
    double _workerFairness{1.0};       ///< Fairness across benchmark workers.
    TimeDelta _totalElapsed{};         ///< Total benchmark wall time.
    List<std::uint64_t> _metricTotals; ///< Aggregate metric values.
};

}
