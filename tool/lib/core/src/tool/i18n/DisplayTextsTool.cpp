// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayTextsTool.hpp"

#include <DisplayTextCompiler.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/StringList.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::i18n {

using namespace el::text::literals;

auto DisplayTextsTool::identifier() const -> el::String {
    return "generate_display_texts"_el;
}

auto DisplayTextsTool::title() const -> el::String {
    return "Generate Core display-text tables."_el;
}

auto DisplayTextsTool::description() const -> el::String {
    return "Generate the checked-in base display-text header and source from the configured ELCL catalog."_el;
}

auto DisplayTextsTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"-c"_el, "--check"_el, cCheckOption})
        .setHelpTitle("Check generated output"_el)
        .setHelpDescription("Fail if either output differs, without writing files."_el);
    return module;
}

auto DisplayTextsTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Display text generation"_el},
        Description{"Catalog, names and destinations for generated display texts."_el});
    rules.addRule(
        cInputFileConfig,
        RuleType::Text,
        Default{"src/erbsland/i18n/impl/DisplayTextCore.elcl"_el},
        Title{"Display text catalog"_el},
        Description{"Project-relative ELCL catalog containing the base display texts."_el});
    rules.addRule(
        cOutputHeaderConfig,
        RuleType::Text,
        Default{"src/erbsland/i18n/impl/DisplayTextCore.hpp"_el},
        Title{"Generated display text header"_el},
        Description{"Project-relative destination for the generated C++ header."_el});
    rules.addRule(
        cOutputSourceConfig,
        RuleType::Text,
        Default{"src/erbsland/i18n/impl/DisplayTextCore.cpp"_el},
        Title{"Generated display text source"_el},
        Description{"Project-relative destination for the generated C++ implementation."_el});
    rules.addRule(
        cClassNameConfig,
        RuleType::Text,
        Default{"DisplayTextCore"_el},
        Title{"Generated class name"_el},
        Description{"C++ class identifier for the generated display text catalog."_el});
    rules.addRule(
        cNamespaceConfig,
        RuleType::Text,
        Default{"erbsland::i18n::impl"_el},
        Matches{R"((?:[A-Za-z][A-Za-z0-9_]*(?:::[A-Za-z][A-Za-z0-9_]*)*)?)"_el},
        Title{"Generated namespace"_el},
        Description{"Qualified C++ namespace containing the generated display text class."_el});
    return rules.takeRules();
}

void DisplayTextsTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    const auto &files = frame::app().files();
    _input = files.checked(configuration->valueOrThrow(cInputFileConfig), el::PathType::RegularFile, false);
    _header = files.checked(configuration->valueOrThrow(cOutputHeaderConfig), el::PathType::RegularFile, true);
    _source = files.checked(configuration->valueOrThrow(cOutputSourceConfig), el::PathType::RegularFile, true);
    if (_input == _header || _input == _source || _header == _source) {
        throw el::conf::ConfError{
            configuration, "Invalid Display-Text Paths"_el, "Input, header, and source must be distinct files."_el};
    }
    _className = configuration->getTextOrThrow(cClassNameConfig);
    _namespace = configuration->getTextOrThrow(cNamespaceConfig);
    if (!el::i18n::compiler::DisplayTextCompiler::validIdentifier(_className)) {
        throw el::conf::ConfError{
            configuration->valueOrThrow(cClassNameConfig),
            "Invalid Display-Text Class"_el,
            "Expected a non-reserved C++ class identifier."_el};
    }
    for (const auto &part : el::StringList::fromSplit(_namespace, el::CharSet{U':'})) {
        if (!el::i18n::compiler::DisplayTextCompiler::validIdentifier(part)) {
            throw el::conf::ConfError{
                configuration->valueOrThrow(cNamespaceConfig),
                "Invalid Display-Text Namespace"_el,
                "Namespace components must be non-reserved C++ identifiers."_el};
        }
    }
    _checkOnly = values->getFlag(cCheckOption);
}

void DisplayTextsTool::execute() {
    auto &files = frame::app().files();
    auto compiler = el::i18n::compiler::DisplayTextCompiler{_className, _namespace};
    const auto output = compiler.generate(
        files.readConfiguration(_input, false),
        _header.toRelativeOrThrow(_source.parent()).toString(),
        frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp));
    const auto headerChanged = files.updateText(_header, output.header, _checkOnly);
    const auto sourceChanged = files.updateText(_source, output.source, _checkOnly);
    if (_checkOnly && (headerChanged || sourceChanged)) {
        throw el::ApplicationError{
            "Generated Output Is Out of Date"_el,
            "The display-text catalog differs. Run generate-display-texts without --check to update it."_el};
    }
    if (headerChanged || sourceChanged) {
        log()->info("Updated display-text catalog."_el);
    } else {
        log()->info("Display-text catalog is unchanged."_el);
    }
}

}
