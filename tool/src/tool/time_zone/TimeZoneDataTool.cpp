// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneDataTool.hpp"

#include "TimeZoneModel.hpp"
#include "TimeZoneRenderer.hpp"
#include "TimeZoneSource.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionType.hpp>

namespace tool::time_zone {

using namespace el::text::literals;

auto TimeZoneDataTool::identifier() const -> el::String {
    return "generate_time_zone_data"_el;
}

auto TimeZoneDataTool::title() const -> el::String {
    return "Generate compact time-zone tables."_el;
}

auto TimeZoneDataTool::description() const -> el::String {
    return "Generate time-zone and Windows mapping files from verified tzdb and CLDR snapshots."_el;
}

auto TimeZoneDataTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"-o"_el, "--output-directory"_el, cOutputDirectoryOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Choose the output directory"_el);
    module->addOption({"-c"_el, "--check"_el, cCheckOption})
        .setHelpTitle("Check generated output"_el)
        .setHelpDescription("Fail if any output differs, without writing files."_el);
    return module;
}

auto TimeZoneDataTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Time zone generation"_el}, Description{"Destination for generated time zone tables."_el});
    rules.addRule(
        cOutputDirectoryConfig,
        RuleType::Text,
        Title{"Generated Time-Zone Data Directory"_el},
        Description{"The directory containing the generated compact time-zone database."_el},
        Default{"src/erbsland/time/tz/impl"_el});
    return rules.takeRules();
}

auto TimeZoneDataTool::usedDataSources() const -> el::StringList {
    return el::StringList{TimeZoneSource::sourceIdentifier()};
}

void TimeZoneDataTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    auto &files = frame::app().files();
    if (const auto option = values->value(cOutputDirectoryOption)) {
        _outputDirectory = files.checked(option, el::PathType::Directory, true);
    } else {
        _outputDirectory =
            files.checked(configuration->valueOrThrow(cOutputDirectoryConfig), el::PathType::Directory, true);
    }
    _checkOnly = values->getFlag(cCheckOption);
}

void TimeZoneDataTool::execute() {
    const auto model = TimeZoneModel{frame::app().source<TimeZoneSource>()};
    const auto rendered = TimeZoneRenderer{model};
    auto changed = false;
    for (const auto &[name, content] : rendered.outputs()) {
        const auto path = _outputDirectory / name;
        if (frame::app().files().updateText(path, content, _checkOnly)) {
            changed = true;
            log()->info(path, _checkOnly ? ": differs"_el : ": updated"_el);
        } else {
            log()->info(path, ": unchanged"_el);
        }
    }
    if (_checkOnly && changed) {
        throw el::ApplicationError{
            "Generated Output Is Out of Date"_el,
            "One or more time-zone files differ. Run without --check to update them."_el};
    }
}

}
