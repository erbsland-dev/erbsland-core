// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"
#include "RenderScenarioWorkload_fwd.hpp"

#include <erbsland/profiling/ScenarioWorkload.hpp>
#include <erbsland/text/render/impl/CompiledLayout_fwd.hpp>
#include <erbsland/text/render/impl/RenderCounters.hpp>
#include <erbsland/text/render/Value_fwd.hpp>

namespace app::render {

/// Shared prepared state for one renderer compilation scenario.
/// @notest{Covered by the render profiler smoke and determinism tests.}
class RenderScenarioWorkload final : public erbsland::profiling::ScenarioWorkload {
public:
    /// Create a workload for one compilation stage.
    explicit RenderScenarioWorkload(ProfileStage stage) noexcept : _stage{stage} {}

public: // implement ScenarioWorkload
    void prepare(
        const erbsland::profiling::RunConfiguration &run, const erbsland::profiling::Scenario &scenario) override;
    [[nodiscard]] auto maximumOperations() const noexcept -> std::uint64_t override;
    [[nodiscard]] auto createWorker(std::uint32_t worker) -> erbsland::profiling::WorkerWorkloadPtr override;
    void validate(const erbsland::profiling::SampleMeasurement &measurement) override;

private:
    /// Find the selected corpus axis value.
    [[nodiscard]] static auto corpusSelection(const erbsland::profiling::Scenario &scenario) -> erbsland::String;
    /// Populate deterministic validation totals outside measured work.
    void prepareExpectedTotals();

private:
    ProfileStage _stage;                     ///< Measured pipeline stage.
    erbsland::List<PreparedLayout> _layouts; ///< Loaded immutable layout corpus.
    erbsland::List<erbsland::text::render::impl::ConstCompiledLayoutPtr> _compiledLayouts; ///< Graph roots.
    erbsland::StringMap<erbsland::text::render::FilterFn> _filters; ///< Identity filters used by corpus sources.
    std::uint64_t _sourceBytes{};                                   ///< Source bytes per corpus pass.
    std::uint64_t _tokens{};                                        ///< Tokens per corpus pass.
    std::uint64_t _bytecodeBytes{};                                 ///< Compiled bytecode bytes per corpus pass.
    std::uint64_t _layoutCount{};                                   ///< Unique graph layouts per corpus pass.
    std::uint64_t _outputBytes{};                                   ///< Rendered output bytes per corpus pass.
    std::uint64_t _inheritanceEdges{};                              ///< Static parent edges per corpus pass.
    std::uint64_t _blockPrograms{};                                 ///< Compiled block programs per corpus pass.
    erbsland::text::render::impl::RenderCounters _renderCounters;   ///< Render dispatches per corpus pass.
    bool _trackAllocations{};                                       ///< Whether worker allocation hooks are active.
};

}
