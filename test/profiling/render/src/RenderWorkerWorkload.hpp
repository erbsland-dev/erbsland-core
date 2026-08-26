// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProfileTypes.hpp"
#include "RenderWorkerWorkload_fwd.hpp"

#include <erbsland/profiling/WorkerWorkload.hpp>
#include <erbsland/text/render/impl/CompiledLayout_fwd.hpp>
#include <erbsland/text/render/impl/RenderCounters.hpp>
#include <erbsland/text/render/Value_fwd.hpp>

namespace app::render {

/// Independent tokenizer or compiler workload state for one worker.
/// @notest{Covered by the render profiler smoke and determinism tests.}
class RenderWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    /// Create a worker over an immutable prepared corpus.
    RenderWorkerWorkload(
        ProfileStage stage,
        erbsland::List<PreparedLayout> layouts,
        erbsland::List<erbsland::text::render::impl::ConstCompiledLayoutPtr> compiledLayouts,
        erbsland::StringMap<erbsland::text::render::FilterFn> filters,
        std::uint64_t sourceBytes,
        std::uint64_t tokens,
        std::uint64_t bytecodeBytes,
        std::uint64_t layoutCount,
        std::uint64_t outputBytes,
        std::uint64_t inheritanceEdges,
        std::uint64_t blockPrograms,
        erbsland::text::render::impl::RenderCounters renderCounters,
        bool trackAllocations);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    /// Tokenize one complete corpus pass and return its validation sink.
    [[nodiscard]] auto tokenize() const -> std::uint64_t;
    /// Compile one complete corpus pass and return its validation sink.
    [[nodiscard]] auto compile() const -> std::uint64_t;
    /// Render one precompiled corpus pass and update measured work counters.
    [[nodiscard]] auto render(std::uint64_t &outputBytes, erbsland::text::render::impl::RenderCounters &counters) const
        -> std::uint64_t;

private:
    ProfileStage _stage;                     ///< Measured pipeline stage.
    erbsland::List<PreparedLayout> _layouts; ///< Immutable worker-local corpus handles.
    erbsland::List<erbsland::text::render::impl::ConstCompiledLayoutPtr> _compiledLayouts; ///< Graph roots.
    erbsland::StringMap<erbsland::text::render::FilterFn> _filters; ///< Immutable identity-filter registry.
    std::uint64_t _sourceBytes{};                                   ///< Source bytes per operation.
    std::uint64_t _tokens{};                                        ///< Tokens per operation.
    std::uint64_t _bytecodeBytes{};                                 ///< Bytecode bytes per operation.
    std::uint64_t _layoutCount{};                                   ///< Unique graph layouts per operation.
    std::uint64_t _outputBytes{};                                   ///< Expected rendered bytes per operation.
    std::uint64_t _inheritanceEdges{};                              ///< Static parent edges per operation.
    std::uint64_t _blockPrograms{};                                 ///< Compiled block programs per operation.
    erbsland::text::render::impl::RenderCounters _renderCounters;   ///< Expected render dispatches per operation.
    bool _trackAllocations{};                                       ///< Whether allocation hooks are active.
};

}
