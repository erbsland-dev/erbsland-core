// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BuildIdentity.hpp"
#include "Execution.hpp"
#include "WorkloadRunner_fwd.hpp"

#include "../ProfileTypes.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace app::regex::impl {

using namespace el::text::literals;

/// Implements regex profiler reporting and run control.
/// @notest{Covered by regex profiler CTest entries.}
class WorkloadRunner final {
    using Clock = std::chrono::steady_clock;

    /// Collect timing statistics for a scenario.
    struct Statistics {
        double minimum{};
        double median{};
        double mean{};
        double p95{};
        double maximum{};
    };

    /// Collect per-worker timing fairness metrics.
    struct Fairness {
        double minimum{};
        double maximum{};
        double coefficientOfVariation{};
    };

public:
    /// Print the configuration without running its scenarios.
    static void printDryRun(const Configuration &configuration) {
        for (const auto &scenario : configuration.scenarios) {
            el::io::printLine(
                "record=scenario id="_el,
                scenario.id,
                " group="_el,
                scenario.group,
                " use-case="_el,
                toString(scenario.useCase),
                " input="_el,
                toString(scenario.inputKind),
                " backend="_el,
                toString(scenario.backend),
                " replacement="_el,
                toString(scenario.replacementMode),
                " pattern="_el,
                scenario.patternName,
                " pattern-code-points="_el,
                scenario.pattern.characterLength(),
                " corpus="_el,
                scenario.corpusName,
                " corpus-source="_el,
                scenario.corpusSource == CorpusSource::File ? "file"_el : "inline"_el,
                " repeat="_el,
                scenario.repetitionCount,
                " timeout-ms="_el,
                scenario.timeout.count(),
                " weight="_el,
                scenario.weight);
        }
        el::io::printLine(
            "record=summary action=dry-run scenarios="_el,
            configuration.scenarios.size(),
            " threads="_el,
            configuration.run.threadCount,
            " suite="_el,
            configuration.run.suite,
            " duration-ns="_el,
            configuration.run.duration.count(),
            " seed="_el,
            configuration.run.seed,
            " warmup-samples="_el,
            configuration.run.warmupSamples,
            " samples="_el,
            configuration.run.samples,
            " minimum-sample-time-ns="_el,
            configuration.run.minimumSampleTime.count(),
            " memory-limit="_el,
            configuration.run.memoryLimit,
            " progress-interval-ns="_el,
            configuration.run.progressInterval.count(),
            " config-md5="_el,
            el::ByteFormat::compact(),
            configurationDigest(configuration));
    }

    /// Print the API coverage registry.
    static void printCoverage() {
        for (const auto &descriptor : coverageRegistry()) {
            el::io::printLine(
                "record=coverage use-case="_el,
                toString(descriptor.useCase),
                " input="_el,
                toString(descriptor.inputKind),
                " replacement="_el,
                toString(descriptor.replacementMode),
                " api="_el,
                descriptor.apiFamily);
        }
        el::io::printLine("record=summary action=coverage paths="_el, coverageRegistry().size());
    }

