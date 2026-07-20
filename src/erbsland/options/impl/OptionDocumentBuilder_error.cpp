// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionDocumentBuilder.hpp"

#include "OptionDisplayModel.hpp"
#include "OptionDisplayRow.hpp"

#include "../Option.hpp"
#include "../OptionModule.hpp"
#include "../Options.hpp"
#include "../OptionSet.hpp"

#include "../../i18n/DisplayTextMap.hpp"
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

auto OptionDocumentBuilder::elementIndexFromLineIndex(const LineIndex index) noexcept -> ElementIndex {
    return ElementIndex::fromSizeT(index.toSizeT());
}

auto OptionDocumentBuilder::markerLength(const String &text) noexcept -> ColumnCount {
    return ColumnCount::fromSizeT(std::max(text.length().toSizeT(), std::size_t{1U}));
}

auto OptionDocumentBuilder::defaultErrorTitle(const OptionErrorReason reason) const -> String {
    switch (reason) {
    case OptionErrorReason::SyntaxError:
        return _displayText->text("options.SyntaxErrorTitle"_el);
    case OptionErrorReason::UnknownName:
        return _displayText->text("options.UnknownNameTitle"_el);
    case OptionErrorReason::UnexpectedValueType:
        return _displayText->text("options.UnexpectedValueTypeTitle"_el);
    case OptionErrorReason::ValidationError:
        return _displayText->text("options.ValidationErrorTitle"_el);
    case OptionErrorReason::NotImplemented:
        return _displayText->text("options.NotImplementedTitle"_el);
    case OptionErrorReason::None:
        return _displayText->text("options.GenericErrorMessage"_el);
    }
    return _displayText->text("options.GenericErrorMessage"_el);
}

void OptionDocumentBuilder::appendErrorSource(TextDocument &document, const OptionErrorContext &context) const {
    if (context.arguments().isEmpty() && context.argumentIndex().isNoIndex()) {
        return;
    }
    auto heading = document.addHeading(2);
    heading->setStyle("diagnostic-section"_el);
    heading->addText(_displayText->text("options.ErrorSourceHeading"_el));
    if (!context.arguments().isEmpty()) {
        auto paragraph = document.addParagraph();
        paragraph->setStyle("diagnostic-source-name"_el);
        paragraph->addText(_displayText->text("options.CommandLineArgumentsHeading"_el));
    }
    if (!context.argumentIndex().isNoIndex()) {
        auto paragraph = document.addParagraph();
        paragraph->setStyle("diagnostic-source-location"_el);
        paragraph->addText(_displayText->text("options.ArgumentIndexLabel"_el));
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
    heading->addText(_displayText->text("options.CommandLineArgumentsHeading"_el));
    document.addCodeSnippet(CodeSnippet{std::move(lines), firstLine, "command-line"_el}, std::move(markers));
}

auto OptionDocumentBuilder::appendContextHelp(TextDocument &document, const OptionErrorContext &context) const -> bool {
    if (const auto &option = context.option(); option != nullptr) {
        appendHeading(document.root(), _displayText->text("options.OptionHelpHeading"_el));
        appendRowsAsTermList(
            document.root(),
            {OptionDisplayRow{
                .title = OptionDisplayModel::optionTitle(option, _displayText),
                .description = OptionDisplayModel::optionDescription(option, _displayText),
                .details = {},
                .option = option}},
            true);
        return true;
    }
    if (const auto &optionSet = context.optionSet();
        optionSet != nullptr && (!optionSet->help().title().isEmpty() || !optionSet->help().description().isEmpty())) {
        appendHeading(document.root(), _displayText->text("options.OptionGroupHelpHeading"_el));
        auto list = document.add(NodeType::TermList);
        auto item = list->add(NodeType::TermItem);
        item->add(NodeType::TermName)
            ->addText(
                optionSet->help().title().isEmpty() ? _displayText->text("options.OptionsHeading"_el)
                                                    : optionSet->help().title());
        if (!optionSet->help().description().isEmpty()) {
            item->add(NodeType::TermDescription)->addText(optionSet->help().description());
        }
        return true;
    }
    if (const auto &module = context.module(); module != nullptr) {
        appendHeading(document.root(), _displayText->text("options.ModuleHelpHeading"_el));
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
        const auto model = OptionDisplayModel{_options, {}, _displayText};
        const auto rows = model.moduleRows();
        if (!rows.empty()) {
            appendHeading(document.root(), _displayText->text("options.ModulesHeading"_el));
            appendRowsAsTermList(document.root(), rows, false);
            return true;
        }
    }
    return false;
}

void OptionDocumentBuilder::appendFullHelpCommand(TextDocument &document, const OptionErrorContext &context) const {
    if (_options == nullptr) {
        return;
    }
    auto heading = document.addHeading(2);
    heading->setStyle("option-section diagnostic-full-help"_el);
    heading->addText(
        context.module() == nullptr ? _displayText->text("options.ViewFullHelpHeading"_el)
                                    : _displayText->text("options.ViewFullModuleHelpHeading"_el));
    auto list = document.add(NodeType::TermList);
    auto item = list->add(NodeType::TermItem);
    auto name = item->add(NodeType::TermName);
    const auto model =
        OptionDisplayModel{_options, context.module() == nullptr ? String{} : context.module()->name(), _displayText};
    name->add(NodeType::OptionExecutable)->addEscapedText(model.executableName(), EscapeFormat::Display);
    if (context.module() != nullptr) {
        name->addText(" "_el);
        name->add(NodeType::OptionModule)->addText(context.module()->name());
    }
    name->addText(" "_el);
    name->add(NodeType::OptionLong)->addText("--help"_el);
}

}
