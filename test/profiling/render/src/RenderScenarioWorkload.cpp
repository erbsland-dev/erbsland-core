// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderScenarioWorkload.hpp"

#include "Corpus.hpp"
#include "LayoutGraphCompiler.hpp"
#include "RenderWorkerWorkload.hpp"
#include "ValidationDigest.hpp"

#include <erbsland/text/render/impl/CompiledLayout.hpp>
#include <erbsland/text/render/impl/Engine.hpp>
#include <erbsland/text/render/impl/Tokenizer.hpp>
#include <erbsland/text/render/impl/TokenKind.hpp>
#include <erbsland/text/render/Value.hpp>
#include <erbsland/text/StringFormat.hpp>

#include <variant>

namespace app::render {

namespace el = erbsland;
namespace pf = erbsland::profiling;
namespace ri = erbsland::text::render::impl;

using namespace el::text::literals;

void RenderScenarioWorkload::prepare([[maybe_unused]] const pf::RunConfiguration &run, const pf::Scenario &scenario) {
    _trackAllocations = false;
    for (const auto &parameter : scenario.parameters) {
        if (parameter.parameter == "track-allocations"_el) {
            _trackAllocations = std::get<bool>(parameter.value);
        }
    }
    _layouts = corpus::load(corpusSelection(scenario));
    _filters.clear();
    const auto identity = [](const el::text::render::ValueList &values) -> el::text::render::Value {
        return values.getRefOrThrow(el::unit::ItemIndex::zero());
    };
    _filters.set("upper"_el, identity)
        .set("legacy"_el, identity)
        .set("profile0"_el, identity)
        .set("profile1"_el, identity)
        .set("profile2"_el, identity);
    prepareExpectedTotals();
}

auto RenderScenarioWorkload::maximumOperations() const noexcept -> std::uint64_t {
    return 100'000U;
}

auto RenderScenarioWorkload::createWorker([[maybe_unused]] const std::uint32_t worker) -> pf::WorkerWorkloadPtr {
    return std::make_shared<RenderWorkerWorkload>(
        _stage,
        _layouts,
        _compiledLayouts,
        _filters,
        _sourceBytes,
        _tokens,
        _stage == ProfileStage::Tokenize ? 0U : _bytecodeBytes,
        _layoutCount,
        _stage == ProfileStage::Render ? _outputBytes : 0U,
        _stage == ProfileStage::Tokenize ? 0U : _inheritanceEdges,
        _stage == ProfileStage::Tokenize ? 0U : _blockPrograms,
        _stage == ProfileStage::Render ? _renderCounters : ri::RenderCounters{},
        _trackAllocations);
}

void RenderScenarioWorkload::validate(const pf::SampleMeasurement &measurement) {
    if (measurement.operations == 0U || measurement.metrics.count() != el::ItemCount{14U}) {
        throw el::ApplicationError{"The render profiling workload produced an invalid measurement."_el};
    }
    const auto operations = measurement.operations;
    if (measurement.metrics.get(el::unit::ItemIndex{0U}, 0U) != operations * _layoutCount ||
        measurement.metrics.get(el::unit::ItemIndex{1U}, 0U) != operations * _sourceBytes ||
        measurement.metrics.get(el::unit::ItemIndex{2U}, 0U) != operations * _tokens ||
        measurement.metrics.get(el::unit::ItemIndex{3U}, 0U) !=
            operations * (_stage == ProfileStage::Tokenize ? 0U : _bytecodeBytes) ||
        measurement.metrics.get(el::unit::ItemIndex{4U}, 0U) !=
            operations * (_stage == ProfileStage::Render ? _outputBytes : 0U) ||
        measurement.metrics.get(el::unit::ItemIndex{5U}, 0U) !=
            operations * (_stage == ProfileStage::Render ? _renderCounters.includeExecutions : 0U) ||
        measurement.metrics.get(el::unit::ItemIndex{6U}, 0U) !=
            operations * (_stage == ProfileStage::Tokenize ? 0U : _inheritanceEdges) ||
        measurement.metrics.get(el::unit::ItemIndex{7U}, 0U) !=
            operations * (_stage == ProfileStage::Tokenize ? 0U : _blockPrograms) ||
        measurement.metrics.get(el::unit::ItemIndex{8U}, 0U) !=
            operations * (_stage == ProfileStage::Render ? _renderCounters.blockExecutions : 0U) ||
        measurement.metrics.get(el::unit::ItemIndex{9U}, 0U) !=
            operations * (_stage == ProfileStage::Render ? _renderCounters.superExecutions : 0U) ||
        measurement.metrics.get(el::unit::ItemIndex{10U}, 0U) !=
            operations * (_stage == ProfileStage::Render ? _renderCounters.capturedSuperExecutions : 0U)) {
        throw el::ApplicationError{"The render profiling workload counters failed validation."_el};
    }
    const auto allocations = measurement.metrics.get(el::unit::ItemIndex{11U}, 0U);
    const auto allocatedBytes = measurement.metrics.get(el::unit::ItemIndex{12U}, 0U);
    const auto deallocations = measurement.metrics.get(el::unit::ItemIndex{13U}, 0U);
    if ((_trackAllocations && (allocations == 0U || allocatedBytes == 0U || deallocations == 0U)) ||
        (!_trackAllocations && (allocations != 0U || allocatedBytes != 0U || deallocations != 0U))) {
        throw el::ApplicationError{"The render allocation counters failed validation."_el};
    }
}

auto RenderScenarioWorkload::corpusSelection(const pf::Scenario &scenario) -> el::String {
    for (const auto &axis : scenario.axes) {
        if (axis.axis == "corpus"_el) {
            return axis.value;
        }
    }
    throw el::ApplicationError{"A render profiling scenario has no corpus selection."_el};
}

void RenderScenarioWorkload::prepareExpectedTotals() {
    _sourceBytes = 0U;
    _tokens = 0U;
    _bytecodeBytes = 0U;
    _layoutCount = 0U;
    _outputBytes = 0U;
    _inheritanceEdges = 0U;
    _blockPrograms = 0U;
    _renderCounters = {};
    _compiledLayouts.clear();
    for (const auto &layout : _layouts) {
        auto sourceBytes = std::uint64_t{};
        auto tokens = std::uint64_t{};
        auto tokenizeDigest = std::uint64_t{};
        for (const auto &[name, source] : layout.sources) {
            sourceBytes += source->text().length().toRawValue();
            auto tokenizer = ri::Tokenizer{name, source->origin(), source->text(), layout.options};
            do {
                tokenizer.advance();
                ++tokens;
                tokenizeDigest = ValidationDigest::token(tokenizeDigest, tokenizer.current());
            } while (tokenizer.current().kind != ri::TokenKind::End);
        }
        const auto compiled = LayoutGraphCompiler::compile(layout, _filters);
        const auto graph = LayoutGraphCompiler::validate(compiled);
        auto renderCounters = ri::RenderCounters{};
        const auto output = ri::Engine{compiled, layout.context, layout.context, renderCounters}.render();
        const auto outputBytes = output.length().toRawValue();
        const auto renderDigest = ValidationDigest::rendered(0U, output);
        const auto &expected = layout.validation;
        if (sourceBytes != expected.sourceBytes || tokens != expected.tokens ||
            graph.bytecodeBytes != expected.bytecodeBytes || tokenizeDigest != expected.tokenizeDigest ||
            graph.digest != expected.compileDigest || outputBytes != expected.outputBytes ||
            renderDigest != expected.renderDigest) {
            throw el::ApplicationError{el::StringFormat{
                "Corpus '{}' validation is source={} tokens={} bytecode={} tokenize-digest={} compile-digest={} "
                "output={} render-digest={} layouts={} includes={} inheritance-edges={} block-programs={} "
                "include-executions={} block-executions={} super-executions={} captured-super-executions={}."_el}
                    .build(
                        layout.id,
                        sourceBytes,
                        tokens,
                        graph.bytecodeBytes,
                        tokenizeDigest,
                        graph.digest,
                        outputBytes,
                        renderDigest,
                        graph.layouts,
                        graph.includes,
                        graph.inheritanceEdges,
                        graph.blockPrograms,
                        renderCounters.includeExecutions,
                        renderCounters.blockExecutions,
                        renderCounters.superExecutions,
                        renderCounters.capturedSuperExecutions)};
        }
        _sourceBytes += sourceBytes;
        _tokens += tokens;
        _bytecodeBytes += graph.bytecodeBytes;
        _layoutCount += graph.layouts;
        _outputBytes += outputBytes;
        _inheritanceEdges += graph.inheritanceEdges;
        _blockPrograms += graph.blockPrograms;
        _renderCounters.includeExecutions += renderCounters.includeExecutions;
        _renderCounters.blockExecutions += renderCounters.blockExecutions;
        _renderCounters.superExecutions += renderCounters.superExecutions;
        _renderCounters.capturedSuperExecutions += renderCounters.capturedSuperExecutions;
        _compiledLayouts.append(compiled);
    }
}

}
