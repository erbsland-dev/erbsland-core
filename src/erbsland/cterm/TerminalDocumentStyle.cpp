// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalDocumentStyle.hpp"

#include "../text/u32/U32StringEditor.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <utility>

namespace erbsland::cterm {

using namespace text;
using namespace text::literals;
using Selector = TerminalDocumentStyleSelector;
using Attributes = BlockAttributes;
using Style = BlockStyle;

TerminalDocumentStyle::TerminalDocumentStyle() : _data{new impl::TerminalDocumentStyleData{}} {
    initializePlainDefaults();
}

void TerminalDocumentStyle::setBaseTextStyle(const Style style) {
    detach();
    _data->baseTextStyle = style;
}

void TerminalDocumentStyle::setBaseBlockLayout(const ParagraphIndents layout) {
    detach();
    _data->baseBlockLayout = layout;
}

auto TerminalDocumentStyle::definition(const TerminalDocumentStyleSelector &selector) const noexcept
    -> std::optional<std::reference_wrapper<const TerminalDocumentStyleRule>> {
    const auto index = findEntry(selector);
    if (index.isNoIndex()) {
        return std::nullopt;
    }
    return _data->definitions.getRefOrThrow(index).rule;
}

auto TerminalDocumentStyle::edit(const TerminalDocumentStyleSelector &selector) -> TerminalDocumentStyleRule & {
    detach();
    if (const auto index = findEntry(selector); !index.isNoIndex()) {
        return _data->definitions.mutableAt(index).rule;
    }
    auto resolved = resolve(selector, selector.requiredStyleTokens());
    _data->definitions.append(
        impl::TerminalDocumentStyleData::Entry{.selector = selector, .rule = std::move(resolved)});
    using EntryList = impl::TerminalDocumentStyleData::EntryList;
    const auto index = EntryList::Index::end(_data->definitions.count() - EntryList::Count::one());
    return _data->definitions.mutableAt(index).rule;
}

void TerminalDocumentStyle::erase(const TerminalDocumentStyleSelector &selector) noexcept {
    detach();
    if (const auto index = findEntry(selector); !index.isNoIndex()) {
        _data->definitions.remove(index);
    }
}

auto TerminalDocumentStyle::resolve(
    const TerminalDocumentStyleSelector &selector,
    const TerminalDocumentStyleSelector::TokenList &contextTokens,
    const std::vector<text::TextNodeType> &ancestors) const -> TerminalDocumentStyleRule {
    const auto tokens = combinedTokens(selector, contextTokens);
    auto result = defaultRuleFor(selector.nodeType(), selector.level());

    auto bestPass = std::numeric_limits<int>::max();
    auto bestLevelRank = -1;
    auto bestTokenCount = -1;
    auto bestLevelValue = std::numeric_limits<int>::min();
    auto hasMatch = false;
    for (const auto &entry : _data->definitions) {
        if (entry.selector.nodeType() != selector.nodeType()) {
            continue;
        }
        if (!tokensMatch(entry.selector.requiredStyleTokens(), tokens)) {
            continue;
        }
        if (entry.selector.ancestorType().has_value() &&
            std::ranges::find(ancestors, *entry.selector.ancestorType()) == ancestors.end()) {
            continue;
        }

        auto levelRank = 0;
        auto levelValue = std::numeric_limits<int>::min();
        if (entry.selector.level().has_value()) {
            if (!selector.level().has_value()) {
                continue;
            }
            if (*entry.selector.level() > *selector.level()) {
                continue;
            }
            levelValue = *entry.selector.level();
            levelRank = *entry.selector.level() == *selector.level() ? 2 : 1;
        }

        auto pass = 6;
        const auto tokenSpecific = !entry.selector.requiredStyleTokens().isEmpty();
        if (entry.selector.ancestorType().has_value()) {
            pass = 0;
        } else if (tokenSpecific && levelRank == 2) {
            pass = 1;
        } else if (tokenSpecific && levelRank == 1) {
            pass = 2;
        } else if (tokenSpecific) {
            pass = 3;
        } else if (levelRank == 2) {
            pass = 4;
        } else if (levelRank == 1) {
            pass = 5;
        }

        const auto tokenCount = static_cast<int>(entry.selector.requiredStyleTokens().count().toSizeT());
        const auto isBetter = !hasMatch || pass < bestPass || (pass == bestPass && levelRank > bestLevelRank) ||
            (pass == bestPass && levelRank == bestLevelRank && tokenCount > bestTokenCount) ||
            (pass == bestPass && levelRank == bestLevelRank && tokenCount == bestTokenCount &&
                levelValue > bestLevelValue);
        if (!isBetter) {
            continue;
        }
        bestPass = pass;
        bestLevelRank = levelRank;
        bestTokenCount = tokenCount;
        bestLevelValue = levelValue;
        result = entry.rule;
        hasMatch = true;
    }
    return result;
}

auto TerminalDocumentStyle::defaultStyle(const Predefined predefined) noexcept -> const TerminalDocumentStyle & {
    switch (predefined) {
    case Predefined::Plain:
        return defaultPlain();
    case Predefined::Simple:
        return defaultSimple();
    case Predefined::Styled:
        return defaultStyled();
    case Predefined::SystemOutput:
        return defaultSystemOutput();
    }
    return defaultPlain();
}

auto TerminalDocumentStyle::defaultPlain() noexcept -> const TerminalDocumentStyle & {
    static const auto style = TerminalDocumentStyle{};
    return style;
}

auto TerminalDocumentStyle::defaultSimple() noexcept -> const TerminalDocumentStyle & {
    static const auto style = createSimpleDefaultStyle();
    return style;
}

auto TerminalDocumentStyle::defaultStyled() noexcept -> const TerminalDocumentStyle & {
    static const auto style = createStyledDefaultStyle();
    return style;
}

auto TerminalDocumentStyle::defaultSystemOutput() noexcept -> const TerminalDocumentStyle & {
    static const auto style = createSystemOutputDefaultStyle();
    return style;
}

void TerminalDocumentStyle::detach() {
    _data.detach();
}

void TerminalDocumentStyle::initializePlainDefaults() {
    auto bold = Attributes{};
    bold.setBold(true);
    auto italic = Attributes{};
    italic.setItalic(true);
    auto underline = Attributes{};
    underline.setUnderline(true);

    edit(Selector::strong()).setTextStyle(Style{fg::Inherited, bold});
    edit(Selector::emphasis()).setTextStyle(Style{fg::Inherited, italic});
    edit(Selector::underline()).setTextStyle(Style{fg::Inherited, underline});
    edit(Selector::link()).setTextStyle(Style{fg::Inherited, underline});
    edit(Selector{TextNodeType::CodeLineNumber}).setSuffix(U" │ "_el);
    edit(Selector::definitionDescription()).setLineIndent(4);
    edit(Selector{TextNodeType::BulletListItem}).setWrappedLineIndent(4);
    edit(Selector{TextNodeType::NumberedListItem}).setWrappedLineIndent(4).setOrderedMarker();
    edit(Selector::bulletListItem(0)).setLiteralMarker(BlockStringEditor{U"•\t"_el});
    edit(Selector::bulletListItem(1)).setLiteralMarker(BlockStringEditor{U"‣\t"_el});
    edit(Selector::bulletListItem(2)).setLiteralMarker(BlockStringEditor{U"⁃\t"_el});
    edit(Selector::bulletListItem(3)).setLiteralMarker(BlockStringEditor{U"◦\t"_el});
}

auto TerminalDocumentStyle::defaultRuleFor(const TextNodeType nodeType, const std::optional<int> &level) const noexcept
    -> TerminalDocumentStyleRule {
    auto rule = TerminalDocumentStyleRule{};
    rule.setTextStyle(Style{});
    rule.setIndents(_data->baseBlockLayout);
    if (nodeType.isListItem() && level.has_value()) {
        rule.setIndents(ParagraphIndents{0, 0, 4, bgeo::BlockMargins{0}});
    }
    return rule;
}

auto TerminalDocumentStyle::findEntry(const TerminalDocumentStyleSelector &selector) noexcept
    -> impl::TerminalDocumentStyleData::EntryList::Index {
    return _data->definitions.findFirstIf([&](const auto &entry) -> auto { return entry.selector == selector; });
}

auto TerminalDocumentStyle::findEntry(const TerminalDocumentStyleSelector &selector) const noexcept
    -> impl::TerminalDocumentStyleData::EntryList::Index {
    return _data->definitions.findFirstIf([&](const auto &entry) -> auto { return entry.selector == selector; });
}

auto TerminalDocumentStyle::createSimpleDefaultStyle() -> TerminalDocumentStyle {
    auto style = TerminalDocumentStyle{};
    style.edit(Selector::paragraph()).setMargins(0, 0, 1, 0);
    style.edit(Selector::heading(1)).setTextStyle(fg::BrightYellow, Attributes::Bold).setMargins(2, 0, 1, 0);
    style.edit(Selector::heading(2)).setTextStyle(fg::BrightCyan, Attributes::Bold).setMargins(0, 0, 1, 0);
    style.edit(Selector::heading(3)).setTextStyle(fg::BrightGreen, Attributes::Bold);
    style.edit(Selector::definitionList()).setMargins(0, 0, 1, 0);
    style.edit(Selector::definitionTerm()).setTextStyle(fg::BrightYellow, Attributes::Bold);
    style.edit(Selector::definitionDescription()).setMargins(0);
    style.edit(Selector::codeBlock()).setMargins(1, 0);
    style.edit(Selector::numberedList(0)).setMargins(0, 0, 1, 0);
    style.edit(Selector::numberedList(1)).setMargins(0);
    style.edit(Selector::bulletList(0)).setMargins(0, 0, 1, 0);
    style.edit(Selector::bulletList(1)).setMargins(0);
    style.edit(Selector::bulletListItem(0)).setIndents(ParagraphIndents{0, 0, 4, bgeo::BlockMargins{0}});
    style.edit(Selector::numberedListItem(0)).setIndents(ParagraphIndents{0, 0, 4, bgeo::BlockMargins{0}});
    style.edit(Selector::horizontalLine()).setMargins(0, 1);
    return style;
}

auto TerminalDocumentStyle::createStyledDefaultStyle() -> TerminalDocumentStyle {
    auto style = TerminalDocumentStyle{};
    style.edit(Selector::document()).setMargins(0, 2);
    style.edit(Selector::paragraph()).setMargins(0, 2, 1, 6);
    style.edit(Selector::heading(1))
        .setMargins(3, 1, 1, 2)
        .setTextStyle(fg::BrightYellow, Attributes::Bold)
        .setPrefix(U"-◆ "_el, Style{fg::Yellow})
        .setSuffix(U" ◆-"_el, Style{fg::Yellow})
        .setLineFill(U'─', fg::BrightBlack);
    style.edit(Selector::heading(2))
        .setMargins(2, 2, 1, 3)
        .setTextStyle(fg::BrightCyan, Attributes::Bold)
        .setPrefix(U"-◆ "_el, Style{fg::Cyan})
        .setSuffix(U" ◆-"_el, Style{fg::Cyan});
    style.edit(Selector::heading(3)).setMargins(1, 2, 1, 6).setTextStyle(fg::BrightGreen, Attributes::Bold);
    style.edit(Selector::definitionTerm()).setTextStyle(fg::BrightYellow, Attributes::Bold).setMargins(0, 2, 0, 6);
    style.edit(Selector::definitionDescription()).setMargins(0, 2, 1, 10);
    style.edit(Selector::codeBlock()).setMargins(0, 2, 1, 8);
    style.edit(Selector::numberedList(0)).setMargins(0, 2, 1, 2);
    style.edit(Selector::numberedList(1)).setMargins(0);
    style.edit(Selector::bulletList(0)).setMargins(0, 2, 1, 3);
    style.edit(Selector::bulletList(1)).setMargins(0);
    style.edit(Selector::bulletListItem(0))
        .setIndents(ParagraphIndents{0, 0, 3, bgeo::BlockMargins{0}})
        .setLiteralMarker(BlockStringEditor{U"•\t"_el}, Style{fg::Yellow});
    style.edit(Selector::bulletListItem(1)).setLiteralMarker(BlockStringEditor{U"⁃\t"_el}, Style{fg::Cyan});
    style.edit(Selector::bulletListItem(2)).setLiteralMarker(BlockStringEditor{U"‣\t"_el}, Style{fg::Green});
    style.edit(Selector::horizontalLine())
        .setLineFill(U'─', fg::Magenta)
        .setMargins(2)
        .setPrefix(U"◆◂"_el, Style{fg::Magenta})
        .setSuffix(U"▸◆"_el, Style{fg::Magenta});
    style.edit(Selector::span({"key"_el}))
        .setPrefix(U"["_el, Style{fg::BrightBlack})
        .setSuffix(U"]"_el, Style{fg::BrightBlack})
        .setTextStyle(Style{fg::BrightWhite});
    return style;
}

auto TerminalDocumentStyle::createSystemOutputDefaultStyle() -> TerminalDocumentStyle {
    auto style = TerminalDocumentStyle{};
    style.setBaseTextStyle(Style::reset());
    style.edit(Selector::document()).setMargins(0, 0, 1, 0);
    style.edit(Selector::paragraph()).setTextStyle(Style{fg::Default});
    style.edit(Selector{TextNodeType::Paragraph, {"option-epilog"_el}}).setMargins(1, 0, 1, 0);
    style.edit(Selector::heading(1)).setTextStyle(fg::Default, Attributes::Bold).setMargins(1, 0, 0, 0);
    style.edit(Selector::heading(2)).setTextStyle(fg::Default, Attributes::Bold).setMargins(1, 0, 0, 0);
    style.edit(Selector{TextNodeType::Heading, std::optional<int>{1}, {"diagnostic-title"_el, "error"_el}})
        .setTextStyle(fg::BrightRed, Attributes::Bold)
        .setMargins(1, 2, 1, 2);
    style
        .edit(
            Selector{
                TextNodeType::Heading,
                std::optional<int>{1},
                {"diagnostic-title"_el, "error"_el},
                TextNodeType::Blockquote})
        .setTextStyle(fg::Default, Attributes::Bold)
        .setMargins(0, 0, 0, 2);
    style.edit(Selector{TextNodeType::Blockquote, {"diagnostic-cause"_el}})
        .setMargins(0, 0, 0, 0)
        .setLinePrefix(U"│ "_el, Style{fg::BrightBlack})
        .setSuffix(U"╰─"_el, Style{fg::BrightBlack});
    style.edit(Selector{TextNodeType::Heading, std::optional<int>{2}, {"diagnostic-cause"_el}})
        .setTextStyle(Style{fg::BrightBlack})
        .setMargins(0)
        .setPrefix(U"╭─ "_el, Style{fg::BrightBlack})
        .setSuffix(":"_el);
    style.edit(Selector{TextNodeType::Heading, std::optional<int>{2}, {"diagnostic-section"_el}}).setSuffix(":"_el);
    style.edit(Selector{TextNodeType::Heading, std::optional<int>{2}, {"diagnostic-full-help"_el}})
        .setMargins(1, 0, 0, 0)
        .setSuffix(":"_el);
    style.edit(Selector{TextNodeType::Paragraph, {"diagnostic-description"_el}}).setMargins(0, 0, 0, 2);
    style.edit(Selector{TextNodeType::Paragraph, {"diagnostic-help"_el}})
        .setTextStyle(Style{fg::Default})
        .setMargins(0, 0, 1, 2);
    style.edit(Selector{TextNodeType::Paragraph, {"diagnostic-source-name"_el}})
        .setTextStyle(Style{fg::BrightGreen})
        .setMargins(0, 0, 0, 2);
    style.edit(Selector{TextNodeType::Paragraph, {"diagnostic-source-location"_el}})
        .setTextStyle(Style{fg::BrightYellow})
        .setMargins(0, 0, 0, 2);
    style.edit(Selector{TextNodeType::Heading, std::optional<int>{2}, {"option-section"_el}}).setSuffix(":"_el);
    style.edit(Selector{TextNodeType::TermList}).setMargins(0);
    style.edit(Selector{TextNodeType::TermList, {"option-details"_el}}).setMargins(0, 0, 0, 4);
    style.edit(Selector{TextNodeType::TermItem}).setMargins(0, 0, 0, 2);
    style.edit(Selector{TextNodeType::TermName, {"option-label"_el}}).setSuffix(":"_el);
    style.edit(Selector{TextNodeType::TermDescription}).setTextStyle(Style{fg::Default});
    style.edit(Selector{TextNodeType::FieldList}).setMargins(0, 0, 0, 2);
    style.edit(Selector::descendantOf(TextNodeType::FieldList, TextNodeType::FieldList)).setMargins(0, 0, 0, 4);
    style.edit(Selector{TextNodeType::FieldItem}).setMargins(0);
    style.edit(Selector{TextNodeType::FieldLabel}).setTextStyle(Style{fg::BrightBlack}).setSuffix(":"_el);
    style.edit(Selector{TextNodeType::FieldContent}).setTextStyle(Style{fg::Default});
    style.edit(Selector{TextNodeType::Separator}).setTextStyle(Style{fg::BrightBlack});
    style.edit(Selector{TextNodeType::EscapeSequence}).setTextStyle(Style{fg::BrightYellow});
    style.edit(Selector{TextNodeType::CodeSnippet}).setMargins(1, 0, 1, 2);
    style.edit(Selector{TextNodeType::CodeLine}).setTextStyle(Style{fg::Default});
    style.edit(Selector{TextNodeType::CodeLineMarker}).setTextStyle(Style{fg::Default});
    style.edit(Selector{TextNodeType::CodeLineNumber}).setTextStyle(Style{fg::BrightBlack});
    style.edit(Selector{TextNodeType::CodeLineText}).setTextStyle(Style{fg::Default});
    style.edit(Selector{TextNodeType::CodeLineMarker, {"error"_el}}).setTextStyle(Style{fg::BrightRed});
    style.edit(Selector{TextNodeType::CodeLineMarker, {"warning"_el}}).setTextStyle(Style{fg::BrightYellow});
    style.edit(Selector{TextNodeType::CodeLineMarker, {"note"_el}}).setTextStyle(Style{fg::BrightCyan});
    style.edit(Selector{TextNodeType::OptionExecutable}).setTextStyle(Style{fg::BrightGreen});
    style.edit(Selector{TextNodeType::OptionModule}).setTextStyle(Style{fg::BrightGreen});
    style.edit(Selector{TextNodeType::OptionShort}).setTextStyle(Style{fg::BrightYellow});
    style.edit(Selector{TextNodeType::OptionLong}).setTextStyle(Style{fg::BrightCyan});
    style.edit(Selector{TextNodeType::OptionMeta})
        .setTextStyle(Style{fg::BrightGreen})
        .setPrefix(U"<"_el, Style{fg::BrightBlack})
        .setSuffix(U">"_el, Style{fg::BrightBlack});
    style.edit(Selector{TextNodeType::OptionMeta, {"positional"_el}}).setTextStyle(Style{fg::BrightMagenta});
    style.edit(Selector{TextNodeType::OptionOptional})
        .setTextStyle(Style{fg::BrightCyan})
        .setPrefix(U"["_el, Style{fg::BrightBlack})
        .setSuffix(U"]"_el, Style{fg::BrightBlack});
    style.edit(Selector{TextNodeType::OptionDetails}).setTextStyle(Style{fg::Default});
    return style;
}

auto TerminalDocumentStyle::combinedTokens(
    const TerminalDocumentStyleSelector &selector, const TerminalDocumentStyleSelector::TokenList &contextTokens)
    -> TerminalDocumentStyleSelector::TokenList {
    auto result = TerminalDocumentStyleSelector::TokenList{};
    result.reserve(selector.requiredStyleTokens().count() + contextTokens.count());
    result.append(selector.requiredStyleTokens());
    result.append(contextTokens);
    TerminalDocumentStyleSelector::normalizeTokens(result);
    return result;
}

auto TerminalDocumentStyle::tokensMatch(
    const TerminalDocumentStyleSelector::TokenList &requiredTokens,
    const TerminalDocumentStyleSelector::TokenList &contextTokens) noexcept -> bool {
    if (requiredTokens.isEmpty()) {
        return true;
    }
    for (const auto &token : requiredTokens) {
        if (!contextTokens.contains(token)) {
            return false;
        }
    }
    return true;
}

}
