// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockKind.hpp"
#include "InlineContent.hpp"

#include "../../Block.hpp"
#include "../../BlockStringEditor.hpp"
#include "../../ParagraphIndents.hpp"
#include "../../TabOverflowBehavior.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace erbsland::cterm::impl::document_renderer {

/// A logical document block with fully resolved layout data.
///
/// Container frames remain separate from content. The physical-line layout stage is the only place where frame,
/// block margin, indentation, marker and content are combined.
/// @tested{TerminalDocumentRendererTest}
class RenderBlock final {
public:
    /// Creates a content block.
    /// @param kind The block kind.
    /// @param content The inline content.
    /// @param indents The paragraph indentation settings.
    RenderBlock(BlockKind kind, InlineContent content, ParagraphIndents indents);
    /// Creates a text block.
    /// @param kind The block kind.
    /// @param text The editable block text.
    /// @param indents The paragraph indentation settings.
    RenderBlock(BlockKind kind, BlockString text, ParagraphIndents indents);
    /// Creates a filled content block.
    /// @param kind The block kind.
    /// @param content The inline content.
    /// @param indents The paragraph indentation settings.
    /// @param fillCharacter The character that fills remaining line space.
    RenderBlock(BlockKind kind, InlineContent content, ParagraphIndents indents, Block fillCharacter);
    /// Creates a filled text block.
    /// @param kind The block kind.
    /// @param text The editable block text.
    /// @param indents The paragraph indentation settings.
    /// @param fillCharacter The character that fills remaining line space.
    RenderBlock(BlockKind kind, BlockString text, ParagraphIndents indents, Block fillCharacter);
    /// Creates a frame block.
    /// @param leadingText The optional text preceding the block.
    /// @param trailingText The optional text following the block.
    /// @param indents The paragraph indentation settings.
    /// @param fillCharacter The character that fills remaining line space.
    RenderBlock(
        std::optional<BlockString> leadingText,
        std::optional<BlockString> trailingText,
        ParagraphIndents indents,
        Block fillCharacter);

    // defaults
    RenderBlock() = default;

public: // accessors
    /// Access the logical block kind.
    [[nodiscard]] auto kind() const noexcept -> BlockKind { return _kind; }
    /// Access the inline content.
    [[nodiscard]] auto content() const noexcept -> const InlineContent & { return _content; }
    /// Access the editable inline text.
    [[nodiscard]] auto text() const noexcept -> const BlockStringEditor & { return _content.text(); }
    /// Access the optional text preceding the block.
    [[nodiscard]] auto leadingText() const noexcept -> const std::optional<BlockString> & { return _leadingText; }
    /// Access the optional text following the block.
    [[nodiscard]] auto trailingText() const noexcept -> const std::optional<BlockString> & { return _trailingText; }
    /// Access the optional list prefix.
    [[nodiscard]] auto listPrefix() const noexcept -> const std::optional<BlockString> & { return _listPrefix; }
    /// Access the block indentation settings.
    [[nodiscard]] auto indents() const noexcept -> const ParagraphIndents & { return _indents; }
    /// Access the mutable block indentation settings.
    [[nodiscard]] auto indents() noexcept -> ParagraphIndents & { return _indents; }
    /// Access the resolved frame prefix.
    [[nodiscard]] auto framePrefix() const noexcept -> const BlockString & { return _framePrefix; }
    /// Access the resolved frame right margin.
    [[nodiscard]] auto frameRightMargin() const noexcept -> int { return _frameRightMargin; }
    /// Access the optional fill character.
    [[nodiscard]] auto fillCharacter() const noexcept -> const std::optional<Block> & { return _fillCharacter; }
    /// Access the optional paragraph tab stops.
    [[nodiscard]] auto paragraphTabStops() const noexcept -> const std::optional<std::vector<int>> & {
        return _paragraphTabStops;
    }
    /// Access the optional paragraph tab overflow behavior.
    [[nodiscard]] auto paragraphTabOverflowBehavior() const noexcept -> std::optional<TabOverflowBehavior> {
        return _paragraphTabOverflowBehavior;
    }
    /// Test whether split words omit their break marker.
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
