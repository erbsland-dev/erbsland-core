// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IdnaTool.hpp"

#include "../frame/ToolApplication.hpp"
#include "../unicode/UnicodeSource.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StandardStreams.hpp>

namespace tool::idna {

using namespace el::text;
using namespace el::text::literals;

auto IdnaTool::identifier() const -> el::String {
    return "generate_idna_data"_el;
}

auto IdnaTool::title() const -> el::String {
    return "Generate compact Unicode IDNA runtime tables."_el;
}

auto IdnaTool::description() const -> el::String {
    return "This tool generates the compact IDNA runtime tables from the original Unicode UCD "
           "sources. The tool reads and interprets the tables from the configured UCD directory "
           "and writes generated C++ code for the Erbsland Core library."_el;
}

auto IdnaTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"-o"_el, "--output-file"_el, "--output"_el, cOutputFileOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Choose an output file"_el)
        .setHelpDescription("Override the output path for the generated source file from the configuration."_el);
    module->addOption({"-c"_el, "--check"_el, cCheckOption})
        .setHelpTitle("Check generated output"_el)
        .setHelpDescription("Fail if output differs, without writing files."_el);
    return module;
}

auto IdnaTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;

    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(Title{"IDNA generation"_el}, Description{"Destination for generated IDNA mapping tables."_el});
    rules.addRule(
        cOutputFileConfig,
        RuleType::Text,
        Title{"Generated Source File"_el},
        Description{"The generated C++ destination inside the selected project."_el},
        Default{"src/erbsland/text/punycode/impl/IdnaData.cpp"_el});
    return rules.takeRules();
}

auto IdnaTool::usedDataSources() const -> el::StringList {
    return el::StringList{unicode::UnicodeSource::sourceIdentifier()};
}

void IdnaTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    auto &files = frame::app().files();
    if (const auto option = values->value(cOutputFileOption)) {
        _outputFile = files.checked(option, el::PathType::RegularFile, true);
    } else {
        _outputFile = files.checked(configuration->valueOrThrow(cOutputFileConfig), el::PathType::RegularFile, true);
    }
    _checkOnly = values->getFlag(cCheckOption);
}

void IdnaTool::execute() {
    log()->info("Generating compact Unicode IDNA runtime tables..."_el);
    const auto &source = frame::app().source<unicode::UnicodeSource>();
    log()->trace("Unicode Version: "_el, source.version());
    const auto &data = source.idnaData();
    const auto generated = render(data);
    const auto changed = frame::app().files().updateText(_outputFile, generated, _checkOnly);
    el::String status;
    if (changed) {
        if (_checkOnly) {
            status = "differs"_el;
        } else {
            status = "updated"_el;
        }
    } else {
        status = "unchanged"_el;
    }
    log()->info(_outputFile, ": "_el, status);
    if (_checkOnly && changed) {
        throwOutputMismatch();
    }
}

auto IdnaTool::render(const IdnaData &data) -> el::String {
    const auto renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    const auto ranges = data.ranges();
    const auto offsets = IdnaData::pageOffsets(ranges);
    const auto hexValueFormat = el::StringFormat{"0x{:number:hex,precision=4,lc=uppercase}U"_el};
    auto context = render::Context{};
    context.set("range_count"_el, ranges.count().toSizeT());
    context.set("page_count"_el, offsets.count().toSizeT());
    auto entries = render::ValueList{};
    for (const auto &range : ranges) {
        auto entry = render::ValueMap{};
        entry.set("first"_el, hexValueFormat.build(static_cast<unsigned>(range.first) & 0xFFFFU));
        entry.set("last"_el, hexValueFormat.build(static_cast<unsigned>(range.last) & 0xFFFFU));
        entry.set("attributes"_el, hexValueFormat.build(range.attributes));
        entries.append(render::Value{entry});
    }
    context.set("ranges"_el, entries);
    auto offsetTexts = el::StringList{};
    const auto offsetFormat = el::StringFormat{"{}U"_el};
    for (const auto value : offsets) {
        offsetTexts.append(offsetFormat.build(value.toRawValue()));
    }
    context.set("page_offsets"_el, offsetTexts.join(", "_el));
    return renderer->render("idna/idna_data.cpp"_el, context);
}

void IdnaTool::throwOutputMismatch() const {
    throw el::ApplicationError{
        "Generated Output Is Out of Date"_el,
        "The existing output differs from the generated IDNA data. Run without --check to update it."_el}
        .withSourcePath(_outputFile.toString());
}

}
