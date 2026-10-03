// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDocumentBuilder.hpp"

#include "OptionDisplayModel.hpp"
#include "OptionDisplayRow.hpp"
#include "OptionHelpSearch.hpp"
#include "OptionLiterals.hpp"

#include "../Option.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"
#include "../OptionType.hpp"

#include "../../i18n/DisplayText.hpp"
#include "../../text/CodeSnippetMarker.hpp"
#include "../../text/EscapeFormat.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringList.hpp"
#include "../../text/TextNode.hpp"
#include "../../unit/ColumnIndex.hpp"
#include "../../unit/LineCount.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace erbsland::options::impl {

using namespace text;
using namespace literals;
using NodeType = TextNodeType;
using namespace unit;

auto OptionDocumentBuilder::lineIndexFromArgumentIndex(const ArgumentIndex index) noexcept -> LineIndex {
    return LineIndex::fromSizeT(index.toSizeT());
}

auto OptionDocumentBuilder::elementIndexFromLineIndex(const LineIndex index) noexcept -> ItemIndex {
    return ItemIndex::fromSizeT(index.toSizeT());
}

auto OptionDocumentBuilder::markerLength(const String &text) noexcept -> ColumnCount {
    return ColumnCount::fromSizeT(std::max(text.length().toSizeT(), std::size_t{1U}));
}

auto OptionDocumentBuilder::defaultErrorTitle(const OptionErrorReason reason) const -> String {
    switch (reason) {
    case OptionErrorReason::SyntaxError:
        return i18n::displayText("options.syntax_error_title"_el);
    case OptionErrorReason::UnknownName:
        return i18n::displayText("options.unknown_name_title"_el);
    case OptionErrorReason::UnexpectedValueType:
        return i18n::displayText("options.unexpected_value_type_title"_el);
    case OptionErrorReason::ValidationError:
        return i18n::displayText("options.validation_error_title"_el);
    case OptionErrorReason::NotImplemented:
        return i18n::displayText("options.not_implemented_title"_el);
    case OptionErrorReason::None:
        return i18n::displayText("options.generic_error_message"_el);
    }
    return i18n::displayText("options.generic_error_message"_el);
}

void OptionDocumentBuilder::appendErrorSource(TextDocument &document, const OptionErrorContext &context) const {
    if (context.arguments().isEmpty() && context.argumentIndex().isNoIndex()) {
        return;
    }
    auto heading = document.addHeading(2);
    heading->setStyle("diagnostic-section"_el);
    heading->addText(i18n::displayText("options.error_source_heading"_el));
    if (!context.arguments().isEmpty()) {
        auto paragraph = document.addParagraph();
        paragraph->setStyle("diagnostic-source-name"_el);
        paragraph->addText(i18n::displayText("options.command_line_arguments_heading"_el));
    }
    if (!context.argumentIndex().isNoIndex()) {
        auto paragraph = document.addParagraph();
        paragraph->setStyle("diagnostic-source-location"_el);
        paragraph->addText(i18n::displayText("options.argument_index_label"_el));
        paragraph->addText(": "_el);
        paragraph->addText(String::fromInteger(context.argumentIndex().toSizeT()));
    }
}

void OptionDocumentBuilder::appendCommandLineSnippet(TextDocument &document, const OptionErrorContext &context) const {
    const auto &arguments = context.arguments();
    if (arguments.isEmpty()) {
        return;
    }

    const auto argumentCount = arguments.count();
    const auto argumentLineCount = LineCount::fromSizeT(argumentCount.toSizeT());
    const auto errorLine =
        context.argumentIndex().isNoIndex() ? LineIndex::zero() : lineIndexFromArgumentIndex(context.argumentIndex());
    const auto hasErrorIndex = !context.argumentIndex().isNoIndex() && errorLine < LineIndex::end(argumentLineCount);
    const auto lineCount = std::min(argumentLineCount, cMaximumArgumentLines);
    auto firstLine = LineIndex::zero();
    if (argumentLineCount > cMaximumArgumentLines && hasErrorIndex) {
        const auto preferredContextStart = LineIndex::end(cPreferredLinesBeforeError);
        firstLine = errorLine > preferredContextStart ? errorLine - cPreferredLinesBeforeError : LineIndex::zero();
        if (firstLine + cMaximumArgumentLines > LineIndex::end(argumentLineCount)) {
            firstLine = LineIndex::end(argumentLineCount - cMaximumArgumentLines);
        }
    }

    auto lines = StringList{};
    for (auto index = firstLine; index < firstLine + lineCount; ++index) {
        lines.append(arguments.get(elementIndexFromLineIndex(index)).toEscaped(EscapeFormat::Display));
    }

    auto markers = CodeSnippetMarkerList{};
    if (hasErrorIndex && errorLine >= firstLine && errorLine < firstLine + lineCount) {
        const auto argument = arguments.get(elementIndexFromLineIndex(errorLine));
        const auto escapedArgument = argument.toEscaped(EscapeFormat::Display);
        markers.append(
            CodeSnippetMarker{errorLine, ColumnIndex::zero(), markerLength(escapedArgument), {}, "error"_el});
    }

    auto heading = document.addHeading(2);
    heading->setStyle("diagnostic-section"_el);
    heading->addText(i18n::displayText("options.command_line_arguments_heading"_el));
    document.addCodeSnippet(CodeSnippet{std::move(lines), firstLine, "command-line"_el}, std::move(markers));
}

