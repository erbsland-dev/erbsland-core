// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Configuration.hpp"

#include "DefaultConfiguration.hpp"

#include "impl/ConfigurationTools.hpp"

#include <erbsland/conf/Parser.hpp>

#include <set>

namespace app::string {

using namespace el::text::literals;

auto ConfigurationLoader::load(const std::optional<el::Path> &path, const RunOverrides &overrides) -> Configuration {
    auto parser = el::conf::Parser{};
    auto run = RunSettings{};
    auto templates = std::vector<impl::ScenarioTemplate>{};
    impl::parseDocument(parser.parseTextOrThrow(el::String{cDefaultConfigurationText}), run, templates, false);
    if (path) {
        impl::parseDocument(parser.parseFileOrThrow(*path), run, templates, true);
    }
    if (overrides.mode) {
        run.mode = *overrides.mode;
    }
    if (overrides.threadCount) {
        run.threadCount = *overrides.threadCount;
    }
    if (overrides.sensitiveSelection) {
        run.sensitiveSelection = *overrides.sensitiveSelection;
    }
    auto result = Configuration{.run = run};
    if (templates.empty()) {
        result.scenarios = impl::builtInSuite(result.run);
    } else {
        for (const auto &source : templates) {
            impl::expandTemplate(source, result.run, result.scenarios, {});
        }
    }
    if (result.scenarios.empty()) {
        impl::configError("The configuration expands to no scenarios."_el);
    }
    auto ids = std::set<el::String>{};
    for (const auto &scenario : result.scenarios) {
        if (!ids.emplace(scenario.id).second) {
            impl::configError(el::StringFormat{"Duplicate expanded scenario ID '{}'."_el}.build(scenario.id));
        }
    }
    return result;
}

void ConfigurationLoader::writeTemplate(const el::Path &path) {
    auto options = el::PathWriteTextOptions{el::StringEncoding::Utf8};
    options.setCreateParents(true).setBomMode(el::StringBomMode::Reject);
    path.content().writeTextOrThrow(el::String{cDefaultConfigurationText}, options);
}

}
