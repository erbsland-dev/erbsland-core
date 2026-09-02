// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DocumentLayout.hpp"

#include "../paragraph/Layout.hpp"
#include "../paragraph/Printer.hpp"

#include <algorithm>

namespace erbsland::cterm::impl::document_renderer {

DocumentLayout::DocumentLayout(const int width) : _width{std::max(width, 1)} {
}

auto DocumentLayout::build(const std::vector<RenderBlock> &blocks) -> BlockStringLines {
    _lines.clear();
    _previousFramePrefix = {};
    _previousBottomMargin = 0;
    _firstBlock = true;
    for (const auto &block : blocks) {
        appendBlock(block);
    }
    if (!_firstBlock) {
        appendGap(_previousBottomMargin, _previousFramePrefix);
    }
    return std::move(_lines);
}

void DocumentLayout::appendBlock(const RenderBlock &block) {
    const auto verticalMargins = block.indents().margins().vertical();
    const auto topMargin = positive(verticalMargins.leading());
    const auto gap = _firstBlock ? topMargin : std::max(_previousBottomMargin, topMargin);
    const auto &gapPrefix =
        _firstBlock || topMargin > _previousBottomMargin ? block.framePrefix() : _previousFramePrefix;
    appendGap(gap, gapPrefix);
    switch (block.kind()) {
    case BlockKind::Paragraph:
        appendParagraph(block);
        break;
    case BlockKind::Preformatted:
        appendPreformatted(block);
        break;
    case BlockKind::FilledLine:
        appendFilledLine(block);
        break;
    case BlockKind::HorizontalRule:
        appendHorizontalRule(block);
        break;
    }
    _previousBottomMargin = positive(verticalMargins.trailing());
    _previousFramePrefix = block.framePrefix();
    _firstBlock = false;
}

void DocumentLayout::appendParagraph(const RenderBlock &block) {
    const auto contentWidth = availableContentWidth(block);
    auto content = InlineContent{};
    if (block.listPrefix().has_value()) {
        content.append(*block.listPrefix());
    }
    content.append(block.content());
    auto options = paragraphOptions(block);
    auto layout =
        paragraph::Layout{
            content.text(), contentWidth, options, paragraph::LayoutNewlineMode::HardLineBreak, &content.semantics()}
            .build();
    if (!layout.valid()) {
        appendContentLine(block, content.text());
        return;
    }
    if (layout.empty()) {
        appendContentLine(block, BlockString{});
        return;
    }
    auto materializationWidth = contentWidth;
    for (const auto &line : layout.lines()) {
        materializationWidth = std::max(materializationWidth, line.textWidth());
    }
    auto buffer = CursorBuffer{
        block::Size{materializationWidth, 1},
        CursorBuffer::OverflowMode::ExpandThenWrap,
        block::Size{materializationWidth, CursorBuffer::cMaximumSize.height().toRawValue()}};
    const auto lineCount =
        paragraph::Printer{
            buffer, 0, contentWidth, options.alignment(), layout, content.text(), options, options.backgroundMode()}
            .print();
    for (auto y = 0; y < lineCount; ++y) {
        appendContentLine(block, trimmedLine(buffer, y));
    }
}

void DocumentLayout::appendPreformatted(const RenderBlock &block) {
    for (const auto &line : BlockString{block.text()}.splitLines()) {
        appendContentLine(block, line);
    }
}

void DocumentLayout::appendFilledLine(const RenderBlock &block) {
    const auto contentWidth = availableContentWidth(block);
    auto content = BlockStringEditor{};
    if (contentWidth > 0 && block.fillCharacter().has_value()) {
        content =
            BlockStringEditor{BlockCount::fromSizeT(static_cast<std::size_t>(contentWidth)), *block.fillCharacter()};
        auto position = BlockIndex{};
        for (const auto &character : block.text()) {
            if (position >= BlockIndex::end(content.length())) {
                break;
            }
            content[position] = character;
            position += BlockCount::one();
        }
    }
    appendContentLine(block, content);
}

void DocumentLayout::appendHorizontalRule(const RenderBlock &block) {
    const auto contentWidth = availableContentWidth(block);
    auto content = BlockStringEditor{
        BlockCount::fromSizeT(static_cast<std::size_t>(contentWidth)),
        block.fillCharacter().value_or(Block{text::Char{U'-'}})};
    if (block.leadingText().has_value()) {
        auto index = BlockIndex{};
        for (const auto &character : *block.leadingText()) {
            if (index >= BlockIndex::end(content.length())) {
                break;
            }
            content[index] = character;
            index += BlockCount::one();
        }
    }
    if (block.trailingText().has_value()) {
        const auto offset = std::max(contentWidth - block.trailingText()->displayWidth(), 0);
        auto index = BlockIndex::fromSizeT(static_cast<std::size_t>(offset));
        for (const auto &character : *block.trailingText()) {
            if (index >= BlockIndex::end(content.length())) {
                break;
            }
            content[index] = character;
            index += BlockCount::one();
        }
    }
    appendContentLine(block, content);
}

void DocumentLayout::appendGap(const int count, const BlockString &prefix) {
    for (auto index = 0; index < count; ++index) {
        _lines.push_back(prefix);
    }
}

void DocumentLayout::appendContentLine(const RenderBlock &block, const BlockString &content) {
    _builder.clear();
    _builder.append(block.framePrefix());
    const auto leftMargin = positive(block.indents().margins().horizontal().leading());
    if (leftMargin > 0) {
        _builder.append(BlockStringEditor{BlockCount::fromSizeT(static_cast<std::size_t>(leftMargin)), Block::space()});
    }
    _builder.append(content);
    _lines.push_back(_builder.toString());
}

auto DocumentLayout::paragraphOptions(const RenderBlock &block) const -> ParagraphOptions {
    auto options = ParagraphOptions{};
    auto indents = block.indents();
    indents.setMargins(block::Margins{0});
    options.setIndents(indents);
    if (block.paragraphTabStops().has_value()) {
        options.setTabStops(*block.paragraphTabStops());
    }
    options.setTabOverflowBehavior(block.paragraphTabOverflowBehavior().value_or(TabOverflowBehavior::AddSpace));
    options.setWordBreakMark(block.suppressWordBreakMark() ? Block{} : Block{text::Char{U'-'}});
    options.setParagraphSpacing(ParagraphSpacing::SingleLine);
    return options;
}

auto DocumentLayout::availableContentWidth(const RenderBlock &block) const noexcept -> int {
    const auto margins = block.indents().margins().horizontal();
    const auto occupiedWidth =
        block.framePrefix().displayWidth() + block.frameRightMargin() + margins.extent().toRawValue();
    return std::max(_width - occupiedWidth, 1);
}

auto DocumentLayout::trimmedLine(const CursorBuffer &buffer, const int y) -> BlockString {
    auto result = BlockStringEditor{};
    auto lastContent = -1;
    for (auto x = 0; x < buffer.size().width().toRawValue(); ++x) {
        const auto &character = buffer.get(block::Position{x, y});
        result.append(character);
        if (character != buffer.fillChar()) {
            lastContent = x;
        }
    }
    if (lastContent + 1 < buffer.size().width().toRawValue()) {
        result =
            result.slice(BlockRange{BlockIndex{}, BlockCount::fromSizeT(static_cast<std::size_t>(lastContent + 1))});
    }
    return BlockString{result};
}

auto DocumentLayout::positive(const block::Coordinate value) noexcept -> int {
    return std::max(value.toRawValue(), 0);
}

}
