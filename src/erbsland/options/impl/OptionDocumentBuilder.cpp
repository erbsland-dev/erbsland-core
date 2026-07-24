// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDocumentBuilder.hpp"

#include "OptionDisplayGroup.hpp"
#include "OptionDisplayModel.hpp"
#include "OptionDisplayRow.hpp"

#include "../Option.hpp"
#include "../OptionModule.hpp"
#include "../OptionParserFlag.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../err/DiagnosticHelper.hpp"
#include "../../err/ErrorDocumentBuilder.hpp"
#include "../../i18n/DisplayTextMap.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;
using NodeType = TextNodeType;

OptionDocumentBuilder::OptionDocumentBuilder(OptionsPtr options, i18n::DisplayTextMapConstPtr displayText) :
    _options{std::move(options)},
    _displayText{displayText != nullptr ? std::move(displayText) : i18n::DisplayTextMap::defaultMap()} {
}

auto OptionDocumentBuilder::helpDocument(const String &moduleName) const -> TextDocument {
    const auto model = OptionDisplayModel{_options, moduleName, _displayText};
    auto document = TextDocument{};
    if (const auto help = model.selectedHelp(); help != nullptr && !help->description().isEmpty()) {
        document.addParagraph()->setStyle("option-summary"_el).addText(help->description());
    }

    appendUsage(document.root(), model);

    if (_options != nullptr && model.module() == nullptr && !_options->optionModules().empty()) {
        appendHeading(document.root(), _displayText->text("options.ModulesHeading"_el));
        appendRowsAsTermList(document.root(), model.moduleRows(), false);
    }
    appendOptionGroups(document.root(), model.optionGroups());

    if (const auto help = model.selectedHelp(); help != nullptr && !help->epilog().isEmpty()) {
        document.addParagraph()->setStyle("option-epilog"_el).addText(help->epilog());
    }
    return document;
}

void OptionDocumentBuilder::appendUsage(const TextNodePtr &parent, const OptionDisplayModel &model) const {
    appendHeading(parent, _displayText->text("options.UsageLabel"_el));
    auto usageList = parent->add(NodeType::TermList);
    auto usageItem = usageList->add(NodeType::TermItem);
    auto usageName = usageItem->add(NodeType::TermName);
    usageName->add(NodeType::OptionExecutable)->addEscapedText(model.executableName(), EscapeFormat::Display);
    if (model.module() != nullptr) {
        usageName->addText(" "_el);
        usageName->add(NodeType::OptionModule)->addText(model.module()->name());
    } else if (model.options() != nullptr && !model.options()->optionModules().empty()) {
        usageName->addText(" "_el);
        appendPlaceholder(
            usageName, NodeType::OptionMeta, _displayText->text("options.ModulePlaceholder"_el), "value"_el);
    }
    usageName->addText(" "_el);
    appendPlaceholder(
        usageName, NodeType::OptionOptional, _displayText->text("options.OptionsPlaceholder"_el), "optional"_el);
    for (const auto &option : model.usageOptions()) {
        usageName->addText(" "_el);
        appendUsageOption(usageName, option);
    }
    for (const auto &option : model.usagePositionalOptions()) {
        const auto valueName = OptionDisplayModel::positionalValueName(option, _displayText);
        if (!valueName.isEmpty()) {
            usageName->addText(" "_el);
            appendPlaceholder(usageName, NodeType::OptionMeta, valueName, "positional"_el);
        }
    }
}

auto OptionDocumentBuilder::versionDocument(const String &) const -> TextDocument {
    auto document = TextDocument{};
    auto list = TextNodePtr{};
    const auto addItem = [&document, &list](String name, const String &value) -> void {
        if (value.isEmpty()) {
            return;
        }
        if (list == nullptr) {
            list = document.add(NodeType::TermList);
        }
        auto item = list->add(NodeType::TermItem);
        auto termName = item->add(NodeType::TermName);
        termName->setStyle("option-label"_el);
        termName->addText(std::move(name));
        item->add(NodeType::TermDescription)->addText(value);
    };
    if (_options == nullptr) {
        addItem(_displayText->text("options.VersionLabel"_el), "0.0.0"_el);
        return document;
    }
    const auto &applicationInfo = _options->applicationInfo();
    if (applicationInfo.applicationName().isEmpty()) {
        addItem(_displayText->text("options.VersionLabel"_el), applicationInfo.applicationVersion().toString());
    } else {
        auto paragraph = document.addParagraph();
        paragraph->add(NodeType::OptionExecutable)->addText(applicationInfo.applicationName());
        paragraph->addText(" "_el);
        paragraph->addText(applicationInfo.applicationVersion().toString());
    }
    addItem(_displayText->text("options.AuthorLabel"_el), applicationInfo.authorName());
    if (!applicationInfo.copyrightLine().isEmpty()) {
        document.addParagraph()->addText(applicationInfo.copyrightLine());
    }
    addItem(_displayText->text("options.LicenseLabel"_el), applicationInfo.licenseText());
    return document;
}

