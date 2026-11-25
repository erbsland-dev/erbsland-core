// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicSuffixTool.hpp"

#include "PublicSuffixSource.hpp"

#include "../idna/LabelProcessor.hpp"
#include "../unicode/UnicodeSource.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringEditor.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::psl {

using namespace el::text::literals;

auto PublicSuffixTool::identifier() const -> el::String {
    return "generate_public_suffix_data"_el;
}

auto PublicSuffixTool::title() const -> el::String {
    return "Generate compact Public Suffix List tables."_el;
}

auto PublicSuffixTool::description() const -> el::String {
    return "Generate the ICANN and PRIVATE Public Suffix List tables from pinned sources. "
           "Unicode mapping, NFC normalization, and IDNA validation use the selected UCD "
           "snapshot independently of Core's compiled Unicode tables."_el;
}

auto PublicSuffixTool::optionModule() const -> el::OptionModulePtr {
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

auto PublicSuffixTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;

    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Public suffix generation"_el}, Description{"Destination for generated public suffix tables."_el});
    rules.addRule(
        cOutputFileConfig,
        RuleType::Text,
        Title{"Generated Source File"_el},
        Description{"The generated C++ destination inside the selected project."_el},
        Default{"src/erbsland/network/impl/http/cookie/PublicSuffixData.cpp"_el});
    return rules.takeRules();
}

auto PublicSuffixTool::usedDataSources() const -> el::StringList {
    return el::StringList{unicode::UnicodeSource::sourceIdentifier(), PublicSuffixSource::sourceIdentifier()};
}

void PublicSuffixTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    auto &files = frame::app().files();
    if (const auto option = values->value(cOutputFileOption)) {
        _outputFile = files.checked(option, el::PathType::RegularFile, true);
    } else {
        _outputFile = files.checked(configuration->valueOrThrow(cOutputFileConfig), el::PathType::RegularFile, true);
    }
    _checkOnly = values->getFlag(cCheckOption);
}

void PublicSuffixTool::execute() {
    log()->info("Generating compact Public Suffix List tables..."_el);
    const auto &unicodeSource = frame::app().source<unicode::UnicodeSource>();
    const auto &snapshot = frame::app().source<PublicSuffixSource>();
    const auto processor =
        idna::LabelProcessor{unicodeSource.idnaData(), unicodeSource.normalizer(), unicodeSource.mapping()};
    const auto parsed = PublicSuffixList{snapshot.text(), snapshot.inputFile().toString(), processor};
    const auto data = EncodedData{parsed.rules()};
    const auto generated = render(parsed.version(), data);
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

auto PublicSuffixTool::renderArray(
    const el::String &type, const el::String &name, const el::List<unsigned> &values, const bool hexadecimal)
    -> el::String {
    auto result =
        el::StringEditor{el::StringFormat{"    static constexpr auto {} = std::array<{}, {}U>{{{{\n"_el}.build(
            name, type, values.count().toSizeT())};
    const auto format = el::StringFormat{hexadecimal ? "0x{:number:hex,precision=2,lc=lowercase}U"_el : "{}U"_el};
    auto entries = el::StringList{};
    for (const auto value : values) {
        entries.append(format.build(value));
        if (entries.count() == el::ItemCount{16}) {
            result.append("        "_el).append(entries.join(", "_el)).append(",\n"_el);
            entries.clear();
        }
    }
    if (!entries.isEmpty()) {
        result.append("        "_el).append(entries.join(", "_el)).append(",\n"_el);
    }
    result.append("    }};\n"_el);
    return result;
}

auto PublicSuffixTool::render(const el::String &version, const EncodedData &data) -> el::String {
    data.verify();
    const auto renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    auto context = el::text::render::Context{};
    auto arrays = el::StringList{};
    for (
        const auto &[name, bytes] : el::List<std::pair<el::String, el::ByteBlock>>{
            {"cEncodedRules"_el, data.encodedRules()},
            {"cPrefixTree"_el, data.prefixTree()},
            {"cContentTree"_el, data.contentTree()}}) {
        auto values = el::List<unsigned>{};
        for (const auto byte : bytes.span()) {
            values.append(byte.toUInt8());
        }
        arrays.append(renderArray("std::uint8_t"_el, name, values, true));
    }
    auto offsets = el::List<unsigned>{};
    for (const auto value : data.blockOffsets()) {
        offsets.append(value);
    }
    arrays.insert(el::ItemIndex{1}, renderArray("std::uint32_t"_el, "cBlockBitOffsets"_el, offsets, false));
    context.set("arrays"_el, arrays.join("\n"_el));
    context.set("version"_el, version);
    context.set("rule_count"_el, data.rules().count().toSizeT());
    context.set("storage_size"_el, data.storageByteCount().toSizeT());
    context.set("legacy_size"_el, data.originalTextByteCount().toSizeT() + data.rules().count().toSizeT() * 8U);
    context.set("maximum_rule_length"_el, data.maximumRuleLength().toSizeT());
    context.set("maximum_label_count"_el, data.maximumLabelCount());
    context.set("original_text_size"_el, data.originalTextByteCount().toSizeT());
    auto counts = std::array<unsigned, 3>{};
    for (const auto &rule : data.rules()) {
        ++counts[rule.token];
    }
    context.set("exact_count"_el, counts[0]);
    context.set("wildcard_count"_el, counts[1]);
    context.set("exception_count"_el, counts[2]);
    return renderer->render("psl/public_suffix_data.cpp"_el, context);
}

void PublicSuffixTool::throwOutputMismatch() const {
    throw el::ApplicationError{
        "Generated Output Is Out of Date"_el,
        "The existing output differs from the generated Public Suffix List data. Run without --check to update it."_el}
        .withSourcePath(_outputFile.toString());
}

}
