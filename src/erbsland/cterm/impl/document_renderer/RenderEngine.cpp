// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderEngine.hpp"

#include "BlockScope.hpp"
#include "Helpers.hpp"
#include "InlineTextBuilder.hpp"
#include "RenderContext.hpp"

#include "../BlockStringBuilder.hpp"

#include "../../../text/impl/CodeLineMarkerData.hpp"
#include "../../../text/impl/CodeSnippetLayout.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/TextNode.hpp"
#include "../../TabOverflowBehavior.hpp"
#include "../../TerminalDocumentStyle.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace erbsland::cterm::impl::document_renderer {

using namespace text::literals;
using namespace text;

RenderEngine::RenderEngine(const TerminalDocumentStyle &style, const int width) noexcept :
    _style{style}, _width{std::max(width, 1)} {
}

auto RenderEngine::build(const TextNode &node) -> std::vector<RenderBlock> {
    _scopes.clear();
    _ancestors.clear();
    _pendingBlock.reset();
    _blocks.clear();
    appendNode(node, RenderContext{});
    flushPendingBlock();
    return std::move(_blocks);
}

void RenderEngine::appendNode(const TextNode &node, const RenderContext &context) {
    struct AncestorScope final {
        std::vector<TextNodeType> &types;
        ~AncestorScope() { types.pop_back(); }
    };
    _ancestors.push_back(node.type());
    const auto ancestorScope = AncestorScope{_ancestors};
    switch (node.type().raw()) {
    case TextNodeType::Document:
    case TextNodeType::Section:
    case TextNodeType::Blockquote:
    case TextNodeType::DefinitionList:
        appendContainer(node, context);
        return;
    case TextNodeType::CodeSnippet:
        appendCodeSnippet(node, context);
        return;
    case TextNodeType::TermList:
    case TextNodeType::FieldList:
        appendTermList(node, context);
        return;
    case TextNodeType::BulletList:
        appendList(node, TextNodeType::BulletListItem, context);
        return;
    case TextNodeType::NumberedList:
        appendList(node, TextNodeType::NumberedListItem, context);
        return;
    case TextNodeType::BulletListItem:
    case TextNodeType::NumberedListItem:
        appendListItemWithoutMarker(node, context);
        return;
    case TextNodeType::Paragraph:
    case TextNodeType::DefinitionTerm:
    case TextNodeType::DefinitionDescription:
    case TextNodeType::TermItem:
    case TextNodeType::TermName:
    case TextNodeType::TermDescription:
    case TextNodeType::FieldItem:
    case TextNodeType::FieldLabel:
    case TextNodeType::FieldContent:
    case TextNodeType::CodeBlock:
    case TextNodeType::CodeLine:
    case TextNodeType::CodeLineMarker:
    case TextNodeType::Unsupported:
    case TextNodeType::Error:
    case TextNodeType::LineBreak:
    case TextNodeType::Text:
    case TextNodeType::Emphasis:
    case TextNodeType::Strong:
    case TextNodeType::Underline:
    case TextNodeType::Span:
    case TextNodeType::Link:
    case TextNodeType::Code:
    case TextNodeType::CodeLineNumber:
    case TextNodeType::CodeLineText:
    case TextNodeType::OptionExecutable:
    case TextNodeType::OptionModule:
    case TextNodeType::OptionName:
    case TextNodeType::OptionShort:
    case TextNodeType::OptionLong:
    case TextNodeType::OptionMeta:
    case TextNodeType::OptionOptional:
    case TextNodeType::OptionDetails:
    case TextNodeType::Separator:
    case TextNodeType::EscapeSequence:
        appendParagraphLikeNode(node, context);
        return;
    case TextNodeType::Heading:
        emitBlock(heading(node, context));
        return;
    case TextNodeType::HorizontalLine:
        emitBlock(horizontalRule(context));
        return;
    case TextNodeType::None:
    case TextNodeType::_count:
        return;
    }
}

void RenderEngine::appendContainer(const TextNode &node, const RenderContext &context) {
    const auto rule = ruleFor(node);
    if (rule.prefix().has_value()) {
        emitBlock(paragraph(*rule.prefix(), TerminalDocumentStyleRule{}, context));
    }
    openScope(
        rule.margins(),
        std::nullopt,
        rule.linePrefix().has_value() ? std::optional<BlockString>{*rule.linePrefix()} : std::nullopt);
    const auto childContext = context.withContainer(rule);
    for (const auto &child : node.children()) {
        if (child->type().renderClass() == TextNodeType::RenderClass::Inline ||
            child->type().renderClass() == TextNodeType::RenderClass::Empty) {
            continue;
        }
        appendNode(*child, childContext);
    }
    closeScope();
    if (rule.suffix().has_value()) {
        emitBlock(paragraph(*rule.suffix(), TerminalDocumentStyleRule{}, context));
    }
}

void RenderEngine::appendCodeSnippet(const TextNode &node, const RenderContext &context) {
    const auto snippetRule = ruleFor(node);
    openScope(snippetRule.margins());
    const auto snippetContext = context.withContainer(snippetRule);
    for (const auto &line : node.children()) {
        if (line->type() == TextNodeType::CodeLine) {
            appendCodeSnippetLine(*line, snippetContext);
        }
    }
    closeScope();
}

