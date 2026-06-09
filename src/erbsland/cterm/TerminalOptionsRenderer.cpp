// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalOptionsRenderer.hpp"

#include "Terminal.hpp"

#include "../err/StreamError.hpp"
#include "../options/Option.hpp"
#include "../options/OptionErrorContext.hpp"
#include "../options/OptionModule.hpp"
#include "../options/Options.hpp"
#include "../options/OptionSet.hpp"
#include "../text/Literals.hpp"
#include "../text/StringBuilder.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace erbsland::cterm {

using namespace text::literals;

TerminalOptionsRenderer::TerminalOptionsRenderer(TerminalPtr terminal) :
    TerminalOptionsRenderer{std::move(terminal), TerminalOptionsTheme::defaultTheme()} {
}

TerminalOptionsRenderer::TerminalOptionsRenderer(TerminalPtr terminal, TerminalOptionsTheme theme) :
    TerminalOptionsRenderer{std::move(terminal), std::move(theme), options::OptionDisplayText::defaultText()} {
}

TerminalOptionsRenderer::TerminalOptionsRenderer(
    TerminalPtr terminal, TerminalOptionsTheme theme, options::OptionDisplayText displayText) :
    options::OptionRendererBase{std::move(displayText)}, _terminal{std::move(terminal)}, _theme{std::move(theme)} {
}

auto TerminalOptionsRenderer::create(TerminalPtr terminal) -> TerminalOptionsRendererPtr {
    return std::make_shared<TerminalOptionsRenderer>(std::move(terminal));
}

auto TerminalOptionsRenderer::create(TerminalPtr terminal, TerminalOptionsTheme theme) -> TerminalOptionsRendererPtr {
    return std::make_shared<TerminalOptionsRenderer>(std::move(terminal), std::move(theme));
}

auto TerminalOptionsRenderer::create(
    TerminalPtr terminal, TerminalOptionsTheme theme, options::OptionDisplayText displayText)
    -> TerminalOptionsRendererPtr {
    return std::make_shared<TerminalOptionsRenderer>(std::move(terminal), std::move(theme), std::move(displayText));
}

void TerminalOptionsRenderer::displayHelp(const options::OptionsPtr &options, text::StringView moduleName) {
    const auto terminal = requireTerminal();
    terminal->testScreenSize();
    const auto width = terminalWidth(*terminal);
    const auto model = options::impl::OptionDisplayModel{options, moduleName, displayText()};

    terminal->printLine(styledText(model.helpTitleText(), _theme.heading()));
    if (const auto title = model.titleText(); !title.isEmpty() && title != model.displayName()) {
        terminal->printLine(styledText(title, _theme.heading()));
    }
    if (const auto help = model.selectedHelp(); help != nullptr) {
        writeHelpText(*terminal, *help, width);
    }
    writeUsage(*terminal, model);

    if (options != nullptr && model.module() == nullptr && !options->optionModules().empty()) {
        writeSection(*terminal, displayText().modulesHeading(), model.moduleRows(), width);
    }
    writeSection(*terminal, displayText().optionsHeading(), model.optionRows(), width);

    if (const auto help = model.selectedHelp(); help != nullptr && !help->epilog().isEmpty()) {
        terminal->writeLineBreak();
        terminal->printParagraph(styledText(help->epilog(), _theme.description()), bodyParagraphOptions());
    }
    terminal->setStyle(BlockStyle::reset());
    terminal->flush();
}