auto OptionDocumentBuilder::errorDocument(const OptionErrorContext &errorContext) const -> TextDocument {
    auto builder = err::ErrorDocumentBuilder{
        errorContext.title().isEmpty() ? defaultErrorTitle(errorContext.reason()) : errorContext.title(),
        errorContext.description(),
        _displayText};
    auto document = builder.takeDocument();
    appendErrorSource(document, errorContext);
    appendCommandLineSnippet(document, errorContext);
    if (_options != nullptr) {
        const auto moduleName = errorContext.module() == nullptr ? String{} : errorContext.module()->name();
        appendUsage(document.root(), OptionDisplayModel{_options, moduleName, _displayText});
    }
    if (appendContextHelp(document, errorContext)) {
        document.addLineBreak();
    }
    appendFullHelpCommand(document, errorContext);
    return document;
}

void OptionDocumentBuilder::appendHeading(const TextNodePtr &parent, String title) const {
    auto heading = parent->addHeading(2);
    heading->setStyle("option-section"_el);
    heading->addText(std::move(title));
}

void OptionDocumentBuilder::appendPlaceholder(
    const TextNodePtr &parent, const TextNodeType type, String placeholder, String style) const {
    auto node = parent->add(type);
    node->setStyle(std::move(style));
    node->addText(std::move(placeholder));
}

void OptionDocumentBuilder::appendOptionName(const TextNodePtr &termName, const OptionPtr &option) const {
    for (const auto &name : option->names()) {
        if (!Option::isOptionName(name)) {
            continue;
        }
        auto optionName = termName->add(NodeType::OptionName);
        if (Option::isShortName(name)) {
            optionName->add(NodeType::OptionShort)->addText(name);
        } else {
            optionName->add(NodeType::OptionLong)->addText(name);
        }
    }
}

void OptionDocumentBuilder::appendOptionValuePlaceholder(const TextNodePtr &termName, const OptionPtr &option) const {
    if (option != nullptr && option->type() == OptionType::Flag && _options != nullptr &&
        !_options->parserFlags().isSet(OptionParserFlag::DisableBooleanValues)) {
        termName->addText("[="_el);
        appendPlaceholder(
            termName, NodeType::OptionMeta, _displayText->text("options.BooleanPlaceholder"_el), "value"_el);
        termName->addText("]"_el);
        return;
    }
    const auto placeholder = OptionDisplayModel::optionValueName(option, _displayText);
    if (!placeholder.isEmpty()) {
        termName->addText(" "_el);
        appendPlaceholder(termName, NodeType::OptionMeta, placeholder, "value"_el);
    }
}

void OptionDocumentBuilder::appendUsageOption(const TextNodePtr &termName, const OptionPtr &option) const {
    auto wrapper = termName->add(NodeType::OptionOptional);
    wrapper->setStyle("optional"_el);
    auto selectedName = String{};
    for (const auto &name : option->names()) {
        if (Option::isLongName(name)) {
            selectedName = name;
            break;
        }
        if (selectedName.isEmpty() && Option::isShortName(name)) {
            selectedName = name;
        }
    }
    if (!selectedName.isEmpty()) {
        wrapper->add(Option::isShortName(selectedName) ? NodeType::OptionShort : NodeType::OptionLong)
            ->addText(selectedName);
    }
    appendOptionValuePlaceholder(wrapper, option);
}

void OptionDocumentBuilder::appendOptionTermName(const TextNodePtr &termName, const OptionPtr &option) const {
    if (option->isRegularOption()) {
        appendOptionName(termName, option);
        appendOptionValuePlaceholder(termName, option);
        return;
    }
    const auto valueName = OptionDisplayModel::positionalValueName(option, _displayText);
    if (!valueName.isEmpty()) {
        appendPlaceholder(termName, NodeType::OptionMeta, valueName, "positional"_el);
    }
}

void OptionDocumentBuilder::appendOptionDescription(const TextNodePtr &item, const OptionDisplayRow &row) const {
    const auto details =
        !row.option.expired() ? OptionDisplayModel::optionDetails(row.option.lock(), _displayText) : String{};
    if (row.description.isEmpty() && details.isEmpty()) {
        return;
    }
    auto description = item->add(NodeType::TermDescription);
    if (!row.description.isEmpty()) {
        description->addText(row.description);
    }
    if (!details.isEmpty()) {
        if (!row.description.isEmpty()) {
            description->addText(" "_el);
        }
        description->add(NodeType::OptionDetails)->addText(details);
    }
}

void OptionDocumentBuilder::appendOptionGroups(
    const TextNodePtr &parent, const std::vector<OptionDisplayGroup> &groups) const {
    for (const auto &group : groups) {
        appendHeading(parent, group.title);
        appendRowsAsTermList(parent, group.rows, true);
    }
}

void OptionDocumentBuilder::appendRowsAsTermList(
    const TextNodePtr &parent,
    const std::vector<OptionDisplayRow> &rows,
    const bool optionRows,
    const bool nestedDetails) const {
    if (rows.empty()) {
        return;
    }
    auto list = parent->add(NodeType::TermList);
    if (nestedDetails) {
        list->setStyle("option-details"_el);
    }
    for (const auto &row : rows) {
        auto item = list->add(NodeType::TermItem);
        auto name = item->add(NodeType::TermName);
        if (optionRows && !row.option.expired()) {
            appendOptionTermName(name, row.option.lock());
        } else {
            name->add(NodeType::OptionModule)->addText(row.title);
        }
        appendOptionDescription(item, row);
        if (!row.details.empty()) {
            appendRowsAsTermList(item, row.details, false, true);
        }
    }
}

}
