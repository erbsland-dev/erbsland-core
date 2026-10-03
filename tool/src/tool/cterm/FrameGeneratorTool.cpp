// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FrameGeneratorTool.hpp"

#include "BoxFrameSource.hpp"

#include "../frame/ToolApplication.hpp"
#include "../unicode/UnicodeSource.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::cterm {

FrameGeneratorTool::FrameGeneratorTool(el::String defaultOutput) : _defaultOutput{std::move(defaultOutput)} {
}

auto FrameGeneratorTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"-o"_el, "--output-file"_el, cOutputFileOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Choose the output file"_el);
    module->addOption({"-c"_el, "--check"_el, cCheckOption})
        .setHelpTitle("Check generated output"_el)
        .setHelpDescription("Fail if the output differs, without writing files."_el);
    return module;
}

auto FrameGeneratorTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Terminal frame generation"_el}, Description{"Destination for generated terminal frame tables."_el});
    rules.addRule(
        cOutputFileConfig,
        RuleType::Text,
        Title{"Generated Terminal Frame File"_el},
        Description{"The project-relative output file for this generator."_el},
        Default{_defaultOutput});
    return rules.takeRules();
}

auto FrameGeneratorTool::usedDataSources() const -> el::StringList {
    return el::StringList{unicode::UnicodeSource::sourceIdentifier(), BoxFrameSource::sourceIdentifier()};
}

void FrameGeneratorTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    auto &files = frame::app().files();
    if (const auto option = values->value(cOutputFileOption)) {
        _outputFile = files.checked(option, el::PathType::RegularFile, true);
    } else {
        _outputFile = files.checked(configuration->valueOrThrow(cOutputFileConfig), el::PathType::RegularFile, true);
    }
    _checkOnly = values->getFlag(cCheckOption);
}

void FrameGeneratorTool::execute() {
    const auto output = render(frame::app().source<BoxFrameSource>().data());
    const auto changed = frame::app().files().updateText(_outputFile, output, _checkOnly);
    log()->info(_outputFile, changed ? (_checkOnly ? ": differs"_el : ": updated"_el) : ": unchanged"_el);
    if (_checkOnly && changed) {
        throw el::ApplicationError{
            "Generated Output Is Out of Date"_el,
            "The terminal frame data differs. Run without --check to update it."_el};
    }
}

auto FrameGeneratorTool::arrayContext(
    const el::String &name, const el::String &type, const std::vector<uint32_t> &values, const uint32_t width)
    -> el::text::render::ValueMap {
    static const auto cHex2 = el::StringFormat{"0x{:number:hex,precision=2,lc=uppercase}U,"_el};
    static const auto cHex4 = el::StringFormat{"0x{:number:hex,precision=4,lc=uppercase}U,"_el};
    static const auto cHex7 = el::StringFormat{"0x{:number:hex,precision=7,lc=uppercase}U,"_el};
    auto result = el::text::render::ValueMap{};
    result.set("name"_el, name);
    result.set("type"_el, type);
    result.set(
        "return_type"_el, type == "uint8_t"_el ? type : el::join("CommonBoxFrameBlockCombinationStyle::"_el, type));
    result.set("count"_el, values.size());
    result.set("row_size"_el, width == 2U ? 16U : 8U);
    auto entries = el::text::render::ValueList{};
    for (const auto value : values) {
        if (width == 2U) {
            entries.append(cHex2.build(value));
        } else if (width == 4U) {
            entries.append(cHex4.build(value));
        } else {
            entries.append(cHex7.build(value));
        }
    }
    result.set("values"_el, entries);
    return result;
}

}