void RenderEngine::appendCodeSnippetLine(const TextNode &line, const RenderContext &context) {
    auto number = String{};
    auto source = String{};
    auto markerNodes = std::vector<TextNodePtr>{};
    auto markers = std::vector<text::impl::CodeSnippetLayoutMarker>{};
    for (const auto &child : line.children()) {
        if (child->type() == TextNodeType::CodeLineNumber) {
            number = nodeText(*child);
        } else if (child->type() == TextNodeType::CodeLineText) {
            source = nodeText(*child);
        } else if (child->type() == TextNodeType::CodeLineMarker && child->data() != nullptr) {
            if (const auto markerData = std::dynamic_pointer_cast<const text::impl::CodeLineMarkerData>(child->data());
                markerData != nullptr) {
                markerNodes.push_back(child);
                markers.push_back(text::impl::CodeSnippetLayoutMarker{markerData->range(), nodeText(*child)});
            }
        }
    }

    const auto lineRule = ruleFor(line);
    const auto lineIndents = context.resolvedIndents(lineRule);
    const auto sourceRule = ruleFor(TextNodeType::CodeLineText, std::nullopt, {});
    const auto sourceStyle = context.resolvedTextStyle(_style.baseTextStyle(), sourceRule);
    const auto gutter = makeCodeSnippetGutter(number, !number.isEmpty(), context);
    const auto markerGutter = makeCodeSnippetGutter({}, !number.isEmpty(), context);
    const auto margins = lineIndents.margins().horizontal();
    const auto availableWidth =
        std::max(_width - frameWidth() - margins.extent().toRawValue() - gutter.displayWidth(), 1);
    const auto layout = text::impl::CodeSnippetLayout{source, markers, availableWidth};
    const auto &rows = layout.rows();
    for (auto rowIndex = std::size_t{0}; rowIndex < rows.size(); ++rowIndex) {
        _blockBuilder.clear();
        _blockBuilder.append(rowIndex == 0 ? BlockString{gutter} : BlockString{markerGutter});
        for (const auto &cell : rows[rowIndex].cells()) {
            auto cellStyle = sourceStyle;
            if (cell.isEllipsis) {
                cellStyle = cellStyle.withOverlay(BlockStyle{fg::BrightBlack});
            } else if (cell.marker.has_value() && *cell.marker < markerNodes.size()) {
                cellStyle = context.resolvedTextStyle(_style.baseTextStyle(), ruleFor(*markerNodes[*cell.marker]));
            }
            _blockBuilder.appendStyled(cell.text, cellStyle);
        }
        emitBlock(RenderBlock{BlockKind::Preformatted, _blockBuilder.toString(), lineIndents});

        for (auto markerIndex = std::size_t{0}; markerIndex < markers.size(); ++markerIndex) {
            const auto &marker = markers[markerIndex];
            const auto placement = rows[rowIndex].markerPlacement(marker.range);
            if (!placement.has_value()) {
                continue;
            }
            const auto markerStyle =
                context.resolvedTextStyle(_style.baseTextStyle(), ruleFor(*markerNodes[markerIndex]));
            _blockBuilder.clear();
            _blockBuilder.append(markerGutter);
            for (auto index = 0; index < placement->start; ++index) {
                _blockBuilder.append(Block{Char{U' '}, sourceStyle});
            }
            const auto point = marker.range.isEmpty();
            for (auto index = 0; index < placement->length; ++index) {
                _blockBuilder.append(Block{Char{point ? U'↑' : U'▔'}, markerStyle});
            }
            const auto &label = marker.label;
            const auto annotationWidth =
                static_cast<std::size_t>(placement->start) + static_cast<std::size_t>(placement->length);
            if (layout.isLastMarkerRow(rowIndex, marker.range) && !label.isEmpty() &&
                annotationWidth + label.characterLength().toSizeT() + 3U <= static_cast<std::size_t>(availableWidth)) {
                _blockBuilder.appendStyled(" ("_el, markerStyle);
                _blockBuilder.appendStyled(label, markerStyle);
                _blockBuilder.appendStyled(")"_el, markerStyle);
            }
            emitBlock(RenderBlock{BlockKind::Preformatted, _blockBuilder.toString(), lineIndents});
        }
    }
}

auto RenderEngine::makeCodeSnippetGutter(const String &number, const bool hasNumber, const RenderContext &context)
    -> BlockString {
    if (!hasNumber) {
        return {};
    }
    const auto numberRule = ruleFor(TextNodeType::CodeLineNumber, std::nullopt, {});
    const auto numberStyle = context.resolvedTextStyle(_style.baseTextStyle(), numberRule);
    auto numberBuilder = StringEditor{};
    const auto padding = std::max(cCodeLineNumberWidth - static_cast<int>(number.characterLength().toSizeT()), 0);
    numberBuilder.append(Char{U' '}, unit::CpLength::fromSizeT(static_cast<std::size_t>(padding)));
    numberBuilder.append(number);
    _blockBuilder.clear();
    _blockBuilder.appendStyled(numberBuilder, numberStyle);
    if (numberRule.suffix().has_value()) {
        _blockBuilder.appendWithBaseStyle(*numberRule.suffix(), numberStyle);
    }
    return _blockBuilder.toString();
}

}
