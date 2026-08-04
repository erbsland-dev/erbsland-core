// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathWalkerScenarioWorkload_fwd.hpp"
#include "PathWalkMethod.hpp"

#include <erbsland/profiling/ScenarioWorkload.hpp>

namespace app::path {

/// Shared state and validation for one PathWalker profiling scenario.
/// @notest{Covered by manual profiling runs and the PathWalker unit tests.}
class PathWalkerScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public:
    /// Create a scenario workload for `method`.
    explicit PathWalkerScenarioWorkload(PathWalkMethod method) noexcept;

public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    PathWalkMethod _method;
    erbsland::Path _root;
    std::uint64_t _expectedEntryCount{};
};

}
