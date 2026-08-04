// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Definitions.hpp"
#include "RunConfiguration.hpp"
#include "SampleMeasurement.hpp"
#include "Scenario.hpp"
#include "ScenarioWorkload_fwd.hpp"
#include "WorkerWorkload_fwd.hpp"

#include <cstdint>

namespace erbsland::profiling {

/// Prepared shared state and validation for one expanded scenario.
/// @tested{WorkloadRunnerTest}
class ScenarioWorkload {
public:
    // defaults
    virtual ~ScenarioWorkload() = default;

public:
    /// Prepare shared fixtures outside measured work.
    virtual void prepare(const RunConfiguration &run, const Scenario &scenario) = 0;
    /// Return the maximum safe operations in one worker batch.
    [[nodiscard]] virtual auto maximumOperations() const noexcept -> std::uint64_t = 0;
    /// Create independent mutable state for one worker.
    /// @param worker Worker index.
    /// @return Worker workload.
    [[nodiscard]] virtual auto createWorker(std::uint32_t worker) -> WorkerWorkloadPtr = 0;
    /// Validate a completed synchronized sample outside measured work.
    /// @param measurement Sample to validate.
    virtual void validate(const SampleMeasurement &measurement) = 0;
};

}
