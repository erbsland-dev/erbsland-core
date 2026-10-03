// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkloadRunner.hpp"

#include <erbsland/StandardStreams.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace app::regex::impl {

auto WorkloadRunner::statistics(std::vector<double> values) -> Statistics {
    std::ranges::sort(values);
    const auto mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    return Statistics{
        .minimum = values.front(),
        .median = values[values.size() / 2U],
        .mean = mean,
        .p95 = values[std::min(values.size() - 1U, (values.size() * 95U) / 100U)],
        .maximum = values.back()};
}

auto WorkloadRunner::fairness(const std::vector<SampleResult> &samples) -> Fairness {
    auto rates = std::vector<double>{};
    for (const auto &sample : samples) {
        for (const auto &worker : sample.workers) {
            rates.emplace_back(
                static_cast<double>(worker.operations) * 1.0e9 /
                static_cast<double>(std::max<std::int64_t>(1, worker.nanoseconds)));
        }
    }
    const auto mean = std::accumulate(rates.begin(), rates.end(), 0.0) / static_cast<double>(rates.size());
    auto variance = 0.0;
    for (const auto value : rates) {
        const auto delta = value - mean;
        variance += delta * delta;
    }
    variance /= static_cast<double>(rates.size());
    return Fairness{
        .minimum = *std::ranges::min_element(rates),
        .maximum = *std::ranges::max_element(rates),
        .coefficientOfVariation = mean == 0.0 ? 0.0 : std::sqrt(variance) / mean};
}

void WorkloadRunner::printSample(const PreparedScenario &prepared, const SampleResult &sample) {
    const auto safeOperations = std::max<std::uint64_t>(1U, sample.operations);
    const auto safeTime = std::max<std::int64_t>(1, sample.wallNanoseconds);
    el::io::printLine(
        "record=sample scenario="_el,
        prepared.scenario.id,
        " use-case="_el,
        toString(prepared.scenario.useCase),
        " input="_el,
        toString(prepared.scenario.inputKind),
        " operations="_el,
        sample.operations,
        " matches="_el,
        sample.matches,
        " logical-bytes="_el,
        sample.logicalBytes,
        " logical-code-points="_el,
        sample.logicalCodePoints,
        " wall-ns="_el,
        sample.wallNanoseconds,
        " ns-per-operation="_el,
        static_cast<double>(safeTime) / static_cast<double>(safeOperations),
        " operations-per-second="_el,
        static_cast<double>(safeOperations) * 1.0e9 / static_cast<double>(safeTime));
}

auto WorkloadRunner::printBenchmark(const PreparedScenario &prepared, const std::vector<SampleResult> &samples)
    -> double {
    auto nanoseconds = std::vector<double>{};
    auto operationRates = std::vector<double>{};
    auto matchRates = std::vector<double>{};
    auto throughputs = std::vector<double>{};
    auto totalOperations = std::uint64_t{};
    auto totalMatches = std::uint64_t{};
    auto totalBytes = std::uint64_t{};
    auto totalCodePoints = std::uint64_t{};
    for (const auto &sample : samples) {
        const auto operations = std::max<std::uint64_t>(1U, sample.operations);
        const auto elapsed = std::max<std::int64_t>(1, sample.wallNanoseconds);
        nanoseconds.emplace_back(static_cast<double>(elapsed) / static_cast<double>(operations));
        operationRates.emplace_back(static_cast<double>(operations) * 1.0e9 / static_cast<double>(elapsed));
        matchRates.emplace_back(static_cast<double>(sample.matches) * 1.0e9 / static_cast<double>(elapsed));
        throughputs.emplace_back(
            static_cast<double>(sample.logicalBytes) * 1.0e9 / static_cast<double>(elapsed) / (1024.0 * 1024.0));
        totalOperations += sample.operations;
        totalMatches += sample.matches;
        totalBytes += sample.logicalBytes;
        totalCodePoints += sample.logicalCodePoints;
    }
    const auto timing = statistics(std::move(nanoseconds));
    const auto operations = statistics(std::move(operationRates));
    const auto matches = statistics(std::move(matchRates));
    const auto throughput = statistics(std::move(throughputs));
    const auto workerFairness = fairness(samples);
    el::io::printLine(
        "record=benchmark scenario="_el,
        prepared.scenario.id,
        " group="_el,
        prepared.scenario.group,
        " use-case="_el,
        toString(prepared.scenario.useCase),
        " input="_el,
        toString(prepared.scenario.inputKind),
        " backend="_el,
        toString(prepared.scenario.backend),
        " replacement="_el,
        toString(prepared.scenario.replacementMode),
        " pattern="_el,
        prepared.scenario.patternName,
        " pattern-code-points="_el,
        prepared.scenario.pattern.characterLength(),
        " pattern-bytes="_el,
        prepared.scenario.pattern.length(),
        " subject-bytes="_el,
        prepared.subject8.length(),
        " subject-code-points="_el,
        prepared.subject8.characterLength(),
        " samples="_el,
        samples.size(),
        " operations="_el,
        totalOperations,
        " matches="_el,
        totalMatches,
        " logical-bytes="_el,
        totalBytes,
        " logical-code-points="_el,
        totalCodePoints,
        " signature="_el,
        prepared.validationSink,
        " validation-matches="_el,
        prepared.validationMatches,
        " min-ns-per-operation="_el,
        timing.minimum,
        " median-ns-per-operation="_el,
        timing.median,
        " mean-ns-per-operation="_el,
        timing.mean,
        " p95-ns-per-operation="_el,
        timing.p95,
        " max-ns-per-operation="_el,
        timing.maximum,
        " median-operations-per-second="_el,
        operations.median,
        " median-matches-per-second="_el,
        matches.median,
        " median-mib-per-second="_el,
        throughput.median,
        " fairness-min-operations-per-second="_el,
        workerFairness.minimum,
        " fairness-max-operations-per-second="_el,
        workerFairness.maximum,
        " fairness-cv="_el,
        workerFairness.coefficientOfVariation);
    return timing.median;
}

