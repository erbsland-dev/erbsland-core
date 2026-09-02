// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../WritableBuffer.hpp"

#include "../../text/String.hpp"
#include "../../text/u32/U32String.hpp"

namespace erbsland::cterm::impl {

/// Renders block text into a writable buffer.
/// @tested{BlockTextRenderTest BufferConvenienceTest BufferTest}
class BlockTextPainter final {
public:
    /// Create a painter that writes to `buffer`.
    explicit BlockTextPainter(WritableBuffer &buffer) : _buffer{buffer} {}

    // defaults/deletions
    ~BlockTextPainter() = default;
    BlockTextPainter(const BlockTextPainter &) = delete;
    BlockTextPainter(BlockTextPainter &&) = delete;
    auto operator=(const BlockTextPainter &) -> BlockTextPainter & = delete;
    auto operator=(BlockTextPainter &&) -> BlockTextPainter & = delete;

public:
    /// Draw unwrapped block text at `pos`.
    void drawBlockText(block::Position pos, const BlockString &str);
    /// Draw a configured block-text object.
    void drawBlockText(const BlockText &text, std::size_t animationCycle = 0);
    /// Draw block text with the supplied text-rendering options.
    void drawBlockText(
        const BlockString &text,
        block::Rectangle rect,
        const BlockTextOptions &options,
        std::size_t animationCycle = 0);
    /// Draw UTF-8 text with a simple alignment and style.
    void drawBlockText(
        const text::String &text,
        block::Rectangle rect,
        geometry::Alignment alignment = geometry::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    /// Draw UTF-32 text with a simple alignment and style.
    void drawBlockText(
        const text::U32String &text,
        block::Rectangle rect,
        geometry::Alignment alignment = geometry::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    /// Draw block text with a simple alignment and style.
    void drawBlockText(
        const BlockString &text,
        block::Rectangle rect,
        geometry::Alignment alignment = geometry::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);

private: // wrapper (to keep code simple)
    /// Get the writable-buffer size.
    [[nodiscard]] auto size() const noexcept -> block::Size { return _buffer.size(); }
    /// Get the complete writable-buffer rectangle.
    [[nodiscard]] auto rect() const noexcept -> block::Rectangle { return _buffer.rect(); }
    /// Get the block at `pos`.
    [[nodiscard]] auto get(block::Position pos) const noexcept -> const Block & { return _buffer.get(pos); }
    /// Set the block at `pos`.
    void set(block::Position pos, const Block &block, const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    /// Fill `rect` with `fillBlock`.
    void fill(
        block::Rectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    /// Create text-rendering options from the simple drawing arguments.
    [[nodiscard]] static auto simpleBlockTextOptions(geometry::Alignment alignment, BlockStyle style) noexcept
        -> BlockTextOptions;
    /// Get the text-content rectangle after applying paragraph margins.
    [[nodiscard]] static auto contentRect(block::Rectangle rect, const ParagraphOptions &options) noexcept
        -> block::Rectangle;
    /// Wrap text into lines for block-based rendering.
    [[nodiscard]] auto buildSimpleBlockTextLines(
        const BlockString &text, block::Rectangle rect, ParagraphSpacing spacing) const -> BlockStringLines;
    /// Render one paragraph with its configured font into block-text lines.
    [[nodiscard]] auto buildFontBlockTextLines(const BlockTextOptions &options, const BlockString &paragraph) const
        -> BlockStringLines;
    /// Draw prepared text lines into `rect`.
    void applyBlockTextLines(
        block::Rectangle rect,
        const BlockTextOptions &options,
        const BlockStringLines &lines,
        std::size_t animationCycle) noexcept;
    /// Determine the text color for a rendered block position.
    [[nodiscard]] auto colorForBlockTextPosition(
        const BlockTextOptions &options,
        const Block &character,
        block::Position position,
        std::size_t animationCycle) const noexcept -> Color;

private:
    WritableBuffer &_buffer; ///< The buffer receiving rendered blocks.
};

}
