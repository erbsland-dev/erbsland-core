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
        block::Coordinate coordinate; ///< The grid coordinate of the line.
        FrameBorderReference border;  ///< The border that defines the line.
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
        block::Rectangle rect, const Block &frameBlock, const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame using a 16-block frame style.
    void drawFrame(
        block::Rectangle rect,
        const Block16StylePtr &frameStyle,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a frame using a tiled nine-block style.
    void drawFrame(
        block::Rectangle rect,
        const Tile9StylePtr &style,
        Color frameColor = {},
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame using a predefined frame style.
    void drawFrame(block::Rectangle rect, FrameStyle frameStyle, Color frameColor = {}) noexcept;
    /// Draw a frame with the supplied draw options.
    void drawFrame(
        block::Rectangle rect,
        const FrameDrawOptions &options = FrameDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    /// Draw a bordered grid layout at `pos`.
    void drawGridLayout(block::Position pos, const GridLayout &layout, const FrameBorder &border) noexcept;
    /// Draw a frame and fill its interior with one block.
    void drawFilledFrame(
        block::Rectangle rect,
        const Block &frameBlock,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a 16-block frame and fill its interior with one block.
    void drawFilledFrame(
        block::Rectangle rect,
        const Block16StylePtr &frameStyle,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a tiled frame and fill its interior with one block.
    void drawFilledFrame(
        block::Rectangle rect,
        const Tile9StylePtr &style,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a predefined frame and fill its interior with one block.
    void drawFilledFrame(
        block::Rectangle rect, FrameStyle frameStyle, const Block &fillBlock, Color frameColor = {}) noexcept;

private: // wrapper (to keep code simple)
    /// Access the complete buffer rectangle.
    [[nodiscard]] auto rect() const noexcept -> block::Rectangle { return _buffer.rect(); }
    /// Access the block at `pos`.
    [[nodiscard]] auto get(block::Position pos) const noexcept -> const Block & { return _buffer.get(pos); }
    /// Set the block at `pos`.
    void set(block::Position pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept {
        _buffer.set(pos, block, combinationStyle);
    }
    /// Fill `rect` with `fillBlock`.
    void fill(
        block::Rectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &combinationStyle = {}) noexcept {
        _buffer.fill(rect, fillBlock, combinationStyle);
    }

private: // helper
    /// Select the block for a position in a 16-block frame.
    [[nodiscard]] static auto blockForFrame(
        block::Rectangle rect, block::Position pos, const Block16StylePtr &frameStyle) -> Block;
    /// Create a grid-line block from its border and connections.
    [[nodiscard]] static auto blockForGridLine(const FrameBorder::Border &border, uint32_t bitMask) noexcept -> Block;
    /// Get the width or height occupied by `border`.
    [[nodiscard]] static auto lineSize(const FrameBorder::Border &border) noexcept -> block::Coordinate;
    /// Find the line at `coordinate` and advance the search index.
    [[nodiscard]] static auto lineAt(
        const FrameLineList &lines, block::Coordinate coordinate, std::size_t &index) noexcept
        -> OptionalFrameBorderReference;
    /// Add a border line at `coordinate`.
    void addGridLine(FrameLineList &lines, block::Coordinate coordinate, const FrameBorder::Border &border) noexcept;
    /// Draw one horizontal grid line between its intersections.
    void drawHorizontalGridLine(
        const FrameLine &horizontalLine,
        const FrameLineList &verticalLines,
        block::Coordinate x1,
        block::Coordinate x2,
        block::Coordinate y1,
        block::Coordinate y2) noexcept;
    /// Draw one vertical grid line between its intersections.
    void drawVerticalGridLine(
        const FrameLine &verticalLine,
        const FrameLineList &horizontalLines,
        block::Coordinate y1,
        block::Coordinate y2) noexcept;
    /// Draw the grid block whose four neighboring borders are supplied.
    void drawGridBlock(
        block::Position pos,
        OptionalFrameBorderReference east,
        OptionalFrameBorderReference south,
        OptionalFrameBorderReference west,
        OptionalFrameBorderReference north) noexcept;
    /// Draw a frame block combined with the current buffer block.
    void drawFrameBlock(
        block::Position pos,
        const Block &block,
        Color baseColor,
        const BlockCombinationStylePtr &combinationStyle) noexcept;
    /// Select the color for one frame position.
    [[nodiscard]] static auto colorForFramePosition(
        const ColorSequence &colorSequence,
        FrameColorMode colorMode,
        block::Rectangle rect,
        block::Position pos,
        std::size_t animationCycle,
        std::size_t animationOffset) noexcept -> Color;

private:
    WritableBuffer &_buffer; ///< The buffer receiving rendered blocks.
};

}
