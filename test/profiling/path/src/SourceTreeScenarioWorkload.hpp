// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceReadMethod.hpp"
#include "SourceTreeScenarioWorkload_fwd.hpp"

#include <erbsland/Path.hpp>
#include <erbsland/profiling/ScenarioWorkload.hpp>

#include <cstdint>

namespace app::path {

/// Untimed inventory and validation for source-tree read workloads.
class SourceTreeScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public:
    /// Create a source-tree scenario.
    /// @param method Whole-file read API to exercise.
    explicit SourceTreeScenarioWorkload(SourceReadMethod method) noexcept;

public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    /// Independently count eligible files and their sizes without reading their contents.
    void inventory();

private:
    SourceReadMethod _method;           ///< API used during measured reads.
    el::Path _root;                     ///< Resolved source-tree directory.
    std::uint64_t _expectedFileCount{}; ///< Files expected in one complete pass.
    std::uint64_t _expectedByteCount{}; ///< File bytes expected in one binary pass.
};

}
