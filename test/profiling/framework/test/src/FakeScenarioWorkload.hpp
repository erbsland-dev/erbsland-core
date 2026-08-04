// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FakeScenarioWorkload_fwd.hpp"

#include <erbsland/profiling/ScenarioWorkload.hpp>

#include <atomic>
#include <memory>

/// A deterministic profiling scenario used by framework tests.
/// @tested{WorkloadRunnerTest}
class FakeScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public:
    /// Create a deterministic scenario workload with shared counters.
    FakeScenarioWorkload(
        std::shared_ptr<std::atomic<std::uint64_t>> executions,
        std::shared_ptr<std::atomic<std::uint64_t>> validations,
        bool fail);

public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    std::shared_ptr<std::atomic<std::uint64_t>> _executions;
    std::shared_ptr<std::atomic<std::uint64_t>> _validations;
    bool _fail{};
};
