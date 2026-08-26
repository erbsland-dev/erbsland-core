// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderWorkerWorkload.hpp"

#include "LayoutGraphCompiler.hpp"
#include "ValidationDigest.hpp"

#include <erbsland/profiling/AllocationScope.hpp>
#include <erbsland/profiling/WorkerExecutionContext.hpp>
#include <erbsland/text/render/impl/CompiledLayout.hpp>
#include <erbsland/text/render/impl/Engine.hpp>
#include <erbsland/text/render/impl/Tokenizer.hpp>
#include <erbsland/text/render/impl/TokenKind.hpp>

namespace app::render {

namespace el = erbsland;
namespace pf = erbsland::profiling;
namespace ri = erbsland::text::render::impl;

RenderWorkerWorkload::RenderWorkerWorkload(
    const ProfileStage stage,
    el::List<PreparedLayout> layouts,
    el::List<ri::ConstCompiledLayoutPtr> compiledLayouts,
    el::StringMap<el::text::render::FilterFn> filters,
    const std::uint64_t sourceBytes,
    const std::uint64_t tokens,
    const std::uint64_t bytecodeBytes,
    const std::uint64_t layoutCount,
    const std::uint64_t outputBytes,
    const std::uint64_t inheritanceEdges,
    const std::uint64_t blockPrograms,
    ri::RenderCounters renderCounters,
    const bool trackAllocations) :
    _stage{stage},
    _layouts{std::move(layouts)},
    _compiledLayouts{std::move(compiledLayouts)},
    _filters{std::move(filters)},
    _sourceBytes{sourceBytes},
    _tokens{tokens},
    _bytecodeBytes{bytecodeBytes},
    _layoutCount{layoutCount},
    _outputBytes{outputBytes},
    _inheritanceEdges{inheritanceEdges},
    _blockPrograms{blockPrograms},
    _renderCounters{renderCounters},
    _trackAllocations{trackAllocations} {
}

auto RenderWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    auto sink = std::uint64_t{};
    auto operations = std::uint64_t{};
    auto outputBytes = std::uint64_t{};
    auto renderCounters = ri::RenderCounters{};
    auto allocationScope = pf::AllocationScope{_trackAllocations};
    for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
        switch (_stage) {
        case ProfileStage::Tokenize:
            sink ^= tokenize();
            break;
        case ProfileStage::Compile:
            sink ^= compile();
            break;
        case ProfileStage::Render:
            sink ^= render(outputBytes, renderCounters);
            break;
        }
    }
    const auto allocations = allocationScope.finish();
    return pf::WorkerMeasurement{
        .operations = operations,
        .metrics =
            el::List<std::uint64_t>{
                operations * _layoutCount,
                operations * _sourceBytes,
                operations * _tokens,
                operations * _bytecodeBytes,
                _stage == ProfileStage::Render ? outputBytes : operations * _outputBytes,
                _stage == ProfileStage::Render ? renderCounters.includeExecutions
                                               : operations * _renderCounters.includeExecutions,
                operations * _inheritanceEdges,
                operations * _blockPrograms,
                _stage == ProfileStage::Render ? renderCounters.blockExecutions
                                               : operations * _renderCounters.blockExecutions,
                _stage == ProfileStage::Render ? renderCounters.superExecutions
                                               : operations * _renderCounters.superExecutions,
                _stage == ProfileStage::Render ? renderCounters.capturedSuperExecutions
                                               : operations * _renderCounters.capturedSuperExecutions,
                allocations.allocations,
                allocations.allocatedBytes,
                allocations.deallocations},
        .sink = sink};
}

auto RenderWorkerWorkload::tokenize() const -> std::uint64_t {
    auto sink = std::uint64_t{};
    for (const auto &layout : _layouts) {
        for (const auto &[name, source] : layout.sources) {
            auto tokenizer = ri::Tokenizer{name, source->origin(), source->text(), layout.options};
            do {
                tokenizer.advance();
                sink = ValidationDigest::token(sink, tokenizer.current());
            } while (tokenizer.current().kind != ri::TokenKind::End);
        }
    }
    return sink;
}

auto RenderWorkerWorkload::compile() const -> std::uint64_t {
    auto sink = std::uint64_t{};
    for (const auto &layout : _layouts) {
        const auto graph = LayoutGraphCompiler::validate(LayoutGraphCompiler::compile(layout, _filters));
        sink = (sink * 131U) ^ graph.digest;
    }
    return sink;
}

auto RenderWorkerWorkload::render(std::uint64_t &outputBytes, ri::RenderCounters &counters) const -> std::uint64_t {
    auto sink = std::uint64_t{};
    const auto &layouts = _layouts.toRawValue();
    const auto &compiledLayouts = _compiledLayouts.toRawValue();
    for (auto index = std::size_t{}; index < layouts.size(); ++index) {
        const auto &layout = layouts[index];
        const auto output = ri::Engine{compiledLayouts[index], layout.context, layout.context, counters}.render();
        outputBytes += output.length().toRawValue();
        sink = (sink * 131U) ^ output.length().toRawValue();
    }
    return sink;
}

}
