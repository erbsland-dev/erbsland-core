// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../WritableBuffer.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace erbsland::cterm::impl {

/// Renders frames and grid borders into a writable buffer.
/// @tested{BufferTest BufferConvenienceTest}
class FramePainter final {
    /// A non-owning reference to one border definition.
    using FrameBorderReference = std::reference_wrapper<const FrameBorder::Border>;
    /// An optional non-owning reference to one border definition.
    using OptionalFrameBorderReference = std::optional<FrameBorderReference>;

    /// One horizontal or vertical grid line.
    /// @tested{BufferTest}
    struct FrameLine final {
        bgeo::BlockCoordinate coordinate; ///< The grid coordinate of the line.
        FrameBorderReference border;      ///< The border that defines the line.
    };

    /// The lines of one grid orientation, ordered by coordinate.
    using FrameLineList = std::vector<FrameLine>;

public:
    /// Create a painter that writes to `buffer`.
    explicit FramePainter(WritableBuffer &buffer) : _buffer(buffer) {}

    // defaults/deletions
    ~FramePainter() = default;
    FramePainter(const FramePainter &) = delete;
    FramePainter(FramePainter &&) = delete;
    auto operator=(const FramePainter &) -> FramePainter & = delete;
    auto operator=(FramePainter &&) -> FramePainter & = delete;

public:
    /// Draw a frame using one block for every frame position.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame using a 16-block frame style.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a frame using a tiled nine-block style.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        Color frameColor = {},
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame using a predefined frame style.
    void drawFrame(bgeo::BlockRectangle rect, FrameStyle frameStyle, Color frameColor = {}) noexcept;
    /// Draw a frame with the supplied draw options.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const FrameDrawOptions &options = FrameDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    /// Draw a bordered grid layout at `pos`.
    void drawGridLayout(bgeo::BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept;
    /// Draw a frame and fill its interior with one block.
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a 16-block frame and fill its interior with one block.
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a tiled frame and fill its interior with one block.
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a predefined frame and fill its interior with one block.
    void drawFilledFrame(
        bgeo::BlockRectangle rect, FrameStyle frameStyle, const Block &fillBlock, Color frameColor = {}) noexcept;

private: // wrapper (to keep code simple)
    /// Access the complete buffer rectangle.
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle { return _buffer.rect(); }
    /// Access the block at `pos`.
    [[nodiscard]] auto get(bgeo::BlockPosition pos) const noexcept -> const Block & { return _buffer.get(pos); }
    /// Set the block at `pos`.
    void set(bgeo::BlockPosition pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    /// Fill `rect` with `fillBlock`.
    void fill(
        bgeo::BlockRectangle rect,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    /// Select the block for a position in a 16-block frame.
    [[nodiscard]] static auto blockForFrame(
        bgeo::BlockRectangle rect, bgeo::BlockPosition pos, const Block16StylePtr &frameStyle) -> Block;
    /// Create a grid-line block from its border and connections.
    [[nodiscard]] static auto blockForGridLine(const FrameBorder::Border &border, uint32_t bitMask) noexcept -> Block;
    /// Get the width or height occupied by `border`.
    [[nodiscard]] static auto lineSize(const FrameBorder::Border &border) noexcept -> bgeo::BlockCoordinate;
    /// Find the line at `coordinate` and advance the search index.
    [[nodiscard]] static auto lineAt(
        const FrameLineList &lines, bgeo::BlockCoordinate coordinate, std::size_t &index) noexcept
        -> OptionalFrameBorderReference;
    /// Add a border line at `coordinate`.
    void addGridLine(
        FrameLineList &lines, bgeo::BlockCoordinate coordinate, const FrameBorder::Border &border) noexcept;
    /// Draw one horizontal grid line between its intersections.
    void drawHorizontalGridLine(
        const FrameLine &horizontalLine,
        const FrameLineList &verticalLines,
        bgeo::BlockCoordinate x1,
        bgeo::BlockCoordinate x2,
        bgeo::BlockCoordinate y1,
        bgeo::BlockCoordinate y2) noexcept;
    /// Draw one vertical grid line between its intersections.
    void drawVerticalGridLine(
        const FrameLine &verticalLine,
        const FrameLineList &horizontalLines,
        bgeo::BlockCoordinate y1,
        bgeo::BlockCoordinate y2) noexcept;
    /// Draw the grid block whose four neighboring borders are supplied.
    void drawGridBlock(
        bgeo::BlockPosition pos,
        OptionalFrameBorderReference east,
        OptionalFrameBorderReference south,
        OptionalFrameBorderReference west,
        OptionalFrameBorderReference north) noexcept;
    /// Draw a frame block combined with the current buffer block.
    void drawFrameBlock(
        bgeo::BlockPosition pos,
        const Block &block,
        Color baseColor,
        const BlockCombinationStylePtr &combinationStyle) noexcept;
    /// Select the color for one frame position.
    [[nodiscard]] static auto colorForFramePosition(
        const ColorSequence &colorSequence,
        FrameColorMode colorMode,
        bgeo::BlockRectangle rect,
        bgeo::BlockPosition pos,
        std::size_t animationCycle,
        std::size_t animationOffset) noexcept -> Color;

private:
    WritableBuffer &_buffer; ///< The buffer receiving rendered blocks.
};

}
