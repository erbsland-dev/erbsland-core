// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../WritableBuffer.hpp"

#include "../../text/String.hpp"
#include "../../text/u32/U32String.hpp"

namespace erbsland::cterm::impl {

class BlockTextPainter final {
public:
    explicit BlockTextPainter(WritableBuffer &buffer) : _buffer{buffer} {}

    // delete copy/move
    ~BlockTextPainter() = default;
    BlockTextPainter(const BlockTextPainter &) = delete;
    BlockTextPainter(BlockTextPainter &&) = delete;
    auto operator=(const BlockTextPainter &) -> BlockTextPainter & = delete;
    auto operator=(BlockTextPainter &&) -> BlockTextPainter & = delete;

public:
    void drawBlockText(bgeo::BlockPosition pos, const BlockString &str);
    void drawBlockText(const BlockText &text, std::size_t animationCycle = 0);
    void drawBlockText(
        const BlockString &text,
        bgeo::BlockRectangle rect,
        const BlockTextOptions &options,
        std::size_t animationCycle = 0);
    void drawBlockText(
        const text::String &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    void drawBlockText(
        const text::U32String &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    void drawBlockText(
        const BlockString &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);

private: // wrapper (to keep code simple)
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize { return _buffer.size(); }
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle { return _buffer.rect(); }
    [[nodiscard]] auto get(bgeo::BlockPosition pos) const noexcept -> const Block & { return _buffer.get(pos); }
    void set(
        bgeo::BlockPosition pos, const Block &block, const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    void fill(
        bgeo::BlockRectangle rect,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    [[nodiscard]] static auto simpleBlockTextOptions(bgeo::Alignment alignment, BlockStyle style) noexcept
        -> BlockTextOptions;
    [[nodiscard]] static auto contentRect(bgeo::BlockRectangle rect, const ParagraphOptions &options) noexcept
        -> bgeo::BlockRectangle;
    [[nodiscard]] auto buildSimpleBlockTextLines(
        const BlockString &text, bgeo::BlockRectangle rect, ParagraphSpacing spacing) const -> BlockStringLines;
    [[nodiscard]] auto buildFontBlockTextLines(const BlockTextOptions &options, const BlockString &paragraph) const
        -> BlockStringLines;
    void applyBlockTextLines(
        bgeo::BlockRectangle rect,
        const BlockTextOptions &options,
        const BlockStringLines &lines,
        std::size_t animationCycle) noexcept;
    [[nodiscard]] auto colorForBlockTextPosition(
        const BlockTextOptions &options,
        const Block &character,
        bgeo::BlockPosition position,
        std::size_t animationCycle) const noexcept -> Color;

private:
    WritableBuffer &_buffer;
};

}
