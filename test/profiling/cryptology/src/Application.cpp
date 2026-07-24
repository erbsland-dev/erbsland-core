// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"

#include "DefaultConfiguration.hpp"
#include "HashScenarioWorkload.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>

#include <memory>

namespace app::cryptology {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void CryptologyHashProfileApplication::configureProfiling(pf::ProfilingDefinition &definition) {
    definition
        .setApplication(
            "Cryptology Hash Profiling"_el,
            el::Version{1, 0, 0},
            "Cryptology Hash Profiler and Benchmark"_el,
            "Profiles fixed-output hash algorithms using deterministic in-memory input."_el)
        .setDefaultConfiguration(el::String{cDefaultConfigurationText})
        .addMetric({.id = "bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addFunctionality(
            {.id = "hash"_el,
                .description = "Hash deterministic in-memory input."_el,
                .api = el::StringList{"cryptology::Hasher::update"_el, "cryptology::Hasher::finalize"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<HashScenarioWorkload>();
                }})
        .addParameter(
            {.id = "input-size"_el,
                .configurationName = "input_size"_el,
                .type = pf::ParameterType::ByteLength,
                .defaultValue = el::ByteLength{128U * 1024U * 1024U}})
        .addParameter(
            {.id = "fail-worker"_el,
                .configurationName = "fail_worker"_el,
                .type = pf::ParameterType::Boolean,
                .defaultValue = false});

    auto algorithmAxis = pf::AxisDefinition{"algorithm"_el, "algorithm"_el, "algorithm"_el};
    for (const auto algorithm : el::cryptology::HashAlgorithm::all()) {
        algorithmAxis.addValue({.id = algorithm.toString(), .description = algorithm.toString()});
    }
    definition.addAxis(std::move(algorithmAxis));

    auto snapshotScenarios = el::List<pf::Scenario>{};
    for (const auto algorithm : el::cryptology::HashAlgorithm::all()) {
        const auto algorithmId = algorithm.toString();
        snapshotScenarios.append(
            pf::Scenario{
                .id = el::StringFormat{"snapshot:hash:{}"_el}.build(algorithmId),
                .group = "snapshot"_el,
                .functionality = "hash"_el,
                .axes = el::List<pf::AxisSelection>{{.axis = "algorithm"_el, .value = algorithmId}},
                .parameters = el::List<pf::ParameterSelection>{
                    {.parameter = "input-size"_el, .value = el::ByteLength{128U * 1024U * 1024U}}}});
    }
    auto smokeScenario = pf::Scenario{.id = "smoke:hash:md5"_el, .group = "smoke"_el, .functionality = "hash"_el};
    smokeScenario.axes.append({.axis = "algorithm"_el, .value = "md5"_el});
    smokeScenario.parameters.append({.parameter = "input-size"_el, .value = el::ByteLength{64U * 1024U}});
    definition.addSuite({.id = "snapshot"_el, .scenarios = std::move(snapshotScenarios)})
        .addSuite({.id = "smoke"_el, .scenarios = el::List<pf::Scenario>{std::move(smokeScenario)}});
}

auto CryptologyHashProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
