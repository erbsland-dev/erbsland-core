// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfilingConfiguration.hpp"
#include "ProfilingDefinition.hpp"
#include "SampleMeasurement.hpp"
#include "TimeSource_fwd.hpp"
#include "WorkloadRunner_fwd.hpp"

#include <erbsland/all.hpp>

#include <cstdint>

namespace erbsland::profiling {

/// Execute declaratively registered profiling scenarios.
/// @tested{WorkloadRunnerTest}
class WorkloadRunner final {
public:
    /// Create a common workload runner.
    WorkloadRunner(
        const ProfilingDefinition &definition, ProfilingConfiguration configuration, TimeSourcePtr timeSource = {});

public:
    /// Print stable scenario records without executing work.
    void printDryRun() const;
    /// Print the registered functionality/API coverage.
    void printCoverage() const;
    /// Run all scenarios according to the configured mode.
    [[nodiscard]] auto run() -> ExitCode;

private:
    /// Execute one calibrated sample.
    auto runSample(ScenarioWorkload &workload, const Scenario &scenario, std::uint64_t sample, std::uint64_t operations)
        -> SampleMeasurement;
    /// Determine the operation count for one scenario sample.
    [[nodiscard]] auto calibrate(ScenarioWorkload &workload, const Scenario &scenario) -> std::uint64_t;
    /// Print one measured scenario sample.
    void printSample(const Scenario &scenario, const SampleMeasurement &measurement) const;
    /// Print the aggregate result of scenario samples.
    void printBenchmark(const Scenario &scenario, const List<SampleMeasurement> &measurements) const;

private:
    const ProfilingDefinition &_definition;
    ProfilingConfiguration _configuration;
    TimeSourcePtr _timeSource;
};

}
