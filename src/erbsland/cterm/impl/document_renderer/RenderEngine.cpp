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
#include "../../../text/StringBuilder.hpp"
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

RenderEngine::RenderEngine(const TerminalDocumentStyle &style, const int width) noexcept :
    _style{style}, _width{std::max(width, 1)} {
}

auto RenderEngine::build(const text::TextNode &node) -> std::vector<RenderBlock> {
    _scopes.clear();
    _ancestors.clear();
    _pendingBlock.reset();
    _blocks.clear();
    appendNode(node, RenderContext{});
    flushPendingBlock();
    return std::move(_blocks);
}

void RenderEngine::appendNode(const text::TextNode &node, const RenderContext &context) {
    struct AncestorScope final {
        std::vector<text::TextNodeType> &types;
        ~AncestorScope() { types.pop_back(); }
    };
    _ancestors.push_back(node.type());
    const auto ancestorScope = AncestorScope{_ancestors};
    switch (node.type().raw()) {
    case text::TextNodeType::Document:
    case text::TextNodeType::Section:
    case text::TextNodeType::Blockquote:
    case text::TextNodeType::DefinitionList:
        appendContainer(node, context);
        return;
    case text::TextNodeType::CodeSnippet:
        appendCodeSnippet(node, context);
        return;
    case text::TextNodeType::TermList:
    case text::TextNodeType::FieldList:
        appendTermList(node, context);
        return;
    case text::TextNodeType::BulletList:
        appendList(node, text::TextNodeType::BulletListItem, context);
        return;
    case text::TextNodeType::NumberedList:
        appendList(node, text::TextNodeType::NumberedListItem, context);
        return;
    case text::TextNodeType::BulletListItem:
    case text::TextNodeType::NumberedListItem:
        appendListItemWithoutMarker(node, context);
        return;
    case text::TextNodeType::Paragraph:
    case text::TextNodeType::DefinitionTerm:
    case text::TextNodeType::DefinitionDescription:
    case text::TextNodeType::TermItem:
    case text::TextNodeType::TermName:
    case text::TextNodeType::TermDescription:
    case text::TextNodeType::FieldItem:
    case text::TextNodeType::FieldLabel:
    case text::TextNodeType::FieldContent:
    case text::TextNodeType::CodeBlock:
    case text::TextNodeType::CodeLine:
    case text::TextNodeType::CodeLineMarker:
    case text::TextNodeType::Unsupported:
    case text::TextNodeType::Error:
    case text::TextNodeType::LineBreak:
    case text::TextNodeType::Text:
    case text::TextNodeType::Emphasis:
    case text::TextNodeType::Strong:
    case text::TextNodeType::Underline:
    case text::TextNodeType::Span:
    case text::TextNodeType::Link:
    case text::TextNodeType::Code:
    case text::TextNodeType::CodeLineNumber:
    case text::TextNodeType::CodeLineText:
    case text::TextNodeType::OptionExecutable:
    case text::TextNodeType::OptionModule:
    case text::TextNodeType::OptionName:
    case text::TextNodeType::OptionShort:
    case text::TextNodeType::OptionLong:
    case text::TextNodeType::OptionMeta:
    case text::TextNodeType::OptionOptional:
    case text::TextNodeType::OptionDetails:
    case text::TextNodeType::Separator:
    case text::TextNodeType::EscapeSequence:
        appendParagraphLikeNode(node, context);
        return;
    case text::TextNodeType::Heading:
        emitBlock(heading(node, context));
        return;
    case text::TextNodeType::HorizontalLine:
        emitBlock(horizontalRule(context));
        return;
    case text::TextNodeType::None:
    case text::TextNodeType::_count:
        return;
    }
}

void RenderEngine::appendContainer(const text::TextNode &node, const RenderContext &context) {
    const auto rule = ruleFor(node);
    if (rule.prefix().has_value()) {
        emitBlock(paragraph(BlockString{*rule.prefix()}, TerminalDocumentStyleRule{}, context));
    }
    openScope(
        rule.margins(),
        std::nullopt,
        rule.linePrefix().has_value() ? std::optional<BlockString>{BlockString{*rule.linePrefix()}} : std::nullopt);
    const auto childContext = context.withContainer(rule);
    for (const auto &child : node.children()) {
        if (child->type().renderClass() == text::TextNodeType::RenderClass::Inline ||
            child->type().renderClass() == text::TextNodeType::RenderClass::Empty) {
            continue;
        }
        appendNode(*child, childContext);
    }
    closeScope();
    if (rule.suffix().has_value()) {
        emitBlock(paragraph(BlockString{*rule.suffix()}, TerminalDocumentStyleRule{}, context));
    }
}

void RenderEngine::appendCodeSnippet(const text::TextNode &node, const RenderContext &context) {
    const auto snippetRule = ruleFor(node);
    openScope(snippetRule.margins());
    const auto snippetContext = context.withContainer(snippetRule);
    for (const auto &line : node.children()) {
        if (line->type() == text::TextNodeType::CodeLine) {
            appendCodeSnippetLine(*line, snippetContext);
        }
    }
    closeScope();
}

