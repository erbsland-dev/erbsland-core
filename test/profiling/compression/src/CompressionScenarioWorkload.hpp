// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionScenarioWorkload_fwd.hpp"
#include "ProfileTypes.hpp"

#include <erbsland/compression/CompressionAlgorithm.hpp>
#include <erbsland/compression/CompressionFormat.hpp>
#include <erbsland/compression/CompressionLevel.hpp>
#include <erbsland/compression/DecompressionOptions.hpp>
#include <erbsland/profiling/ScenarioWorkload.hpp>

#include <memory>

namespace app::compression {

/// Prepared shared state for one compression profiling scenario.
/// @notest{Covered by the compression profiling smoke and coverage tests.}
class CompressionScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public:
    /// Create a workload for a one-shot operation.
    explicit CompressionScenarioWorkload(ProfileOperation operation) noexcept : _operation{operation} {}

public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    /// Find a required axis value.
    [[nodiscard]] static auto axisValue(const erbsland::profiling::Scenario &scenario, const erbsland::String &axis)
        -> erbsland::String;
    /// Parse a compression-format axis value.
    [[nodiscard]] static auto parseFormat(const erbsland::String &value) -> erbsland::compression::CompressionFormat;
    /// Parse a compression-level axis value.
    [[nodiscard]] static auto parseLevel(const erbsland::String &value) -> erbsland::compression::CompressionLevel;
    /// Validate the projected retained fixture memory.
    void validateMemory(const erbsland::profiling::RunConfiguration &run) const;

private:
    ProfileOperation _operation;                            ///< Operation measured by this scenario.
    erbsland::compression::CompressionAlgorithm _algorithm; ///< Selected algorithm.
    erbsland::compression::CompressionFormat _format{};     ///< Selected representation.
    erbsland::compression::CompressionLevel _level{};       ///< Selected encoder effort.
    erbsland::compression::DecompressionOptions _options;   ///< Exact decoder validation and limits.
    std::shared_ptr<const erbsland::ByteBlock> _source;     ///< Shared uncompressed fixture.
    std::shared_ptr<const erbsland::ByteBlock> _compressed; ///< Shared encoded fixture.
    std::shared_ptr<const erbsland::ByteBlock> _expected;   ///< Exact expected worker result.
};

}
