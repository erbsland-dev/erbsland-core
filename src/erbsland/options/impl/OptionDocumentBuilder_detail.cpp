// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDocumentBuilder.hpp"

#include "HelpName.hpp"
#include "OptionDisplayModel.hpp"
#include "OptionHelpSearch.hpp"
#include "OptionLiterals.hpp"

#include "../Option.hpp"
#include "../OptionChoice.hpp"
#include "../OptionError.hpp"
#include "../OptionFlag.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionType.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;
using NodeType = TextNodeType;

auto OptionDocumentBuilder::detailedHelpDocument(const String &moduleName, const String &helpName) const
    -> TextDocument {
    if (!isValidHelpName(helpName)) {
        auto context = OptionErrorContext{}
                           .setReason(OptionErrorReason::SyntaxError)
                           .setTitle(i18n::displayText("options.malformed_help_name_title"_el))
                           .setDescription(i18n::displayText("options.malformed_help_name_description"_el))
                           .setOptions(_options);
        throw OptionError{std::move(context)};
    }
    const auto search = OptionHelpSearch{_options, moduleName};
    auto matches = search.exactMatches(helpName);
    if (matches.empty()) {
        matches = search.partialMatches(helpName);
        if (!matches.empty()) {
            return helpResultsDocument(matches, helpName, moduleName);
        }
        auto context = OptionErrorContext{}
                           .setReason(OptionErrorReason::UnknownName)
                           .setTitle(i18n::displayText("options.unknown_help_name_title"_el))
                           .setDescription(
                               i18n::displayFormat("options.unknown_help_name_description"_el)
                                   .build(helpName.toEscaped(EscapeFormat::Display)))
                           .setOptions(_options);
        throw OptionError{std::move(context)};
    }
    if (OptionHelpSearch::requiresResults(matches)) {
        return helpResultsDocument(matches, helpName, moduleName);
    }
    const auto &target = matches.front();
    if (target.isModule()) {
        return helpDocument(target.module->name());
    }

    auto document = TextDocument{};
    appendDetailedHeader(document, target);
    appendHeading(document.root(), i18n::displayText("options.usage_label"_el));
    auto usageList = document.add(NodeType::TermList);
    for (const auto &name : target.option->names()) {
        if (Option::isOptionName(name)) {
            appendDetailedUsage(usageList->add(NodeType::TermItem)->add(NodeType::TermName), target, name);
        }
    }
    appendDetailedDescription(document, target);
    const auto details = OptionDisplayModel::optionDetails(target.option);
    if (!details.isEmpty()) {
        document.addParagraph()->add(NodeType::OptionDetails)->addText(details);
    }
    appendDetailedExample(document, target.help());
    if (matches.size() > 1U) {
        appendHeading(document.root(), i18n::displayText("options.see_also_heading"_el));
        auto list = document.add(NodeType::TermList);
        for (auto index = std::size_t{1U}; index < matches.size(); ++index) {
            appendDetailedHelpCommand(list->add(NodeType::TermItem)->add(NodeType::TermName), matches[index]);
        }
    }
    appendDetailedEnding(document, target.help());
    return document;
}

auto OptionDocumentBuilder::detailedBrief(const OptionHelpTarget &target) const -> String {
    if (!target.isModule() && !target.isChoice()) {
        return OptionDisplayModel::optionBrief(target.option, _options);
    }
    return target.help().title();
}

void OptionDocumentBuilder::appendDetailedHeader(TextDocument &document, const OptionHelpTarget &target) const {
    auto title = document.addHeading(1);
    const auto brief = detailedBrief(target);
    if (!brief.isEmpty()) {
        title->addText(brief);
    } else {
        auto key = String{"options.option_details_title"_el};
        if (target.isModule()) {
            key = "options.module_details_title"_el;
        } else if (target.isChoice()) {
            key = "options.choice_details_title"_el;
        }
        title->addText(i18n::displayFormat(key).build(target.commandName()));
    }
    auto identity = document.addParagraph();
    if (target.isModule()) {
        identity->add(NodeType::OptionModule)->addText(target.module->name());
    } else if (target.isChoice()) {
        appendDetailedTitle(identity, target.option, false);
        identity->addText(option_literal::cValueSeparator);
        identity->add(NodeType::OptionValue)->addEscapedText(target.choice->text(), EscapeFormat::Display);
    } else {
        appendDetailedTitle(identity, target.option);
    }
}

