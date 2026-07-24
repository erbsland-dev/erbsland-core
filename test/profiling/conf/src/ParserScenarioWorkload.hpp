// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserScenarioWorkload_fwd.hpp"

#include <erbsland/profiling/ScenarioWorkload.hpp>

namespace app::conf {

/// Shared embedded-corpus parser workload.
/// @tested{WorkloadRunnerTest}
class ParserScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    erbsland::StringList _documents;
    std::uint64_t _corpusBytes{};
    bool _failWorker{};
};

}
