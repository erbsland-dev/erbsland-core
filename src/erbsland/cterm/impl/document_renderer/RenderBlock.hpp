// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InlineContent.hpp"

#include "../../Block.hpp"
#include "../../BlockString.hpp"
#include "../../ParagraphIndents.hpp"
#include "../../TabOverflowBehavior.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace erbsland::cterm::impl::document_renderer {

/// The concrete kind of a prepared document block.
enum class BlockKind : uint8_t {
    Paragraph,
    Preformatted,
    FilledLine,
    HorizontalRule,
};

/// A logical document block with fully resolved layout data.
///
/// Container frames remain separate from content. The physical-line layout stage is the only place where frame,
/// block margin, indentation, marker and content are combined.
/// @tested{TerminalDocumentRendererTest}
class RenderBlock final {
public:
    RenderBlock() = default;
    RenderBlock(BlockKind kind, InlineContent content, ParagraphIndents indents);
    RenderBlock(BlockKind kind, BlockString text, ParagraphIndents indents);
    RenderBlock(BlockKind kind, InlineContent content, ParagraphIndents indents, Block fillCharacter);
    RenderBlock(BlockKind kind, BlockString text, ParagraphIndents indents, Block fillCharacter);
    RenderBlock(
        std::optional<BlockString> leadingText,
        std::optional<BlockString> trailingText,
        ParagraphIndents indents,
        Block fillCharacter);

public: // accessors
    [[nodiscard]] auto kind() const noexcept -> BlockKind { return _kind; }
    [[nodiscard]] auto content() const noexcept -> const InlineContent & { return _content; }
    [[nodiscard]] auto text() const noexcept -> const BlockString & { return _content.text(); }
    [[nodiscard]] auto leadingText() const noexcept -> const std::optional<BlockString> & { return _leadingText; }
    [[nodiscard]] auto trailingText() const noexcept -> const std::optional<BlockString> & { return _trailingText; }
    [[nodiscard]] auto listPrefix() const noexcept -> const std::optional<BlockString> & { return _listPrefix; }
    [[nodiscard]] auto indents() const noexcept -> const ParagraphIndents & { return _indents; }
    [[nodiscard]] auto indents() noexcept -> ParagraphIndents & { return _indents; }
    [[nodiscard]] auto framePrefix() const noexcept -> const BlockString & { return _framePrefix; }
    [[nodiscard]] auto frameRightMargin() const noexcept -> int { return _frameRightMargin; }
    [[nodiscard]] auto fillCharacter() const noexcept -> const std::optional<Block> & { return _fillCharacter; }
    [[nodiscard]] auto paragraphTabStops() const noexcept -> const std::optional<std::vector<int>> & {
        return _paragraphTabStops;
    }
    [[nodiscard]] auto paragraphTabOverflowBehavior() const noexcept -> std::optional<TabOverflowBehavior> {
        return _paragraphTabOverflowBehavior;
    }
    [[nodiscard]] auto suppressWordBreakMark() const noexcept -> bool { return _suppressWordBreakMark; }

public:
    /// Set paragraph tab stops.
    void setParagraphTabStops(std::vector<int> tabStops);
    /// Set paragraph tab overflow behavior.
    void setParagraphTabOverflowBehavior(TabOverflowBehavior behavior) noexcept;
    /// Set the marker placed before the first content line.
    void setListPrefix(BlockString listPrefix);
    /// Set the resolved container frame around this block.
    void setFrame(BlockString prefix, int rightMargin) noexcept;
    /// Suppress the marker normally added when an oversized word is split.
    void setSuppressWordBreakMark(bool value) noexcept { _suppressWordBreakMark = value; }

private:
    BlockKind _kind{BlockKind::Paragraph};
    InlineContent _content;
    std::optional<BlockString> _leadingText;
    std::optional<BlockString> _trailingText;
    ParagraphIndents _indents;
    BlockString _framePrefix;
    int _frameRightMargin{0};
    std::optional<Block> _fillCharacter;
    std::optional<BlockString> _listPrefix;
    std::optional<std::vector<int>> _paragraphTabStops;
    std::optional<TabOverflowBehavior> _paragraphTabOverflowBehavior;
    bool _suppressWordBreakMark{false};
};

}