    /// Run the configured profiling scenarios.
    [[nodiscard]] static auto run(const Configuration &configuration) -> el::ExitCode {
        auto workspace = createWorkspace(configuration);
        el::io::printLine(
            "record=run profile=regex-api mode="_el,
            toString(configuration.run.mode),
            " executable=regex-api-profile build-type="_el,
            el::String{cBuildType},
            " compiler="_el,
            el::String{cCompilerId},
            " compiler-version="_el,
            el::String{cCompilerVersion},
            " system="_el,
            el::String{cSystemName},
            " processor="_el,
            el::String{cSystemProcessor},
            " suite="_el,
            configuration.run.suite,
            " workload-threads="_el,
            configuration.run.threadCount,
            " seed="_el,
            configuration.run.seed,
            " scenarios="_el,
            configuration.scenarios.size(),
            " duration-ns="_el,
            configuration.run.duration.count(),
            " workspace="_el,
            workspace->path().toString(),
            " config-md5="_el,
            el::ByteFormat::compact(),
            configurationDigest(configuration));

        auto prepared = std::vector<PreparedScenario>{};
        prepared.reserve(configuration.scenarios.size());
        for (const auto &scenario : configuration.scenarios) {
            prepared.emplace_back(Execution::prepare(configuration, scenario, workspace));
            Execution::validate(prepared.back());
        }
        auto operations = std::vector<std::uint64_t>{};
        operations.reserve(prepared.size());
        for (const auto &scenario : prepared) {
            operations.emplace_back(Execution::calibrate(configuration, scenario));
        }

        const auto runStart = Clock::now();
        const auto deadline = runStart + configuration.run.duration;
        auto nextProgress = runStart + configuration.run.progressInterval;
        for (auto index = std::size_t{}; index < prepared.size(); ++index) {
            const auto &scenario = prepared[index];
            for (auto warmup = std::uint32_t{}; warmup < configuration.run.warmupSamples; ++warmup) {
                requireBeforeDeadline(deadline, "The regex profiler deadline was reached during warm-up."_el);
                Execution::runSample(configuration, scenario, warmup, operations[index]);
            }
        }

        auto completed = std::size_t{};
        auto comparisons = std::vector<ComparisonResult>{};
        if (configuration.run.mode == RunMode::Benchmark) {
            for (auto index = std::size_t{}; index < prepared.size(); ++index) {
                auto samples = std::vector<SampleResult>{};
                samples.reserve(configuration.run.samples);
                for (auto sample = std::uint32_t{}; sample < configuration.run.samples; ++sample) {
                    requireBeforeDeadline(
                        deadline,
                        "The benchmark deadline was reached before every scenario collected its required samples."_el);
                    samples.emplace_back(
                        Execution::runSample(configuration, prepared[index], sample + 100U, operations[index]));
                }
                const auto median = printBenchmark(prepared[index], samples);
                recordComparison(prepared[index], median, comparisons);
                if (prepared[index].scenario.useCase == UseCase::LazyContendedFirstUse) {
                    printContention(prepared[index], samples);
                }
                ++completed;
                printProgressIfDue(nextProgress, completed, prepared.size(), configuration.run.progressInterval);
            }
        } else {
            auto sample = std::uint64_t{100U};
            while (Clock::now() < deadline) {
                for (auto index = std::size_t{}; index < prepared.size() && Clock::now() < deadline; ++index) {
                    for (
                        auto repeat = std::uint32_t{};
                        repeat < prepared[index].scenario.weight && Clock::now() < deadline;
                        ++repeat) {
                        const auto result =
                            Execution::runSample(configuration, prepared[index], sample++, operations[index]);
                        if (Clock::now() >= nextProgress) {
                            printSample(prepared[index], result);
                            nextProgress = Clock::now() + configuration.run.progressInterval;
                        }
                    }
                    if (completed < prepared.size()) {
                        ++completed;
                    }
                }
            }
        }
        el::io::printLine(
            "record=summary covered-scenarios="_el,
            completed,
            " total-scenarios="_el,
            prepared.size(),
            " elapsed-ns="_el,
            elapsedNanoseconds(runStart),
            " workspace="_el,
            workspace->path().toString(),
            " files-kept="_el,
            configuration.run.keepFiles ? "yes"_el : "no"_el);
        return el::ExitCode::success();
    }

private:
    /// Collect results that can be compared between profiler backends.
    struct ComparisonResult {
        el::String name;
        std::optional<double> erbslandMedian;
        std::optional<double> standardMedian;
        std::optional<std::uint64_t> erbslandSignature;
        std::optional<std::uint64_t> standardSignature;
        std::optional<std::uint64_t> erbslandMatches;
        std::optional<std::uint64_t> standardMatches;
    };

    /// Raise a workload error with the given message.
    [[noreturn]] static void workloadError(const el::String &message) { throw el::ApplicationError{message}; }

