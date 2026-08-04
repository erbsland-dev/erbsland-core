// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Statistics.hpp"

#include "SampleMeasurement.hpp"
#include "WorkerMeasurement.hpp"

#include <cmath>
#include <limits>

namespace erbsland::profiling {

Statistics::Statistics(List<double> values) {
    calculateDistribution(std::move(values));
}

Statistics::Statistics(const List<SampleMeasurement> &measurements) {
    using namespace text::literals;
    if (measurements.isEmpty()) {
        throw ApplicationError{"Cannot calculate statistics for an empty measurement list."_el};
    }
    auto values = List<double>{};
    values.reserve(measurements.count());
    const auto metricCount = measurements.first().metrics.count();
    _metricTotals.resize(metricCount, 0U);
    auto minimumWorkerRate = std::numeric_limits<double>::max();
    auto maximumWorkerRate = 0.0;
    auto hasWorkerRates = false;
    for (const auto &measurement : measurements) {
        if (measurement.metrics.count() != metricCount) {
            throw ApplicationError{"Benchmark samples contain different metric counts."_el};
        }
        const auto operations = std::max<std::uint64_t>(1U, measurement.operations);
        values.append(
            static_cast<double>(measurement.wallTime.toNanoseconds().toRawValue()) / static_cast<double>(operations));
        _totalElapsed += measurement.wallTime;
        for (auto index = std::size_t{}; index < metricCount.toSizeT(); ++index) {
            _metricTotals.set(
                ItemIndex{index}, _metricTotals.toRawValue()[index] + measurement.metrics.toRawValue()[index]);
        }
        for (const auto &worker : measurement.workers) {
            const auto rate = normalizedRate(worker.operations, worker.elapsed);
            minimumWorkerRate = std::min(minimumWorkerRate, rate);
            maximumWorkerRate = std::max(maximumWorkerRate, rate);
            hasWorkerRates = true;
        }
    }
    _workerFairness = calculateFairness(minimumWorkerRate, maximumWorkerRate, hasWorkerRates);
    calculateDistribution(std::move(values));
}

auto Statistics::normalizedRate(const std::uint64_t value, const TimeDelta elapsed) noexcept -> double {
    const auto nanoseconds = elapsed.toNanoseconds().toRawValue();
    if (nanoseconds <= 0) {
        return 0.0;
    }
    return static_cast<double>(value) * 1'000'000'000.0 / static_cast<double>(nanoseconds);
}

auto Statistics::fairness(const List<double> &rates) noexcept -> double {
    if (rates.isEmpty()) {
        return 1.0;
    }
    auto minimum = rates.first();
    auto maximum = minimum;
    for (const auto rate : rates) {
        minimum = std::min(minimum, rate);
        maximum = std::max(maximum, rate);
    }
    return calculateFairness(minimum, maximum, true);
}

auto Statistics::workerFairness(const List<WorkerMeasurement> &workers) noexcept -> double {
    auto minimum = std::numeric_limits<double>::max();
    auto maximum = 0.0;
    auto hasRates = false;
    for (const auto &worker : workers) {
        const auto rate = normalizedRate(worker.operations, worker.elapsed);
        minimum = std::min(minimum, rate);
        maximum = std::max(maximum, rate);
        hasRates = true;
    }
    return calculateFairness(minimum, maximum, hasRates);
}

auto Statistics::metricTotal(const std::size_t index) const noexcept -> std::uint64_t {
    return _metricTotals.toRawValue()[index];
}

void Statistics::calculateDistribution(List<double> values) {
    using namespace text::literals;
    if (values.isEmpty()) {
        throw ApplicationError{"Cannot calculate statistics for an empty value list."_el};
    }
    values.sort();
    const auto count = values.count().toSizeT();
    _minimum = values.toRawValue().front();
    _maximum = values.toRawValue().back();
    _median = values.toRawValue()[count / 2U];
    auto total = 0.0;
    for (const auto value : values) {
        total += value;
    }
    _mean = total / static_cast<double>(count);
    const auto percentileIndex =
        std::min(count - 1U, static_cast<std::size_t>(std::ceil(static_cast<double>(count) * 0.95)) - 1U);
    _p95 = values.toRawValue()[percentileIndex];
}

auto Statistics::calculateFairness(const double minimum, const double maximum, const bool hasRates) noexcept -> double {
    return !hasRates || maximum <= 0.0 ? 1.0 : std::max(0.0, minimum / maximum);
}

}
