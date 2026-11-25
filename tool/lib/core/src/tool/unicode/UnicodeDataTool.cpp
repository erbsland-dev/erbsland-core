// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeDataTool.hpp"

#include "UnicodeLightTables.hpp"
#include "UnicodeNormalizationTables.hpp"
#include "UnicodeSource.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionType.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::unicode {

using namespace el::text::literals;

auto UnicodeDataTool::identifier() const -> el::String {
    return "generate_unicode_data"_el;
}

auto UnicodeDataTool::title() const -> el::String {
    return "Generate Unicode Light and normalization tables."_el;
}

auto UnicodeDataTool::description() const -> el::String {
    return "Generate the Unicode Light, version, and normalization tables from the verified UCD snapshot."_el;
}

auto UnicodeDataTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"-o"_el, "--output-directory"_el, cOutputOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Choose the output directory"_el);
    module->addOption({"-c"_el, "--check"_el, cCheckOption})
        .setHelpTitle("Check generated output"_el)
        .setHelpDescription("Fail if any output differs, without writing files."_el);
    return module;
}

auto UnicodeDataTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(Title{"Unicode generation"_el}, Description{"Destination for generated Unicode tables."_el});
    rules.addRule(
        cOutputDirectoryConfig,
        RuleType::Text,
        Title{"Generated Unicode Data Directory"_el},
        Description{"The project directory containing the generated Unicode translation units."_el},
        Default{"src/erbsland/text/impl"_el});
    return rules.takeRules();
}

auto UnicodeDataTool::usedDataSources() const -> el::StringList {
    return el::StringList{UnicodeSource::sourceIdentifier()};
}

void UnicodeDataTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    auto &files = frame::app().files();
    if (const auto option = values->value(cOutputOption)) {
        _outputDirectory = files.checked(option, el::PathType::Directory, true);
    } else {
        _outputDirectory =
            files.checked(configuration->valueOrThrow(cOutputDirectoryConfig), el::PathType::Directory, true);
    }
    _checkOnly = values->getFlag(cCheckOption);
}

void UnicodeDataTool::execute() {
    const auto &source = frame::app().source<UnicodeSource>();
    const auto light = UnicodeLightTables{source};
    const auto normalization = UnicodeNormalizationTables{source};
    const auto outputs = el::List<std::pair<el::Path, el::String>>{
        {_outputDirectory / "UnicodeData.cpp"_el, light.renderData()},
        {_outputDirectory / "UnicodeVersion.cpp"_el, UnicodeLightTables::renderVersion(source.version())},
        {_outputDirectory / "UnicodeNormalizationData.cpp"_el, normalization.render()}};
    auto changed = false;
    for (const auto &[path, text] : outputs) {
        if (frame::app().files().updateText(path, text, _checkOnly)) {
            changed = true;
            log()->info(path, _checkOnly ? ": differs"_el : ": updated"_el);
        } else {
            log()->info(path, ": unchanged"_el);
        }
    }
    if (_checkOnly && changed) {
        throw el::ApplicationError{
            "Generated Output Is Out of Date"_el,
            "One or more Unicode data files differ. Run without --check to update them."_el};
    }
}

}
