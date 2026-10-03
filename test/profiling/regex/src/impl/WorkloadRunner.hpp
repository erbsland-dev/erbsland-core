// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Execution.hpp"
#include "WorkloadRunner_fwd.hpp"

#include "../ProfileTypes.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TempDirectory.hpp>

#include <chrono>
#include <optional>
#include <utility>
#include <vector>

namespace app::regex::impl {

using namespace el::text::literals;

/// Implements regex profiler reporting and run control.
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

public:
    /// Own the effective configuration for one profiling workload.
    /// @param configuration The expanded configuration, after command-line overrides.
    explicit WorkloadRunner(Configuration configuration) : _configuration{std::move(configuration)} {}

    // defaults/deletions
    WorkloadRunner(const WorkloadRunner &) = delete;
    WorkloadRunner(WorkloadRunner &&) = delete;
    auto operator=(const WorkloadRunner &) -> WorkloadRunner & = delete;
    auto operator=(WorkloadRunner &&) -> WorkloadRunner & = delete;

public: // reporting and execution
    /// Print the configuration without running its scenarios.
    void printDryRun() const;
    /// Print the API coverage registry.
    static void printCoverage();
    /// Run the configured profiling scenarios.
    [[nodiscard]] auto run() -> el::ExitCode;

private:
    /// Raise a workload error with the given message.
    [[noreturn]] static void workloadError(const el::String &message);
    /// Calculate elapsed nanoseconds since a start time.
    [[nodiscard]] static auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t;
    /// Raise an error when the configured deadline has passed.
    static void requireBeforeDeadline(const Clock::time_point deadline, const el::String &message);
    /// Create the temporary workspace for a profiling run.
    [[nodiscard]] auto createWorkspace() const -> el::TempDirectoryPtr;
    /// Calculate summary statistics for sample values.
    [[nodiscard]] static auto statistics(std::vector<double> values) -> Statistics;
    /// Calculate per-worker fairness for sample results.
    [[nodiscard]] static auto fairness(const std::vector<SampleResult> &samples) -> Fairness;
    /// Calculate a stable digest of a profiling configuration.
    [[nodiscard]] auto configurationDigest() const -> el::ByteBlock;
    /// Print one profile sample.
    static void printSample(const PreparedScenario &prepared, const SampleResult &sample);
    /// Print benchmark statistics and return the median duration.
    [[nodiscard]] static auto printBenchmark(const PreparedScenario &prepared, const std::vector<SampleResult> &samples)
        -> double;
    /// Record an Erbsland-to-standard comparison result.
    static void recordComparison(
        const PreparedScenario &prepared, const double median, std::vector<ComparisonResult> &comparisons);
    /// Print contention diagnostics for a prepared scenario.
    static void printContention(const PreparedScenario &prepared, const std::vector<SampleResult> &samples);
    /// Print progress when its scheduled interval has elapsed.
    static void printProgressIfDue(
        Clock::time_point &nextProgress,
        const std::size_t completed,
        const std::size_t total,
        const std::chrono::nanoseconds interval);

private:
    Configuration _configuration;       ///< Effective configuration owned for the complete workload.
    el::TempDirectoryPtr _workspace;    ///< File fixtures retained while executions use them.
    std::vector<Execution> _executions; ///< Prepared and validated scenario executions.
};

}
