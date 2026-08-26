// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"
#include "Corpus.hpp"
#include "DefaultConfiguration.hpp"
#include "RenderScenarioWorkload.hpp"

#include <erbsland/text/StringFormat.hpp>

namespace app::render {

namespace el = erbsland;
namespace pf = erbsland::profiling;

using namespace el::text::literals;

void RenderProfileApplication::configureProfiling(pf::ProfilingDefinition &definition) {
    auto corpusAxis = pf::AxisDefinition{"corpus"_el, "corpora"_el, "corpus"_el};
    for (const auto &entry : corpus::entries()) {
        corpusAxis.addValue({.id = entry.id, .description = entry.group});
    }
    corpusAxis.addValue({.id = "legacy"_el, .description = "Preserved pre-completion aggregate corpus."_el});
    corpusAxis.addValue({.id = "all"_el, .description = "Complete aggregate corpus."_el});

    auto snapshot = el::List<pf::Scenario>{};
    for (const auto functionality : {"tokenize"_el, "compile"_el, "render"_el}) {
        for (const auto &entry : corpus::entries()) {
            snapshot.append(
                pf::Scenario{
                    .id = el::StringFormat{"snapshot:{}:{}"_el}.build(functionality, entry.id),
                    .group = entry.group,
                    .functionality = functionality,
                    .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, entry.id}}});
        }
        snapshot.append(
            pf::Scenario{
                .id = el::StringFormat{"snapshot:{}:legacy"_el}.build(functionality),
                .group = "legacy-aggregate"_el,
                .functionality = functionality,
                .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "legacy"_el}}});
        snapshot.append(
            pf::Scenario{
                .id = el::StringFormat{"snapshot:{}:all"_el}.build(functionality),
                .group = "aggregate"_el,
                .functionality = functionality,
                .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "all"_el}}});
    }

    const auto smoke = el::List<pf::Scenario>{
        pf::Scenario{
            .id = "smoke:tokenize:email"_el,
            .group = "smoke"_el,
            .functionality = "tokenize"_el,
            .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "email"_el}}},
        pf::Scenario{
            .id = "smoke:compile:email"_el,
            .group = "smoke"_el,
            .functionality = "compile"_el,
            .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "email"_el}}},
        pf::Scenario{
            .id = "smoke:render:include-site"_el,
            .group = "smoke"_el,
            .functionality = "render"_el,
            .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "include-site"_el}}},
        pf::Scenario{
            .id = "smoke:render:inheritance-stress"_el,
            .group = "smoke"_el,
            .functionality = "render"_el,
            .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "inheritance-stress"_el}}},
    };

    auto allocations = el::List<pf::Scenario>{};
    for (const auto functionality : {"tokenize"_el, "compile"_el, "render"_el}) {
        allocations.append(
            pf::Scenario{
                .id = el::StringFormat{"allocations:{}:language-completion.html"_el}.build(functionality),
                .group = "allocations"_el,
                .functionality = functionality,
                .axes = el::List<pf::AxisSelection>{pf::AxisSelection{"corpus"_el, "language-completion.html"_el}},
                .parameters = el::List<pf::ParameterSelection>{{.parameter = "track-allocations"_el, .value = true}}});
    }

    definition
        .setApplication(
            "Layout Rendering Profiling"_el,
            el::Version{1, 0, 0},
            "Layout Tokenizer, Compiler and Renderer Profiler"_el,
            "Profiles deterministic layout graphs without loader, resource-provider or cache timing."_el)
        .setDefaultConfiguration(el::String{cDefaultConfigurationText})
        .addAxis(std::move(corpusAxis))
        .addMetric({.id = "layouts"_el, .unit = "layouts"_el, .reportRate = true})
        .addMetric({.id = "source-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addMetric({.id = "tokens"_el, .unit = "tokens"_el, .reportRate = true})
        .addMetric({.id = "bytecode-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addMetric({.id = "output-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addMetric({.id = "include-executions"_el, .unit = "includes"_el, .reportRate = true})
        .addMetric({.id = "inheritance-edges"_el, .unit = "edges"_el, .reportRate = true})
        .addMetric({.id = "block-programs"_el, .unit = "blocks"_el, .reportRate = true})
        .addMetric({.id = "block-executions"_el, .unit = "blocks"_el, .reportRate = true})
        .addMetric({.id = "super-executions"_el, .unit = "supers"_el, .reportRate = true})
        .addMetric({.id = "captured-super-executions"_el, .unit = "supers"_el, .reportRate = true})
        .addMetric({.id = "allocations"_el, .unit = "allocations"_el, .reportRate = true})
        .addMetric({.id = "allocated-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addMetric({.id = "deallocations"_el, .unit = "deallocations"_el, .reportRate = true})
        .addParameter(
            {.id = "track-allocations"_el,
                .configurationName = "track_allocations"_el,
                .type = pf::ParameterType::Boolean,
                .defaultValue = false})
        .addFunctionality(
            {.id = "tokenize"_el,
                .description = "Tokenize complete layout sources."_el,
                .api = el::StringList{"text::render::impl::Tokenizer"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<RenderScenarioWorkload>(ProfileStage::Tokenize);
                }})
        .addFunctionality(
            {.id = "compile"_el,
                .description = "Compile complete layout sources into immutable bytecode."_el,
                .api = el::StringList{"text::render::impl::Compiler"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<RenderScenarioWorkload>(ProfileStage::Compile);
                }})
        .addFunctionality(
            {.id = "render"_el,
                .description = "Render precompiled immutable dependency graphs."_el,
                .api = el::StringList{"text::render::impl::Engine"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<RenderScenarioWorkload>(ProfileStage::Render);
                }})
        .addSuite({.id = "snapshot"_el, .scenarios = std::move(snapshot)})
        .addSuite({.id = "allocations"_el, .scenarios = std::move(allocations)})
        .addSuite({.id = "smoke"_el, .scenarios = smoke});
}

auto RenderProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
