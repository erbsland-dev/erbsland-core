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
#include <utility>
#include <vector>

namespace erbsland::cterm::impl::document_renderer {

using namespace text::literals;
using namespace text;

void RenderEngine::appendList(const TextNode &node, const TextNodeType itemType, const RenderContext &context) {
    const auto listContainerRule = ruleFor(node);
    openScope(listContainerRule.margins());
    const auto childContext = context.withContainer(listContainerRule);
    auto number = std::size_t{1};
    for (const auto &child : node.children()) {
        if (child->type() != itemType) {
            continue;
        }
        const auto listItemRule = ruleFor(*child, std::max(node.level(), 0));
        const auto listItemLayout = makeListItemLayout(listItemRule, childContext, number);
        appendListItem(*child, listItemRule, listItemLayout, childContext);
        number += 1U;
    }
    closeScope();
}

void RenderEngine::appendListItem(
    const TextNode &node,
    const TerminalDocumentStyleRule &listItemRule,
    ListItemLayout listItemLayout,
    const RenderContext &context) {
    openScope(listItemRule.margins(), std::move(listItemLayout));
    const auto childContext = context.withContainer(listItemRule);
    auto inlineRun = std::vector<TextNodePtr>{};
    for (const auto &child : node.children()) {
        if (child->type().renderClass() == TextNodeType::RenderClass::Inline) {
            inlineRun.push_back(child);
            continue;
        }
        appendListItemInlineRun(inlineRun, childContext);
        appendListItemChild(*child, childContext);
    }
    appendListItemInlineRun(inlineRun, childContext);
    if (!currentListItemHasBlocks()) {
        emitBlock(paragraph(BlockString{}, listItemParagraphRule(), childContext));
    }
    closeScope();
}

void RenderEngine::appendListItemWithoutMarker(const TextNode &node, const RenderContext &context) {
    auto inlineRun = std::vector<TextNodePtr>{};
    for (const auto &child : node.children()) {
        if (child->type().renderClass() == TextNodeType::RenderClass::Inline) {
            inlineRun.push_back(child);
            continue;
        }
        appendListItemInlineRun(inlineRun, context);
        appendNode(*child, context);
    }
    appendListItemInlineRun(inlineRun, context);
}

void RenderEngine::appendListItemChild(const TextNode &node, const RenderContext &context) {
    if (node.type().isListContainer() || node.type() == TextNodeType::TermList) {
        if (!currentListItemHasBlocks()) {
            emitBlock(paragraph(BlockString{}, listItemParagraphRule(), context));
        }
        appendNode(node, context);
        return;
    }
    if (node.type() == TextNodeType::Paragraph) {
        emitBlock(paragraph(node, listItemParagraphRule(), context));
        return;
    }
    appendNode(node, context);
}

void RenderEngine::appendListItemInlineRun(std::vector<TextNodePtr> &nodes, const RenderContext &context) {
    if (nodes.empty()) {
        return;
    }
    const auto rule = listItemParagraphRule();
    const auto textStyle = context.resolvedTextStyle(_style.baseTextStyle(), rule);
    _inlineTextBuilder.reset();
    for (const auto &node : nodes) {
        appendInlineNode(*node, textStyle, false);
    }
    emitBlock(paragraph(_inlineTextBuilder.takeContent(), rule, context));
    nodes.clear();
}

void RenderEngine::appendTermList(const TextNode &node, const RenderContext &context) {
    const auto listRule = ruleFor(node);
    openScope(listRule.margins());
    const auto childContext = context.withContainer(listRule);
    auto items = collectTermItems(node, childContext);
    const auto layout = termListLayout(items, node.type() == TextNodeType::FieldList);
    for (const auto &item : items) {
        appendNormalTermItem(item, layout);
        if (item.node != nullptr) {
            const auto nestedContext = childContext.withContainer(ruleFor(*item.node));
            for (const auto &nestedTermList : item.nestedTermLists) {
                appendNode(*nestedTermList, nestedContext);
            }
        }
    }
    closeScope();
}

auto RenderEngine::collectTermItems(const TextNode &node, const RenderContext &context)
    -> std::vector<TermItemRenderData> {
    auto result = std::vector<TermItemRenderData>{};
    for (const auto &child : node.children()) {
        if (child->type() != TextNodeType::TermItem && child->type() != TextNodeType::FieldItem) {
            continue;
        }
        auto nameNode = TextNodePtr{};
        auto descriptionNode = TextNodePtr{};
        auto nestedTermLists = std::vector<TextNodePtr>{};
        for (const auto &termChild : child->children()) {
            if (termChild->type() == TextNodeType::TermName || termChild->type() == TextNodeType::FieldLabel) {
                nameNode = termChild;
            } else if (
                termChild->type() == TextNodeType::TermDescription || termChild->type() == TextNodeType::FieldContent) {
                descriptionNode = termChild;
            } else if (termChild->type() == TextNodeType::TermList || termChild->type() == TextNodeType::FieldList) {
                nestedTermLists.push_back(termChild);
            }
        }
        auto name = nameNode != nullptr ? renderTermName(*nameNode, context) : TermNameRenderData{};
        result.push_back(
            TermItemRenderData{
                child,
                std::move(name.text),
                0,
                name.allowsOptionFirstLineIndent,
                name.enablesOptionFirstLineIndent,
                descriptionNode != nullptr ? renderInlineText(*descriptionNode, _style.baseTextStyle(), false)
                                           : InlineContent{},
                context.resolvedIndents(ruleFor(*child)),
                std::move(nestedTermLists)});
    }
    const auto useOptionFirstLineIndent = std::ranges::any_of(
        result, [](const TermItemRenderData &item) noexcept -> bool { return item.enablesOptionFirstLineIndent; });
    if (useOptionFirstLineIndent) {
        for (auto &item : result) {
            if (item.allowsOptionFirstLineIndent) {
                item.nameFirstLineIndent = cLongOnlyOptionFirstLineIndent;
            }
        }
    }
    return result;
}

auto RenderEngine::termListLayout(const std::vector<TermItemRenderData> &items, const bool fieldList) const noexcept
    -> TermListLayout {
    auto maxNameWidth = 0;
    auto maxDescriptionWidth = 0;
    auto finalLeftMargin = 0;
    auto finalRightMargin = 0;
    for (const auto &item : items) {
        maxNameWidth = std::max(maxNameWidth, item.nameFirstLineIndent + item.name.displayWidth());
        maxDescriptionWidth = std::max(maxDescriptionWidth, item.description.displayWidth());
        finalLeftMargin = std::max(finalLeftMargin, positive(item.indents.margins().left()));
        finalRightMargin = std::max(finalRightMargin, positive(item.indents.margins().right()));
    }
    const auto terminalWidth = _width - frameWidth();
    const auto availableWidth = std::max(terminalWidth - finalLeftMargin - finalRightMargin, 0);
    if (fieldList) {
        const auto stacked = maxNameWidth > availableWidth / 4;
        // The rendered name already includes the field-label suffix. Start content one
        // column after the widest complete label.
        return TermListLayout{stacked ? finalLeftMargin + 8 : finalLeftMargin + maxNameWidth + 1, stacked, true};
    }
    auto descriptionStartColumn = 0;
    if (maxNameWidth + maxDescriptionWidth + 2 <= availableWidth) {
        descriptionStartColumn = finalLeftMargin + maxNameWidth + 2;
    } else {
        const auto idealStartColumn = terminalWidth / 4;
        if (terminalWidth - finalRightMargin - idealStartColumn >= 60) {
            descriptionStartColumn = idealStartColumn;
        } else {
            descriptionStartColumn = finalLeftMargin + 8;
        }
    }
    return TermListLayout{std::max(descriptionStartColumn, finalLeftMargin), false, false};
}

void RenderEngine::appendNormalTermItem(const TermItemRenderData &item, const TermListLayout &layout) {
    if (item.description.displayWidth() == 0) {
        auto itemIndents = item.indents;
        itemIndents.setFirstLineIndent(itemIndents.firstLineIndent() + item.nameFirstLineIndent);
        auto block = RenderBlock{BlockKind::Paragraph, item.name, std::move(itemIndents)};
        emitBlock(std::move(block));
        return;
    }
    const auto localDescriptionColumn =
        std::max(layout.descriptionStartColumn - positive(item.indents.margins().left()), 0);
    if (layout.stacked) {
        auto nameBlock = RenderBlock{BlockKind::Paragraph, item.name, item.indents};
        emitBlock(std::move(nameBlock));
        auto contentIndents = item.indents;
        contentIndents.setLineIndent(contentIndents.lineIndent() + localDescriptionColumn);
        auto contentBlock = RenderBlock{BlockKind::Paragraph, item.description, std::move(contentIndents)};
        contentBlock.setSuppressWordBreakMark(layout.fieldList);
        emitBlock(std::move(contentBlock));
        return;
    }
    auto itemIndents = item.indents;
    itemIndents.setFirstLineIndent(itemIndents.firstLineIndent() + item.nameFirstLineIndent);
    itemIndents.setWrappedLineIndent(localDescriptionColumn);
    auto content = InlineContent{};
    content.append(item.name);
    content.append(BlockStringEditor{BlockCount::one(), Block{Char{U'\t'}}});
    content.append(item.description);
    auto block = RenderBlock{BlockKind::Paragraph, std::move(content), std::move(itemIndents)};
    block.setSuppressWordBreakMark(layout.fieldList);
    block.setParagraphTabStops({localDescriptionColumn});
    if (!layout.fieldList) {
        block.setParagraphTabOverflowBehavior(TabOverflowBehavior::LineBreak);
    }
    emitBlock(std::move(block));
}

auto RenderEngine::renderTermName(const TextNode &node, const RenderContext &context) -> TermNameRenderData {
    const auto rule = ruleFor(node);
    const auto baseStyle = context.resolvedTextStyle(_style.baseTextStyle(), rule);
    auto startsWithLongOnlyOption = false;
    auto startsWithShortOption = false;
    auto content = InlineContent{};
    if (rule.prefix().has_value()) {
        _blockBuilder.clear();
        _blockBuilder.appendWithBaseStyle(*rule.prefix(), baseStyle);
        content.append(_blockBuilder.toString());
    }
    const auto usesOptionNames = node.contains(TextNodeType::OptionName);
    const auto usesOptionMeta = node.contains(TextNodeType::OptionMeta);
    auto firstOptionName = true;
    for (const auto &child : node.children()) {
        if (usesOptionNames && child->type() == TextNodeType::OptionName) {
            if (firstOptionName) {
                startsWithShortOption = child->contains(TextNodeType::OptionShort);
                startsWithLongOnlyOption = !startsWithShortOption;
            } else {
                _blockBuilder.clear();
                _blockBuilder.appendWithBaseStyle(BlockStringEditor{", "_el}, baseStyle);
                content.append(_blockBuilder.toString());
            }
            content.append(renderInlineText(*child, baseStyle, false));
            firstOptionName = false;
            continue;
        }
        if (!usesOptionNames || child->type() != TextNodeType::OptionName) {
            content.append(renderInlineText(*child, baseStyle, true));
        }
    }
    if (rule.suffix().has_value()) {
        _blockBuilder.clear();
        _blockBuilder.appendWithBaseStyle(*rule.suffix(), baseStyle);
        content.append(_blockBuilder.toString());
    }
    return TermNameRenderData{
        std::move(content),
        (usesOptionNames || usesOptionMeta) && !startsWithShortOption,
        startsWithShortOption || (startsWithLongOnlyOption && usesOptionMeta)};
}

auto RenderEngine::ruleFor(const TextNode &node) -> TerminalDocumentStyleRule {
    const auto level = usesLevel(node.type()) ? std::optional<int>{node.level()} : std::nullopt;
    return ruleFor(node.type(), level, TerminalDocumentStyleSelector::splitStyleTokens(node.style()));
}

auto RenderEngine::ruleFor(const TextNode &node, const int level) -> TerminalDocumentStyleRule {
    return ruleFor(node.type(), level, TerminalDocumentStyleSelector::splitStyleTokens(node.style()));
}

auto RenderEngine::ruleFor(
    const TextNodeType nodeType, const std::optional<int> level, const TerminalDocumentStyleSelector::TokenList &tokens)
    -> TerminalDocumentStyleRule {
    auto ancestors = _ancestors;
    if (!ancestors.empty()) {
        ancestors.pop_back();
    }
    return _style.resolve(TerminalDocumentStyleSelector{nodeType, level}, tokens, ancestors);
}

void RenderEngine::emitBlock(RenderBlock block) {
    _blockBuilder.clear();
    auto frameRightMargin = 0;
    for (const auto &scope : _scopes) {
        const auto leftMargin = positive(scope.margins.left());
        if (leftMargin > 0) {
            _blockBuilder.append(
                BlockStringEditor{BlockCount::fromSizeT(static_cast<std::size_t>(leftMargin)), Block{Char{U' '}}});
        }
        if (scope.linePrefix.has_value()) {
            _blockBuilder.append(*scope.linePrefix);
        }
        frameRightMargin += positive(scope.margins.right());
    }
    block.setFrame(_blockBuilder.toString(), frameRightMargin);
    for (auto &scope : _scopes) {
        const auto isFirstBlockInScope = !scope.hasBlocks;
        if (isFirstBlockInScope) {
            collapseVerticalMargin(block, bgeo::BlockMargins::Side::Top, scope.margins.top());
            scope.hasBlocks = true;
        }
        if (scope.listItemLayout.has_value()) {
            if (isFirstBlockInScope) {
                scope.listItemLayout->applyPrefixTo(block);
            } else {
                scope.listItemLayout->applyContinuationTo(block);
            }
        }
    }
    flushPendingBlock();
    _pendingBlock = std::move(block);
}

void RenderEngine::openScope(
    const bgeo::BlockMargins margins,
    std::optional<ListItemLayout> listItemLayout,
    std::optional<BlockString> linePrefix) {
    _scopes.push_back(BlockScope{margins, std::move(listItemLayout), false, std::move(linePrefix)});
}

void RenderEngine::closeScope() {
    if (_scopes.empty()) {
        return;
    }
    auto scope = std::move(_scopes.back());
    _scopes.pop_back();
    if (!scope.hasBlocks || !_pendingBlock.has_value()) {
        return;
    }
    collapseVerticalMargin(*_pendingBlock, bgeo::BlockMargins::Side::Bottom, scope.margins.bottom());
}

auto RenderEngine::currentListItemHasBlocks() const noexcept -> bool {
    for (auto iterator = _scopes.rbegin(); iterator != _scopes.rend(); ++iterator) {
        if (iterator->listItemLayout.has_value()) {
            return iterator->hasBlocks;
        }
    }
    return false;
}

void RenderEngine::collapseVerticalMargin(
    RenderBlock &block, const bgeo::BlockMargins::Side side, const bgeo::BlockCoordinate margin) {
    if (margin == 0) {
        return;
    }
    auto margins = block.indents().margins();
    margins.set(side, collapsedVerticalMarginValue(margins.at(side), margin));
    block.indents().setMargins(margins);
}

void RenderEngine::flushPendingBlock() {
    if (_pendingBlock.has_value()) {
        _blocks.push_back(std::move(*_pendingBlock));
        _pendingBlock.reset();
    }
}

auto RenderEngine::usesLevel(const TextNodeType nodeType) noexcept -> bool {
    return nodeType == TextNodeType::Heading || nodeType == TextNodeType::BulletList ||
        nodeType == TextNodeType::NumberedList || nodeType.isListItem();
}

auto RenderEngine::positive(const bgeo::BlockCoordinate value) noexcept -> int {
    return std::max(value.toRawValue(), 0);
}

auto RenderEngine::frameWidth() const noexcept -> int {
    auto result = 0;
    for (const auto &scope : _scopes) {
        result += positive(scope.margins.left()) + positive(scope.margins.right());
        if (scope.linePrefix.has_value()) {
            result += scope.linePrefix->displayWidth();
        }
    }
    return result;
}

void RenderEngine::appendNodeText(StringEditor &builder, const TextNode &node) {
    if (!node.text().isEmpty()) {
        builder.append(node.text());
    }
    for (const auto &child : node.children()) {
        appendNodeText(builder, *child);
    }
}

auto RenderEngine::nodeText(const TextNode &node) -> String {
    auto builder = StringEditor{};
    appendNodeText(builder, node);
    return builder;
}

}