void TerminalOptionsRenderer::displayVersion(const options::OptionsPtr &options, text::StringView) {
    const auto terminal = requireTerminal();
    if (options == nullptr) {
        terminal->printLine(styledLabel(displayText().versionLabel(), "0.0.0"_el, _theme.defaultValue()));
        terminal->flush();
        return;
    }

    const auto &applicationInfo = options->applicationInfo();
    if (applicationInfo.applicationName().isEmpty()) {
        terminal->printLine(styledLabel(
            displayText().versionLabel(), applicationInfo.applicationVersion().toString(), _theme.defaultValue()));
    } else {
        auto line = BlockString{};
        line.append(_theme.programName(), applicationInfo.applicationName());
        line.append(_theme.description(), " "_el, applicationInfo.applicationVersion().toString());
        terminal->printLine(line);
    }
    if (!applicationInfo.authorName().isEmpty()) {
        terminal->printLine(
            styledLabel(displayText().authorLabel(), applicationInfo.authorName(), _theme.description()));
    }
    if (!applicationInfo.copyrightLine().isEmpty()) {
        terminal->printLine(styledText(applicationInfo.copyrightLine(), _theme.description()));
    }
    if (!applicationInfo.licenseText().isEmpty()) {
        terminal->printLine(
            styledLabel(displayText().licenseLabel(), applicationInfo.licenseText(), _theme.description()));
    }
    terminal->setStyle(BlockStyle::reset());
    terminal->flush();
}

void TerminalOptionsRenderer::displayError(
    const options::OptionsPtr &, const options::OptionErrorContext &errorContext) {
    const auto terminal = requireTerminal();
    if (errorContext.description().isEmpty()) {
        auto line = BlockString{};
        line.append(_theme.error(), displayText().errorLabel());
        line.append(_theme.description(), displayText().genericErrorMessage());
        terminal->printLine(line);
    } else {
        auto line = BlockString{};
        line.append(_theme.error(), displayText().errorLabel());
        line.append(_theme.description(), errorContext.description());
        terminal->printLine(line);
    }
    if (!errorContext.moduleName().isEmpty()) {
        terminal->printLine(styledLabel(displayText().moduleLabel(), errorContext.moduleName(), _theme.programName()));
    }
    if (const auto option = errorContext.option(); option != nullptr) {
        terminal->printLine(styledLabel(
            displayText().optionLabel(),
            options::impl::OptionDisplayModel::optionTitle(option, displayText()),
            _theme.optionName()));
    }
    if (!errorContext.argumentIndex().isNoIndex()) {
        auto builder = text::StringBuilder{};
        builder.appendInteger(errorContext.argumentIndex().toRawValue());
        terminal->printLine(styledLabel(displayText().argumentIndexLabel(), builder.toString(), _theme.defaultValue()));
    }
    terminal->setStyle(BlockStyle::reset());
    terminal->flush();
}

auto TerminalOptionsRenderer::requireTerminal() const -> TerminalPtr {
    if (_terminal == nullptr) {
        throw err::StreamError{"The terminal options renderer has no terminal."};
    }
    return _terminal;
}

auto TerminalOptionsRenderer::terminalWidth(Terminal &terminal) const noexcept -> int {
    return std::max(terminal.size().width().toRawValue(), 20);
}

auto TerminalOptionsRenderer::descriptionColumn(
    const std::vector<options::impl::OptionDisplayRow> &rows, const int width) const -> int {
    if (width < 40) {
        return 6;
    }
    const auto widestTitle = rowTitleWidth(rows);
    const auto upperBound = std::max(12, std::min(36, width - 10));
    const auto preferred = std::max(26, widestTitle + 4);
    return std::min(preferred, upperBound);
}

auto TerminalOptionsRenderer::rowTitleWidth(const std::vector<options::impl::OptionDisplayRow> &rows) const noexcept
    -> int {
    auto result = 0;
    for (const auto &row : rows) {
        result = std::max(result, BlockString{row.title}.displayWidth());
    }
    return result;
}

auto TerminalOptionsRenderer::bodyParagraphOptions(const int lineIndent, const int wrappedLineIndent) const
    -> ParagraphOptions {
    auto options = ParagraphOptions{};
    options.setLineIndent(lineIndent);
    options.setFirstLineIndent(lineIndent);
    options.setWrappedLineIndent(wrappedLineIndent <= 0 ? lineIndent : wrappedLineIndent);
    options.setOnError(ParagraphOnError::PlainOutput);
    return options;
}

