// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HashScenarioWorkload_fwd.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/profiling/ScenarioWorkload.hpp>

#include <memory>

namespace app::cryptology {

/// Prepared shared state for one hash algorithm and input size.
/// @notest{Covered by the cryptology profiling smoke tests.}
class HashScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    erbsland::cryptology::HashAlgorithm _algorithm;
    std::shared_ptr<const erbsland::ByteBuffer> _input;
    erbsland::ByteBlock _expectedDigest;
    bool _failWorker{};
};

}
