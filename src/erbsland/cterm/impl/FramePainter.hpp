// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../WritableBuffer.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace erbsland::cterm::impl {

class FramePainter final {
    using FrameBorderReference = std::reference_wrapper<const FrameBorder::Border>;
    using OptionalFrameBorderReference = std::optional<FrameBorderReference>;

    struct FrameLine final {
        bgeo::BlockCoordinate coordinate;
        FrameBorderReference border;
    };

    using FrameLineList = std::vector<FrameLine>;

public:
    explicit FramePainter(WritableBuffer &buffer) : _buffer(buffer) {}

    // delete move/copy
    ~FramePainter() = default;
    FramePainter(const FramePainter &) = delete;
    FramePainter(FramePainter &&) = delete;
    auto operator=(const FramePainter &) -> FramePainter & = delete;
    auto operator=(FramePainter &&) -> FramePainter & = delete;

public:
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        Color frameColor = {},
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    void drawFrame(bgeo::BlockRectangle rect, FrameStyle frameStyle, Color frameColor = {}) noexcept;
    void drawFrame(
        bgeo::BlockRectangle rect,
        const FrameDrawOptions &options = FrameDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    void drawGridLayout(bgeo::BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept;
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    void drawFilledFrame(
        bgeo::BlockRectangle rect, FrameStyle frameStyle, const Block &fillBlock, Color frameColor = {}) noexcept;

private: // wrapper (to keep code simple)
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle { return _buffer.rect(); }
    [[nodiscard]] auto get(bgeo::BlockPosition pos) const noexcept -> const Block & { return _buffer.get(pos); }
    void set(bgeo::BlockPosition pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    void fill(
        bgeo::BlockRectangle rect,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    [[nodiscard]] static auto blockForFrame(
        bgeo::BlockRectangle rect, bgeo::BlockPosition pos, const Block16StylePtr &frameStyle) -> Block;
    [[nodiscard]] static auto blockForGridLine(const FrameBorder::Border &border, uint32_t bitMask) noexcept -> Block;
    [[nodiscard]] static auto lineSize(const FrameBorder::Border &border) noexcept -> bgeo::BlockCoordinate;
    [[nodiscard]] static auto lineAt(
        const FrameLineList &lines, bgeo::BlockCoordinate coordinate, std::size_t &index) noexcept
        -> OptionalFrameBorderReference;
    void addGridLine(
        FrameLineList &lines, bgeo::BlockCoordinate coordinate, const FrameBorder::Border &border) noexcept;
    void drawHorizontalGridLine(
        const FrameLine &horizontalLine,
        const FrameLineList &verticalLines,
        bgeo::BlockCoordinate x1,
        bgeo::BlockCoordinate x2,
        bgeo::BlockCoordinate y1,
        bgeo::BlockCoordinate y2) noexcept;
    void drawVerticalGridLine(
        const FrameLine &verticalLine,
        const FrameLineList &horizontalLines,
        bgeo::BlockCoordinate y1,
        bgeo::BlockCoordinate y2) noexcept;
    void drawGridBlock(
        bgeo::BlockPosition pos,
        OptionalFrameBorderReference east,
        OptionalFrameBorderReference south,
        OptionalFrameBorderReference west,
        OptionalFrameBorderReference north) noexcept;
    void drawFrameBlock(
        bgeo::BlockPosition pos,
        const Block &block,
        Color baseColor,
        const BlockCombinationStylePtr &combinationStyle) noexcept;
    [[nodiscard]] static auto colorForFramePosition(
        const ColorSequence &colorSequence,
        FrameColorMode colorMode,
        bgeo::BlockRectangle rect,
        bgeo::BlockPosition pos,
        std::size_t animationCycle,
        std::size_t animationOffset) noexcept -> Color;

private:
    WritableBuffer &_buffer;
};

}
