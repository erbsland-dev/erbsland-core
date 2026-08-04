// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"
#include "CharSetScenarioWorkload.hpp"
#include "DefaultConfiguration.hpp"
#include "OperationDescriptor.hpp"

#include <memory>

namespace app::charset {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void CharSetProfileApplication::configureProfiling(pf::ProfilingDefinition &definition) {
    definition
        .setApplication(
            "Character Set Profiling"_el,
            el::Version{1, 0, 0},
            "Character Set Profiler and Benchmark"_el,
            "Profiles every public CharSet API path with representative normalized range shapes."_el)
        .setDefaultConfiguration(el::String{cDefaultConfigurationText})
        .addMetric({.id = "tracked-operations"_el, .unit = "operations"_el, .reportRate = true})
        .addMetric({.id = "characters"_el, .unit = "characters"_el, .reportRate = true})
        .addMetric({.id = "allocations"_el, .unit = "allocations"_el, .reportRate = true})
        .addMetric({.id = "allocated-bytes"_el, .unit = "bytes"_el, .reportRate = true})
        .addMetric({.id = "deallocations"_el, .unit = "deallocations"_el, .reportRate = true})
        .addParameter(
            {.id = "case"_el,
                .configurationName = "case"_el,
                .type = pf::ParameterType::Text,
                .defaultValue = el::String{"default"_el}})
        .addParameter(
            {.id = "track-allocations"_el,
                .configurationName = "track_allocations"_el,
                .type = pf::ParameterType::Boolean,
                .defaultValue = false});

    for (const auto &descriptor : operationDescriptors()) {
        const auto operation = descriptor.operation;
        definition.addFunctionality(
            {.id = descriptor.id,
                .description = descriptor.description,
                .api = descriptor.api,
                .factory = [operation](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<CharSetScenarioWorkload>(operation);
                }});
    }

    const auto createScenario = [](const OperationDescriptor &descriptor,
                                    const el::String &caseName,
                                    const el::String &group,
                                    const bool trackAllocations) -> pf::Scenario {
        return pf::Scenario{
            .id = el::StringFormat{"{}:{}:{}"_el}.build(group, descriptor.id, caseName),
            .group = group,
            .functionality = descriptor.id,
            .parameters = el::List<pf::ParameterSelection>{
                {.parameter = "case"_el, .value = caseName},
                {.parameter = "track-allocations"_el, .value = trackAllocations}}};
    };

    auto snapshotScenarios = el::List<pf::Scenario>{};
    for (const auto &descriptor : operationDescriptors()) {
        for (const auto &caseName : descriptor.cases) {
            snapshotScenarios.append(createScenario(descriptor, caseName, "snapshot"_el, false));
        }
    }
    auto allocationScenarios = el::List<pf::Scenario>{};
    for (const auto &descriptor : operationDescriptors()) {
        for (const auto &caseName : descriptor.cases) {
            allocationScenarios.append(createScenario(descriptor, caseName, "allocations"_el, true));
        }
    }

    const auto isSmokeCase = [](const Operation operation, const el::String &caseName) noexcept -> bool {
        switch (operation) {
        case Operation::ConstructDefault:
            return true;
        case Operation::ConstructChar:
            return caseName == "ascii"_el;
        case Operation::ConstructInitializerList:
            return caseName == "sparse-ascii"_el;
        case Operation::Contains:
            return caseName == "sparse-last"_el;
        case Operation::UnitedWith:
            return caseName == "overlap"_el;
        case Operation::AddChar:
            return caseName == "disjoint"_el;
        case Operation::ForEachChar:
            return caseName == "range"_el;
        case Operation::CaseFolded:
            return caseName == "ascii"_el;
        case Operation::ToU8String:
            return caseName == "unicode-small"_el;
        case Operation::FromAsciiCategory:
            return caseName == "whitespace"_el;
        case Operation::FromUnicodeCategory:
            return caseName == "decimal-number"_el;
        case Operation::FromPatternU8:
            return caseName == "mixed"_el;
        default:
            return false;
        }
    };
    auto smokeScenarios = el::List<pf::Scenario>{};
    for (const auto &descriptor : operationDescriptors()) {
        for (const auto &caseName : descriptor.cases) {
            if (isSmokeCase(descriptor.operation, caseName)) {
                smokeScenarios.append(createScenario(descriptor, caseName, "smoke"_el, true));
            }
        }
    }

    definition.addSuite({.id = "snapshot"_el, .scenarios = std::move(snapshotScenarios)})
        .addSuite({.id = "allocations"_el, .scenarios = std::move(allocationScenarios)})
        .addSuite({.id = "smoke"_el, .scenarios = std::move(smokeScenarios)});
}

auto CharSetProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