void RenderEngine::appendCodeSnippetLine(const text::TextNode &line, const RenderContext &context) {
    auto number = text::String{};
    auto source = text::String{};
    auto markerNodes = std::vector<text::TextNodePtr>{};
    auto markerLabels = std::vector<text::String>{};
    auto markers = std::vector<text::impl::CodeSnippetLayoutMarker>{};
    for (const auto &child : line.children()) {
        if (child->type() == text::TextNodeType::CodeLineNumber) {
            number = nodeText(*child);
        } else if (child->type() == text::TextNodeType::CodeLineText) {
            source = nodeText(*child);
        } else if (child->type() == text::TextNodeType::CodeLineMarker && child->data() != nullptr) {
            if (const auto markerData = std::dynamic_pointer_cast<const text::impl::CodeLineMarkerData>(child->data());
                markerData != nullptr) {
                markerNodes.push_back(child);
                markerLabels.push_back(nodeText(*child));
                markers.push_back(text::impl::CodeSnippetLayoutMarker{markerData->range(), markerLabels.back()});
            }
        }
    }

    const auto lineRule = ruleFor(line);
    const auto lineIndents = context.resolvedIndents(lineRule);
    const auto sourceRule = ruleFor(text::TextNodeType::CodeLineText, std::nullopt, {});
    const auto sourceStyle = context.resolvedTextStyle(_style.baseTextStyle(), sourceRule);
    const auto gutter = makeCodeSnippetGutter(number, !number.isEmpty(), context);
    const auto markerGutter = makeCodeSnippetGutter({}, !number.isEmpty(), context);
    const auto margins = lineIndents.margins();
    const auto availableWidth = std::max(
        _width - frameWidth() - positive(margins.left()) - positive(margins.right()) - gutter.displayWidth(), 1);
    const auto rows = text::impl::CodeSnippetLayout::build(source, markers, availableWidth);
    for (auto rowIndex = std::size_t{0}; rowIndex < rows.size(); ++rowIndex) {
        _blockBuilder.clear();
        _blockBuilder.append(rowIndex == 0 ? BlockStringView{gutter} : BlockStringView{markerGutter});
        for (const auto &cell : rows[rowIndex].cells) {
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
            if (!text::impl::CodeSnippetLayout::markerIntersects(rows[rowIndex], marker)) {
                continue;
            }
            const auto markerStyle =
                context.resolvedTextStyle(_style.baseTextStyle(), ruleFor(*markerNodes[markerIndex]));
            _blockBuilder.clear();
            _blockBuilder.append(markerGutter);
            const auto start = text::impl::CodeSnippetLayout::markerStart(rows[rowIndex], marker);
            for (auto index = 0; index < start; ++index) {
                _blockBuilder.append(Block{text::Char{U' '}, sourceStyle});
            }
            const auto point = marker.range.isEmpty();
            const auto length = text::impl::CodeSnippetLayout::markerLength(rows[rowIndex], marker);
            for (auto index = 0; index < length; ++index) {
                _blockBuilder.append(Block{text::Char{point ? U'↑' : U'▔'}, markerStyle});
            }
            auto isLastMarkerRow = true;
            for (auto following = rowIndex + 1; following < rows.size(); ++following) {
                if (text::impl::CodeSnippetLayout::markerIntersects(rows[following], marker)) {
                    isLastMarkerRow = false;
                    break;
                }
            }
            const auto label = markerLabels[markerIndex];
            const auto annotationWidth = static_cast<std::size_t>(start) + static_cast<std::size_t>(length);
            if (isLastMarkerRow && !label.isEmpty() &&
                annotationWidth + label.characterLength().toSizeT() + 3U <= static_cast<std::size_t>(availableWidth)) {
                _blockBuilder.appendStyled(" ("_el, markerStyle);
                _blockBuilder.appendStyled(label, markerStyle);
                _blockBuilder.appendStyled(")"_el, markerStyle);
            }
            emitBlock(RenderBlock{BlockKind::Preformatted, _blockBuilder.toString(), lineIndents});
        }
    }
}

auto RenderEngine::makeCodeSnippetGutter(
    const text::StringView number, const bool hasNumber, const RenderContext &context) -> BlockString {
    if (!hasNumber) {
        return {};
    }
    const auto numberRule = ruleFor(text::TextNodeType::CodeLineNumber, std::nullopt, {});
    const auto numberStyle = context.resolvedTextStyle(_style.baseTextStyle(), numberRule);
    auto numberBuilder = text::StringBuilder{};
    const auto padding = std::max(cCodeLineNumberWidth - static_cast<int>(number.characterLength().toSizeT()), 0);
    numberBuilder.append(text::Char{U' '}, unit::CpLength::fromSizeT(static_cast<std::size_t>(padding)));
    numberBuilder.append(number);
    _blockBuilder.clear();
    _blockBuilder.appendStyled(numberBuilder.toString(), numberStyle);
    if (numberRule.suffix().has_value()) {
        _blockBuilder.appendWithBaseStyle(*numberRule.suffix(), numberStyle);
    }
    return _blockBuilder.toString();
}

}