    /// Calculate elapsed nanoseconds since a start time.
    [[nodiscard]] static auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
    }

    /// Raise an error when the configured deadline has passed.
    static void requireBeforeDeadline(const Clock::time_point deadline, const el::String &message) {
        if (Clock::now() >= deadline) {
            workloadError(message);
        }
    }

    /// Create the temporary workspace for a profiling run.
    [[nodiscard]] static auto createWorkspace(const Configuration &configuration) -> el::TempDirectoryPtr {
        auto options = el::PathTempDirectoryOptions{};
        options.setPrefix("regex-api-profile-"_el).setRandomLength(el::CpLength{12U});
        auto root = el::Path::systemTempDirectoryOrThrow();
        if (!configuration.run.workspace.isEmpty()) {
            root = el::Path{configuration.run.workspace};
            if (!root.isValid()) {
                workloadError("The configured workspace is not a valid path."_el);
            }
            auto directoryOptions = el::PathCreateDirectoryOptions{};
            directoryOptions.setCreateParents(true).setCreationMode(el::PathCreateMode::CreateOrOverwrite);
            root.operations().createDirectoryOrThrow(directoryOptions);
        }
        auto workspace = root.operations().createTempDirectoryOrThrow(options);
        if (configuration.run.keepFiles) {
            workspace->setRemoveOnDestroy(false);
        }
        return workspace;
    }

    /// Calculate summary statistics for sample values.
    [[nodiscard]] static auto statistics(std::vector<double> values) -> Statistics {
        std::ranges::sort(values);
        const auto mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
        return Statistics{
            .minimum = values.front(),
            .median = values[values.size() / 2U],
            .mean = mean,
            .p95 = values[std::min(values.size() - 1U, (values.size() * 95U) / 100U)],
            .maximum = values.back()};
    }

    /// Calculate per-worker fairness for sample results.
    [[nodiscard]] static auto fairness(const std::vector<SampleResult> &samples) -> Fairness {
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

    /// Calculate a stable digest of a profiling configuration.
    [[nodiscard]] static auto configurationDigest(const Configuration &configuration) -> el::ByteBlock {
        auto text = el::StringEditor{};
        text.append(
            el::StringFormat{"{} {} {} {} {} {} {} {} {} {} {}\n"_el}.build(
                toString(configuration.run.mode),
                configuration.run.suite,
                configuration.run.duration.count(),
                configuration.run.threadCount,
                configuration.run.seed,
                configuration.run.warmupSamples,
                configuration.run.samples,
                configuration.run.minimumSampleTime.count(),
                configuration.run.memoryLimit,
                configuration.run.regexTimeout.count(),
                configuration.run.keepFiles));
        for (const auto &scenario : configuration.scenarios) {
            text.append(
                el::StringFormat{"{} {} {} {} {} {} {} {} {}\n"_el}.build(
                    scenario.id,
                    toString(scenario.useCase),
                    toString(scenario.inputKind),
                    toString(scenario.replacementMode),
                    scenario.pattern,
                    scenario.corpusName,
                    scenario.subject.length(),
                    scenario.sourceFile,
                    scenario.repetitionCount));
            text.append(el::StringFormat{" {} {}\n"_el}.build(toString(scenario.backend), scenario.comparisonName));
        }
        auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
        hasher.update(el::String{text});
        return hasher.finalize();
    }

    /// Print one profile sample.
    static void printSample(const PreparedScenario &prepared, const SampleResult &sample) {
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

    /// Print benchmark statistics and return the median duration.
    [[nodiscard]] static auto printBenchmark(const PreparedScenario &prepared, const std::vector<SampleResult> &samples)
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

    /// Record an Erbsland-to-standard comparison result.
    static void recordComparison(
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

    /// Print contention diagnostics for a prepared scenario.
    static void printContention(const PreparedScenario &prepared, const std::vector<SampleResult> &samples) {
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

    /// Print progress when its scheduled interval has elapsed.
    static void printProgressIfDue(
        Clock::time_point &nextProgress,
        const std::size_t completed,
        const std::size_t total,
        const std::chrono::nanoseconds interval) {
        if (Clock::now() >= nextProgress) {
            el::io::printLine("record=progress completed="_el, completed, " total="_el, total);
            nextProgress = Clock::now() + interval;
        }
    }
};

}
