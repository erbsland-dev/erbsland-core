// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderEngine.hpp"

#include "Helpers.hpp"

#include <utility>

namespace erbsland::cterm::impl::document_renderer {

void RenderEngine::appendParagraphLikeNode(const text::TextNode &node, const RenderContext &context) {
    const auto rule = ruleFor(node);
    emitBlock(paragraph(node, rule, context));
}

auto RenderEngine::paragraph(
    const text::TextNode &node, const TerminalDocumentStyleRule &rule, const RenderContext &context) -> RenderBlock {
    const auto textStyle = context.resolvedTextStyle(_style.baseTextStyle(), rule);
    auto content = renderInlineText(node, textStyle, node.type().preserveWhitespace());
    return paragraph(std::move(content), rule, context);
}

auto RenderEngine::paragraph(BlockString text, const TerminalDocumentStyleRule &rule, const RenderContext &context)
    -> RenderBlock {
    return paragraph(InlineContent{std::move(text)}, rule, context);
}

auto RenderEngine::paragraph(InlineContent content, const TerminalDocumentStyleRule &rule, const RenderContext &context)
    -> RenderBlock {
    const auto textStyle = context.resolvedTextStyle(_style.baseTextStyle(), rule);
    auto result = InlineContent{};
    if (rule.prefix().has_value()) {
        _blockBuilder.clear();
        _blockBuilder.appendWithBaseStyle(*rule.prefix(), textStyle);
        result.append(_blockBuilder.toString());
    }
    result.append(content);
    if (rule.suffix().has_value()) {
        _blockBuilder.clear();
        _blockBuilder.appendWithBaseStyle(*rule.suffix(), textStyle);
        result.append(_blockBuilder.toString());
    }
    return RenderBlock{BlockKind::Paragraph, std::move(result), context.resolvedIndents(rule)};
}

auto RenderEngine::heading(const text::TextNode &node, const RenderContext &context) -> RenderBlock {
    const auto rule = ruleFor(node);
    const auto headingTextStyle = context.resolvedTextStyle(_style.baseTextStyle(), rule);
    auto renderedText = renderInlineText(node, headingTextStyle, false);
    auto content = InlineContent{};
    if (rule.prefix().has_value()) {
        _blockBuilder.clear();
        _blockBuilder.appendWithBaseStyle(*rule.prefix(), headingTextStyle);
        content.append(_blockBuilder.toString());
    }
    content.append(renderedText);
    if (rule.suffix().has_value()) {
        _blockBuilder.clear();
        _blockBuilder.appendWithBaseStyle(*rule.suffix(), headingTextStyle);
        content.append(_blockBuilder.toString());
    }
    auto indents = context.resolvedIndents(rule);
    if (rule.lineFill().has_value()) {
        return RenderBlock{
            BlockKind::FilledLine, std::move(content), std::move(indents), rule.lineFill()->withBase(headingTextStyle)};
    }
    return RenderBlock{BlockKind::Paragraph, std::move(content), std::move(indents)};
}

auto RenderEngine::horizontalRule(const RenderContext &context) -> RenderBlock {
    const auto rule = ruleFor(text::TextNodeType::HorizontalLine, std::nullopt, {});
    const auto ruleTextStyle = context.resolvedTextStyle(_style.baseTextStyle(), rule);
    return RenderBlock{
        resolvedDecoration(_decorationBuilder, rule.prefix(), ruleTextStyle),
        resolvedDecoration(_decorationBuilder, rule.suffix(), ruleTextStyle),
        context.resolvedIndents(rule),
        rule.lineFill().has_value() ? rule.lineFill()->withBase(ruleTextStyle)
                                    : Block{text::Char{U'-'}, ruleTextStyle}};
}

auto RenderEngine::makeListItemLayout(
    const TerminalDocumentStyleRule &listItemRule, const RenderContext &context, const std::size_t number)
    -> ListItemLayout {
    const auto textStyle = context.resolvedTextStyle(_style.baseTextStyle(), listItemRule);
    auto renderedMarker = listItemRule.marker().render(number, textStyle);
    return {
        std::move(renderedMarker),
        listItemRule.indents().firstLineIndent(),
        listItemRule.indents().wrappedLineIndent()};
}

auto RenderEngine::renderInlineText(const text::TextNode &node, const BlockStyle style, const bool preserveWhitespace)
    -> InlineContent {
    _inlineTextBuilder.reset();
    appendInlineNode(node, style, preserveWhitespace);
    return _inlineTextBuilder.takeContent();
}

void RenderEngine::appendInlineNode(const text::TextNode &node, const BlockStyle style, const bool preserveWhitespace) {
    switch (node.type().raw()) {
    case text::TextNodeType::Text:
    case text::TextNodeType::Unsupported:
    case text::TextNodeType::Error:
        _inlineTextBuilder.appendText(node.text(), style, preserveWhitespace);
        return;
    case text::TextNodeType::LineBreak:
        _inlineTextBuilder.appendLineBreak(style);
        return;
    default:
        break;
    }
    if (node.type().isInline()) {
        const auto inlineRule = ruleFor(node);
        const auto inlineStyle = style.withOverlay(inlineRule.textStyle());
        if (inlineRule.prefix().has_value()) {
            _inlineTextBuilder.appendDecoration(*inlineRule.prefix(), inlineStyle, preserveWhitespace);
        }
        const auto indivisibleStart = BlockIndex::end(_inlineTextBuilder.length());
        if (!node.text().isEmpty()) {
            _inlineTextBuilder.appendText(node.text(), inlineStyle, preserveWhitespace);
        }
        appendInlineChildren(node, inlineStyle, preserveWhitespace);
        if (inlineRule.suffix().has_value()) {
            _inlineTextBuilder.appendDecoration(*inlineRule.suffix(), inlineStyle, preserveWhitespace);
        }
        if (node.type() == text::TextNodeType::Separator) {
            _inlineTextBuilder.addSoftBreak();
        } else if (node.type() == text::TextNodeType::EscapeSequence) {
            const auto indivisibleEnd = BlockIndex::end(_inlineTextBuilder.length());
            _inlineTextBuilder.addIndivisibleRange(
                BlockRange{indivisibleStart, indivisibleStart.absoluteDistanceTo(indivisibleEnd)});
        }
        return;
    }
    appendInlineChildren(node, style, preserveWhitespace);
}

void RenderEngine::appendInlineChildren(
    const text::TextNode &node, const BlockStyle style, const bool preserveWhitespace) {
    for (const auto &child : node.children()) {
        appendInlineNode(*child, style, preserveWhitespace);
    }
}

}