void WorkloadRunner::recordComparison(
    const PreparedScenario &prepared, const double median, std::vector<ComparisonResult> &comparisons) {
    if (prepared.scenario.comparisonName.isEmpty()) {
        return;
    }
    auto found = std::ranges::find(comparisons, prepared.scenario.comparisonName, &ComparisonResult::name);
    if (found == comparisons.end()) {
        found = comparisons.emplace(
            comparisons.end(),
            ComparisonResult{
                .name = prepared.scenario.comparisonName,
                .erbslandMedian = {},
                .standardMedian = {},
                .erbslandSignature = {},
                .standardSignature = {},
                .erbslandMatches = {},
                .standardMatches = {}});
    }
    if (prepared.scenario.backend == Backend::Erbsland) {
        found->erbslandMedian = median;
        found->erbslandSignature = prepared.validationSink;
        found->erbslandMatches = prepared.validationMatches;
    } else {
        found->standardMedian = median;
        found->standardSignature = prepared.validationSink;
        found->standardMatches = prepared.validationMatches;
    }
    if (!found->erbslandMedian || !found->standardMedian) {
        return;
    }
    const auto parity =
        found->erbslandSignature == found->standardSignature && found->erbslandMatches == found->standardMatches;
    const auto ratio = *found->erbslandMedian / *found->standardMedian;
    const auto fasterOrEqual = ratio <= 1.0;
    el::io::printLine(
        "record=comparison name="_el,
        found->name,
        " erbsland-median-ns="_el,
        *found->erbslandMedian,
        " std-median-ns="_el,
        *found->standardMedian,
        " ratio="_el,
        ratio,
        " parity="_el,
        parity ? "yes"_el : "no"_el,
        " faster-or-equal="_el,
        fasterOrEqual ? "yes"_el : "no"_el,
        " pass="_el,
        parity && fasterOrEqual ? "yes"_el : "no"_el);
}

void WorkloadRunner::printContention(const PreparedScenario &prepared, const std::vector<SampleResult> &samples) {
    auto wallTimes = std::vector<double>{};
    auto waiterLatencies = std::vector<double>{};
    for (const auto &sample : samples) {
        wallTimes.emplace_back(static_cast<double>(sample.wallNanoseconds));
        for (const auto &worker : sample.workers) {
            waiterLatencies.emplace_back(static_cast<double>(worker.nanoseconds));
        }
    }
    const auto wall = statistics(std::move(wallTimes));
    const auto waiters = statistics(std::move(waiterLatencies));
    el::io::printLine(
        "record=sample kind=contention scenario="_el,
        prepared.scenario.id,
        " initialization-wall-median-ns="_el,
        wall.median,
        " waiter-latency-min-ns="_el,
        waiters.minimum,
        " waiter-latency-median-ns="_el,
        waiters.median,
        " waiter-latency-p95-ns="_el,
        waiters.p95,
        " waiter-latency-max-ns="_el,
        waiters.maximum);
}

}