auto OptionDocumentBuilder::appendContextHelp(TextDocument &document, const OptionErrorContext &context) const -> bool {
    if (const auto &option = context.option(); option != nullptr) {
        appendHeading(document.root(), i18n::displayText("options.option_help_heading"_el));
        appendRowsAsTermList(
            document.root(),
            {OptionDisplayRow{
                .title = OptionDisplayModel::optionTitle(option),
                .description = OptionDisplayModel::optionBrief(option, _options),
                .details = {},
                .option = option}},
            true);
        return true;
    }
    if (const auto &optionSet = context.optionSet();
        optionSet != nullptr && (!optionSet->help().title().isEmpty() || !optionSet->help().description().isEmpty())) {
        appendHeading(document.root(), i18n::displayText("options.option_group_help_heading"_el));
        auto list = document.add(NodeType::TermList);
        auto item = list->add(NodeType::TermItem);
        item->add(NodeType::TermName)
            ->addText(
                optionSet->help().title().isEmpty() ? i18n::displayText("options.options_heading"_el)
                                                    : optionSet->help().title());
        if (!optionSet->help().description().isEmpty()) {
            item->add(NodeType::TermDescription)->addText(optionSet->help().description());
        }
        return true;
    }
    if (const auto &module = context.module(); module != nullptr) {
        appendHeading(document.root(), i18n::displayText("options.module_help_heading"_el));
        auto list = document.add(NodeType::TermList);
        auto item = list->add(NodeType::TermItem);
        item->add(NodeType::TermName)->add(NodeType::OptionModule)->addText(module->name());
        const auto description =
            !module->help().description().isEmpty() ? module->help().description() : module->help().title();
        if (!description.isEmpty()) {
            item->add(NodeType::TermDescription)->addText(description);
        }
        return true;
    }
    if (_options != nullptr && !_options->optionModules().empty()) {
        const auto model = OptionDisplayModel{_options, {}};
        const auto rows = model.moduleRows();
        if (!rows.empty()) {
            appendHeading(document.root(), i18n::displayText("options.modules_heading"_el));
            appendRowsAsTermList(document.root(), rows, false);
            return true;
        }
    }
    return false;
}

void OptionDocumentBuilder::appendSuggestion(
    const TextNodePtr &parent, const String &suggestion, const OptionErrorContext &context) const {
    const auto &option = context.option();
    if (context.reason() == OptionErrorReason::UnexpectedValueType && option != nullptr &&
        option->type() == OptionType::Choice && option->matchingChoiceText(suggestion).has_value()) {
        parent->add(NodeType::OptionValue)->addText(suggestion);
        return;
    }
    if (const auto &options = context.options(); options != nullptr) {
        for (const auto &module : options->optionModules()) {
            if (module != nullptr && module->hasName(suggestion)) {
                parent->add(NodeType::OptionModule)->addText(suggestion);
                return;
            }
        }
    }
    const auto matches = OptionHelpSearch{_options, {}}.exactMatches(suggestion);
    for (const auto &entry : matches) {
        if (entry.option != nullptr) {
            if (Option::isOptionName(suggestion)) {
                appendOptionAlias(parent, suggestion);
            } else {
                parent->add(NodeType::OptionValue)->addText(suggestion);
            }
            return;
        }
    }
    parent->addText(suggestion);
}

void OptionDocumentBuilder::appendFullHelpCommand(TextDocument &document, const OptionErrorContext &context) const {
    if (_options == nullptr) {
        return;
    }
    auto heading = document.addHeading(2);
    heading->setStyle("option-section diagnostic-full-help"_el);
    heading->addText(
        context.module() == nullptr ? i18n::displayText("options.view_full_help_heading"_el)
                                    : i18n::displayText("options.view_full_module_help_heading"_el));
    auto list = document.add(NodeType::TermList);
    auto item = list->add(NodeType::TermItem);
    auto name = item->add(NodeType::TermName);
    appendCommandPrefix(name, context.module());
    name->addText(" "_el);
    appendOptionAlias(name, option_literal::cHelpLongName);
}

}
