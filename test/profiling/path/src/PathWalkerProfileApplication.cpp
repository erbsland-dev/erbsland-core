// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Application.hpp"
#include "DefaultConfiguration.hpp"
#include "PathWalkerScenarioWorkload.hpp"
#include "SourceDirectory.hpp"
#include "SourceReadMethod.hpp"
#include "SourceTreeScenarioWorkload.hpp"

#include <erbsland/ExitCode.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/profiling/ParameterSelection.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/Version.hpp>

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
            "Profiles recursive discovery and whole-file reads over real source trees."_el)
        .setDefaultConfiguration(cDefaultConfigurationText)
        .addMetric({.id = "entries"_el, .unit = "entries"_el, .reportRate = true})
        .addMetric({.id = "files"_el, .unit = "files"_el, .reportRate = true})
        .addMetric({.id = "returned-bytes"_el, .unit = "bytes"_el, .reportRate = true})
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
                }})
        .addFunctionality(
            {.id = "walk-read-text"_el,
                .description = "Walk C++ sources and read each file with tolerant text decoding."_el,
                .api =
                    el::StringList{
                        "path::PathWalker::walkOrThrow(PathInfoWalkFn)"_el, "path::PathContent::readTextOrThrow"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<SourceTreeScenarioWorkload>(SourceReadMethod::Text);
                }})
        .addFunctionality(
            {.id = "walk-read-data"_el,
                .description = "Walk C++ sources and read each file as bytes."_el,
                .api =
                    el::StringList{
                        "path::PathWalker::walkOrThrow(PathInfoWalkFn)"_el, "path::PathContent::readDataOrThrow"_el},
                .factory = [](const pf::Scenario &) -> pf::ScenarioWorkloadPtr {
                    return std::make_shared<SourceTreeScenarioWorkload>(SourceReadMethod::Data);
                }});

    auto scenarios = el::List<pf::Scenario>{};
    scenarios.append({.id = "snapshot:path-callback"_el, .group = "snapshot"_el, .functionality = "path-callback"_el});
    scenarios.append(
        {.id = "snapshot:path-info-callback"_el, .group = "snapshot"_el, .functionality = "path-info-callback"_el});
    scenarios.append({.id = "snapshot:std-recursive"_el, .group = "snapshot"_el, .functionality = "std-recursive"_el});
    definition.addSuite({.id = "snapshot"_el, .scenarios = scenarios})
        .addSuite({.id = "smoke"_el, .scenarios = std::move(scenarios)});

    auto sourceScenarios = el::List<pf::Scenario>{};
    for (const auto &directory : el::StringList{"demos"_el, "src"_el, "test"_el}) {
        const auto root = el::Path{el::String{cSourceDirectory}} / directory;
        for (const auto &method : el::StringList{"text"_el, "data"_el}) {
            sourceScenarios.append(
                {.id = el::StringFormat{"source-tree:{}:{}"_el}.build(directory, method),
                    .group = "source-tree"_el,
                    .functionality = el::StringFormat{"walk-read-{}"_el}.build(method),
                    .parameters = el::List<pf::ParameterSelection>{{.parameter = "root"_el, .value = root}}});
        }
    }
    definition.addSuite({.id = "source-tree"_el, .scenarios = std::move(sourceScenarios)});
}

auto PathWalkerProfileApplication::executeProfiling() -> el::ExitCode {
    return runRegisteredProfiling();
}

}