void OptionDocumentBuilder::appendDetailedDescription(TextDocument &document, const OptionHelpTarget &target) const {
    auto description = target.help().description();
    if (!target.isModule() && !target.isChoice()) {
        description = OptionDisplayModel::optionDescription(target.option, _options);
    }
    if (!description.isEmpty()) {
        appendHeading(document.root(), i18n::displayText("options.description_heading"_el));
        document.addParagraph()->addText(description);
    }
}

void OptionDocumentBuilder::appendDetailedExample(TextDocument &document, const OptionHelp &help) const {
    if (!help.example().isEmpty()) {
        appendHeading(document.root(), i18n::displayText("options.example_heading"_el));
        document.addParagraph()->addText(help.example());
    }
}

void OptionDocumentBuilder::appendDetailedEnding(TextDocument &document, const OptionHelp &help) const {
    if (!help.epilog().isEmpty()) {
        document.addParagraph()->setStyle("option-epilog"_el).addText(help.epilog());
    }
}

auto OptionDocumentBuilder::helpResultsDocument(
    const std::vector<OptionHelpTarget> &matches, const String &helpName, const String &moduleName) const
    -> TextDocument {
    constexpr auto maximumResults = std::size_t{10U};
    auto displayed = std::vector<OptionHelpTarget>{};
    for (auto index = std::size_t{0}; index < std::min(matches.size(), maximumResults); ++index) {
        displayed.emplace_back(matches[index]);
    }
    auto document = TextDocument{};
    document.addHeading(1)->addText(
        i18n::displayFormat("options.help_results_title"_el).build(helpName.toEscaped(EscapeFormat::Display)));
    auto modules = std::vector<OptionHelpTarget>{};
    for (const auto &target : displayed) {
        if (target.isModule()) {
            modules.emplace_back(target);
        }
    }
    appendHelpResultSection(document.root(), i18n::displayText("options.modules_heading"_el), modules);
    const auto scopes = helpResultScopes(moduleName);
    for (const auto &scope : scopes) {
        auto options = std::vector<OptionHelpTarget>{};
        for (const auto &target : displayed) {
            if (!target.isModule() && !target.isChoice() && target.module == scope) {
                options.emplace_back(target);
            }
        }
        if (options.empty()) {
            continue;
        }
        auto heading = i18n::displayText("options.options_heading"_el);
        if (scope != nullptr) {
            heading = i18n::displayFormat("options.options_for_module_heading"_el).build(scope->name());
        }
        appendHelpResultSection(document.root(), heading, options);
    }
    for (const auto &scope : scopes) {
        auto owners = std::vector<OptionPtr>{};
        for (const auto &target : displayed) {
            if (target.isChoice() && target.module == scope &&
                std::ranges::find(owners, target.option) == owners.end()) {
                owners.emplace_back(target.option);
            }
        }
        for (const auto &owner : owners) {
            auto choices = std::vector<OptionHelpTarget>{};
            for (const auto &target : displayed) {
                if (target.isChoice() && target.module == scope && target.option == owner) {
                    choices.emplace_back(target);
                }
            }
            const auto ownerName = OptionHelpTarget{scope, owner, {}, {}}.commandName();
            auto heading = String{};
            if (scope != nullptr) {
                heading =
                    i18n::displayFormat("options.choices_for_module_option_heading"_el).build(scope->name(), ownerName);
            } else {
                heading = i18n::displayFormat("options.choices_for_option_heading"_el).build(ownerName);
            }
            appendHelpResultSection(document.root(), heading, choices);
        }
    }
    if (matches.size() > maximumResults) {
        document.addParagraph()->addText(
            i18n::displayFormat("options.help_results_omitted"_el).build(matches.size() - maximumResults));
    }
    return document;
}

