// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"

#include "DefaultConfiguration.hpp"
#include "PathWalkerScenarioWorkload.hpp"

#include <memory>

namespace app::path {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

void PathWalkerProfileApplication::configureProfiling(pf::ProfilingDefinition &definition) {
    definition
        .setApplication(
            "PathWalker Profiling"_el,
            el::Version{1, 0, 0},
            "PathWalker Profiler and Benchmark"_el,
            "Profiles recursive PathWalker discovery and compares it with the standard-library iterator."_el)
        .setDefaultConfiguration(el::String{cDefaultConfigurationText})
        .addMetric({.id = "entries"_el, .unit = "entries"_el, .reportRate = true})
        .addParameter(
            {.id = "root"_el,
                .configurationName = "root"_el,
                .type = pf::ParameterType::Path,
                .defaultValue = el::Path{"."_el}})
        .addFunctionality(
            {.id = "path-callback"_el,
                .description = "Walk with the Path-only callback."_el,
                .api = el::StringList{"path::PathWalker::walkOrThrow(PathWalkFn)"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<PathWalkerScenarioWorkload>(PathWalkMethod::PathCallback);
                }})
        .addFunctionality(
            {.id = "path-info-callback"_el,
                .description = "Walk with the prefetched PathInfo callback."_el,
                .api = el::StringList{"path::PathWalker::walkOrThrow(PathInfoWalkFn)"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<PathWalkerScenarioWorkload>(PathWalkMethod::PathInfoCallback);
                }})
        .addFunctionality(
            {.id = "std-recursive"_el,
                .description = "Walk with std::filesystem::recursive_directory_iterator."_el,
                .api = el::StringList{"std::filesystem::recursive_directory_iterator"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<PathWalkerScenarioWorkload>(PathWalkMethod::StdRecursive);
                }});

    auto scenarios = el::List<pf::Scenario>{};
    scenarios.append({.id = "snapshot:path-callback"_el, .group = "snapshot"_el, .functionality = "path-callback"_el});
    scenarios.append(
        {.id = "snapshot:path-info-callback"_el, .group = "snapshot"_el, .functionality = "path-info-callback"_el});
    scenarios.append({.id = "snapshot:std-recursive"_el, .group = "snapshot"_el, .functionality = "std-recursive"_el});
    definition.addSuite({.id = "snapshot"_el, .scenarios = scenarios})
        .addSuite({.id = "smoke"_el, .scenarios = std::move(scenarios)});
}

auto PathWalkerProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
