// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharSetFixture.hpp"
#include "CharSetScenarioWorkload_fwd.hpp"
#include "Operation.hpp"

#include <erbsland/profiling/ScenarioWorkload.hpp>

namespace app::charset {

/// Prepared shared state for one character-set API scenario.
/// @notest{Covered by the character-set profiling smoke tests.}
class CharSetScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public:
    /// Create a scenario workload for an operation.
    explicit CharSetScenarioWorkload(Operation operation) noexcept : _operation{operation} {}

public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    /// Prepare the fixture shared by the selected scenario.
    void prepareFixture(const el::String &caseName);
    /// Prepare a character-set construction scenario.
    void prepareConstruction(const el::String &caseName);
    /// Prepare a character-set source scenario.
    void prepareSource(const el::String &caseName);
    /// Prepare a character-membership scenario.
    void prepareContains(const el::String &caseName);
    /// Prepare a character-set relation scenario.
    void prepareRelation(const el::String &caseName);
    /// Prepare a case-insensitive relation scenario.
    void prepareCaseRelation(const el::String &caseName);
    /// Prepare a character-set operation scenario.
    void prepareSetOperation(const el::String &caseName);
    /// Prepare a character mapping scenario.
    void prepareMapping(const el::String &caseName);
    /// Prepare a character-range factory scenario.
    void prepareRangeFactory(const el::String &caseName);
    /// Prepare an ASCII-category scenario.
    void prepareAsciiCategory(const el::String &caseName);
    /// Prepare a Unicode-category scenario.
    void prepareUnicodeCategory(const el::String &caseName);
    /// Prepare a Unicode-group scenario.
    void prepareUnicodeGroup(const el::String &caseName);
    /// Prepare a character-set pattern scenario.
    void preparePattern(const el::String &caseName);
    /// Set the fixture input from UTF-8 text.
    void setCharacterInputs(const el::U8String &text);
    /// Update the prepared input character count.
    void updateCharacterCount();

private:
    Operation _operation;    ///< The operation dispatched by each worker.
    CharSetFixture _fixture; ///< The prepared immutable fixture.
};

}
