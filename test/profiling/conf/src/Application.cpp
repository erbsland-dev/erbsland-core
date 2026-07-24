// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"

#include "DefaultConfiguration.hpp"
#include "ParserScenarioWorkload.hpp"

namespace app::conf {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void ConfParserProfileApplication::configureProfiling(pf::ProfilingDefinition &definition) {
    definition
        .setApplication(
            "ELCL Parser Profiling"_el,
            el::Version{1, 0, 0},
            "ELCL Parser Profiler and Benchmark"_el,
            "Profiles the Erbsland Configuration Language parser using a deterministic embedded corpus."_el)
        .setDefaultConfiguration(el::String{cDefaultConfigurationText})
        .addMetric({.id = "documents"_el, .unit = "documents"_el, .reportRate = true})
        .addMetric({.id = "bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addParameter(
            {.id = "fail-worker"_el,
                .configurationName = "fail_worker"_el,
                .type = pf::ParameterType::Boolean,
                .defaultValue = false})
        .addFunctionality(
            {.id = "parse"_el,
                .description = "Parse the complete embedded ELCL corpus."_el,
                .api = el::StringList{"conf::Parser::parseTextOrThrow"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<ParserScenarioWorkload>();
                }})
        .addSuite(
            {.id = "snapshot"_el,
                .scenarios = el::List<pf::Scenario>{pf::Scenario{
                    .id = "snapshot:parse:embedded-corpus"_el, .group = "snapshot"_el, .functionality = "parse"_el}}})
        .addSuite(
            {.id = "smoke"_el,
                .scenarios = el::List<pf::Scenario>{pf::Scenario{
                    .id = "smoke:parse:embedded-corpus"_el, .group = "smoke"_el, .functionality = "parse"_el}}});
}

auto ConfParserProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
