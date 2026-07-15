// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderBlock.hpp"

#include <algorithm>

namespace erbsland::cterm::impl::document_renderer {

RenderBlock::RenderBlock(BlockKind kind, InlineContent content, ParagraphIndents indents) :
    _kind{kind}, _content{std::move(content)}, _indents{std::move(indents)} {
}

RenderBlock::RenderBlock(BlockKind kind, BlockString text, ParagraphIndents indents) :
    RenderBlock{kind, InlineContent{std::move(text)}, std::move(indents)} {
}

RenderBlock::RenderBlock(BlockKind kind, InlineContent content, ParagraphIndents indents, Block fillCharacter) :
    _kind{kind}, _content{std::move(content)}, _indents{std::move(indents)}, _fillCharacter{std::move(fillCharacter)} {
}

RenderBlock::RenderBlock(BlockKind kind, BlockString text, ParagraphIndents indents, Block fillCharacter) :
    RenderBlock{kind, InlineContent{std::move(text)}, std::move(indents), std::move(fillCharacter)} {
}

RenderBlock::RenderBlock(
    std::optional<BlockString> leadingText,
    std::optional<BlockString> trailingText,
    ParagraphIndents indents,
    Block fillCharacter) :
    _kind{BlockKind::HorizontalRule},
    _leadingText{std::move(leadingText)},
    _trailingText{std::move(trailingText)},
    _indents{std::move(indents)},
    _fillCharacter{std::move(fillCharacter)} {
}

void RenderBlock::setParagraphTabStops(std::vector<int> tabStops) {
    _paragraphTabStops = std::move(tabStops);
}

void RenderBlock::setParagraphTabOverflowBehavior(const TabOverflowBehavior behavior) noexcept {
    _paragraphTabOverflowBehavior = behavior;
}

void RenderBlock::setListPrefix(BlockString listPrefix) {
    _listPrefix = std::move(listPrefix);
}

void RenderBlock::setFrame(BlockString prefix, const int rightMargin) noexcept {
    _framePrefix = std::move(prefix);
    _frameRightMargin = std::max(rightMargin, 0);
}

}
