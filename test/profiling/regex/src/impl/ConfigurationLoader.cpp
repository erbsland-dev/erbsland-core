// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfigurationLoader.hpp"

#include "BuiltInConfiguration.hpp"
#include "DefaultConfiguration.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathWriteTextOptions.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringSet.hpp>

namespace app::regex::impl {

auto ConfigurationLoader::load() const -> Configuration {
    auto parser = el::conf::Parser{};
    auto run = RunSettings{};
    auto templates = std::vector<ScenarioTemplate>{};
    parseDocument(parser.parseTextOrThrow(cDefaultConfigurationText), run, templates, false, {});
    if (_path) {
        parseDocument(parser.parseFileOrThrow(*_path), run, templates, true, _path->parent());
    }
    auto result = Configuration{.run = run, .scenarios = {}};
    if (templates.empty()) {
        result.scenarios = BuiltInConfiguration::suite(result.run);
    } else {
        auto ids = el::StringSet{};
        for (const auto &source : templates) {
            const auto firstNewScenario = result.scenarios.size();
            expandTemplate(source, result.scenarios);
            for (auto index = firstNewScenario; index < result.scenarios.size(); ++index) {
                const auto &scenario = result.scenarios[index];
                if (!ids.tryInsert(scenario.id)) {
                    configError(
                        el::StringFormat{"Duplicate expanded scenario ID '{}'."_el}.build(scenario.id),
                        source.sourceValue);
                }
            }
        }
    }
    if (result.scenarios.empty()) {
        configError("The configuration expands to no scenarios."_el);
    }
    auto ids = el::StringSet{};
    for (const auto &scenario : result.scenarios) {
        if (!ids.tryInsert(scenario.id)) {
            configError(el::StringFormat{"Duplicate expanded scenario ID '{}'."_el}.build(scenario.id));
        }
    }
    return result;
}

void ConfigurationLoader::writeTemplate(const el::Path &path) {
    auto options = el::PathWriteTextOptions{el::StringEncoding::Utf8};
    options.setCreateParents(true).setBomMode(el::StringBomMode::Reject);
    path.content().writeTextOrThrow(cDefaultConfigurationText, options);
}

}