auto OptionDocumentBuilder::helpResultScopes(const String &moduleName) const -> std::vector<OptionModulePtr> {
    auto scopes = std::vector<OptionModulePtr>{};
    const auto selected = OptionDisplayModel{_options, moduleName}.module();
    if (selected != nullptr) {
        scopes.emplace_back(selected);
    }
    scopes.emplace_back(nullptr);
    if (_options != nullptr) {
        for (const auto &module : _options->optionModules()) {
            if (module != nullptr && std::ranges::find(scopes, module) == scopes.end()) {
                scopes.emplace_back(module);
            }
        }
    }
    return scopes;
}

void OptionDocumentBuilder::appendHelpResultSection(
    const TextNodePtr &parent, const String &title, const std::vector<OptionHelpTarget> &matches) const {
    if (matches.empty()) {
        return;
    }
    appendHeading(parent, title);
    auto list = parent->add(NodeType::TermList);
    for (const auto &target : matches) {
        auto item = list->add(NodeType::TermItem);
        auto name = item->add(NodeType::TermName);
        if (target.isModule()) {
            name->add(NodeType::OptionModule)->addText(target.module->name());
        } else if (target.isChoice()) {
            name->add(NodeType::OptionValue)->addEscapedText(target.choice->text(), EscapeFormat::Display);
        } else {
            appendOptionTermName(name, target.option);
        }
        const auto brief = detailedBrief(target);
        if (!brief.isEmpty()) {
            item->add(NodeType::TermDescription)->addText(brief);
        }
    }
}

void OptionDocumentBuilder::appendDetailedTitle(
    const TextNodePtr &parent, const OptionPtr &option, const bool withPlaceholder) const {
    auto first = true;
    for (const auto &name : option->names()) {
        if (!Option::isOptionName(name)) {
            continue;
        }
        if (!first) {
            parent->addText(", "_el);
        }
        appendOptionAlias(parent, name);
        first = false;
    }
    const auto valueName = OptionDisplayModel::optionValueName(option);
    if (withPlaceholder && !valueName.isEmpty()) {
        parent->addText(" "_el);
        appendPlaceholder(parent, NodeType::OptionMeta, valueName, "value"_el);
    }
}

void OptionDocumentBuilder::appendDetailedHelpCommand(const TextNodePtr &parent, const OptionHelpTarget &entry) const {
    appendCommandPrefix(parent, entry.module);
    parent->addText(" "_el);
    appendOptionAlias(parent, option_literal::cHelpLongName);
    if (!entry.isModule()) {
        parent->addText(option_literal::cValueSeparator);
        parent->add(NodeType::OptionValue)->addText(entry.commandName());
    }
}

void OptionDocumentBuilder::appendDetailedUsage(
    const TextNodePtr &parent, const OptionHelpTarget &entry, const String &name) const {
    appendCommandPrefix(parent, entry.module, true);
    parent->addText(" "_el);
    appendOptionAlias(parent, name);
    if (entry.isChoice()) {
        parent->addText(option_literal::cValueSeparator);
        parent->add(NodeType::OptionValue)->addEscapedText(entry.choice->text(), EscapeFormat::Display);
        return;
    }
    if (entry.option->type() == OptionType::Flag) {
        return;
    }
    auto valueParent = parent;
    if (entry.option->flags().isSet(OptionFlag::AcceptAsFlag)) {
        valueParent = parent->add(NodeType::OptionOptional);
        valueParent->setStyle("optional"_el);
    }
    valueParent->addText(option_literal::cValueSeparator);
    appendPlaceholder(valueParent, NodeType::OptionMeta, OptionDisplayModel::optionValueName(entry.option), "value"_el);
}

}