auto TerminalOptionsRenderer::optionParagraphOptions(const int descriptionColumnValue) const -> ParagraphOptions {
    auto options = bodyParagraphOptions(2, descriptionColumnValue);
    options.setTabStops({ParagraphOptions::cTabWrappedLineIndent});
    options.setTabOverflowBehavior(TabOverflowBehavior::LineBreak);
    return options;
}

auto TerminalOptionsRenderer::styledText(text::StringView text, const BlockStyle style) const -> BlockString {
    auto result = BlockString{};
    result.append(style, text);
    return result;
}

auto TerminalOptionsRenderer::styledUsage(const options::impl::OptionDisplayModel &model) const -> BlockString {
    auto result = BlockString{};
    result.append(_theme.usage(), displayText().usageLabel());
    result.append(_theme.programName(), model.executableName());
    if (model.module() != nullptr) {
        result.append(_theme.description(), " "_el);
        result.append(_theme.programName(), model.module()->name());
    } else if (model.options() != nullptr && !model.options()->optionModules().empty()) {
        result.append(_theme.description(), " "_el);
        result.append(_theme.valueName(), displayText().modulePlaceholder());
    }
    result.append(_theme.description(), " "_el);
    result.append(_theme.valueName(), displayText().optionsPlaceholder());

    if (model.module() != nullptr || model.options() == nullptr || model.options()->optionModules().empty()) {
        for (const auto &optionSet : model.visibleOptionSets()) {
            for (const auto &option : optionSet->options()) {
                if (option == nullptr || !option->isPositionalArgument() || option->isDisabled() ||
                    !options::impl::OptionDisplayModel::visibleHelp(option->help()) || option->names().empty()) {
                    continue;
                }
                result.append(_theme.description(), " "_el);
                result.append(
                    _theme.valueName(),
                    displayText().placeholderPrefix(),
                    option->names().front(),
                    displayText().placeholderSuffix());
            }
        }
    }
    return result;
}

auto TerminalOptionsRenderer::styledRow(const options::impl::OptionDisplayRow &row) const -> BlockString {
    auto result = BlockString{};
    result.append(_theme.optionName(), row.title);
    if (!row.description.isEmpty()) {
        result.append(_theme.description(), "\t"_el);
        result.append(_theme.description(), row.description);
    }
    return result;
}

auto TerminalOptionsRenderer::styledLabel(
    text::StringView label, text::StringView value, const BlockStyle valueStyle) const -> BlockString {
    auto result = BlockString{};
    result.append(_theme.label(), label);
    result.append(valueStyle, value);
    return result;
}

void TerminalOptionsRenderer::writeHelpText(Terminal &terminal, const options::OptionHelp &help, int) {
    if (!help.description().isEmpty()) {
        terminal.printParagraph(styledText(help.description(), _theme.description()), bodyParagraphOptions());
    }
}

void TerminalOptionsRenderer::writeUsage(Terminal &terminal, const options::impl::OptionDisplayModel &model) {
    terminal.printParagraph(styledUsage(model), bodyParagraphOptions());
}

void TerminalOptionsRenderer::writeSection(
    Terminal &terminal,
    text::StringView title,
    const std::vector<options::impl::OptionDisplayRow> &rows,
    const int width) {
    if (rows.empty()) {
        return;
    }
    terminal.writeLineBreak();
    terminal.printLine(styledText(title, _theme.heading()));
    writeRows(terminal, rows, descriptionColumn(rows, width));
}

void TerminalOptionsRenderer::writeRows(
    Terminal &terminal, const std::vector<options::impl::OptionDisplayRow> &rows, const int descriptionColumnValue) {
    for (const auto &row : rows) {
        writeRow(terminal, row, descriptionColumnValue);
    }
}

void TerminalOptionsRenderer::writeRow(
    Terminal &terminal, const options::impl::OptionDisplayRow &row, const int descriptionColumnValue) {
    terminal.printParagraph(styledRow(row), optionParagraphOptions(descriptionColumnValue));
    if (!row.details.empty()) {
        for (const auto &detail : row.details) {
            terminal.printParagraph(styledRow(detail), optionParagraphOptions(descriptionColumnValue + 2));
        }
    }
}

}
