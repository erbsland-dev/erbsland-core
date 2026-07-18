// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitmapDrawOptions.hpp"
#include "Block16Style.hpp"
#include "BlockCombinationStyle.hpp"
#include "BlockString.hpp"
#include "BlockText.hpp"
#include "BufferDrawOptions.hpp"
#include "BufferResizeMode.hpp"
#include "FrameBorder.hpp"
#include "FrameDrawOptions.hpp"
#include "GridLayout.hpp"
#include "ReadableBuffer.hpp"
#include "Tile9Style.hpp"

#include "../text/String.hpp"
#include "../text/u32/U32String.hpp"

#include <optional>

namespace erbsland::cterm {

/// Abstract writable terminal buffer interface.
///
/// This base class combines the read-only `ReadableBuffer` API with mutation and higher-level drawing helpers such as
/// frames, text, and bitmap rendering. Concrete implementations like `Buffer` provide the actual storage.
class WritableBuffer : public ReadableBuffer {
public:
    ~WritableBuffer() override = default;

public: // abstract API
    /// Resize this buffer in a memory-efficient way.
    /// The content of the resized buffer is undefined and must be filled with new content.
    /// @param newSize The new size for the buffer.
    virtual void resize(bgeo::BlockSize newSize) = 0;
    /// Resize this buffer and optionally preserve visible content.
    /// The default implementation calls `resize(bgeo::BlockSize)` for `BufferResizeMode::Fast`.
    /// For `BufferResizeMode::PreserveContent`, it clones the current buffer, resizes it using
    /// `resize(bgeo::BlockSize)`, and restores the visible content with `setFrom()`. Implementations can override this
    /// when they provide a faster preserve-content path.
    /// @param size The new size for the buffer.
    /// @param mode How existing content should be handled during resizing.
    /// @param fillChar The character to fill newly visible cells with in preserve-content mode.
    virtual void resize(bgeo::BlockSize size, BufferResizeMode mode, Block fillChar);
    /// Write a block at the given position.
    /// @param pos The coordinates within the buffer.
    /// @param block The block value to store.
    /// @note Writes outside the buffer are ignored.
    virtual void set(bgeo::BlockPosition pos, const Block &block) noexcept = 0;

public: // convenience methods
    /// Copy the content from another buffer and match its size.
    /// This buffer is completely overwritten and resized to the size of `other`.
    /// @param other The buffer to copy from.
    virtual void setAndResizeFrom(const ReadableBuffer &other);
    /// Write a block at the given position using a combination style
    /// @param pos The coordinates within the buffer.
    /// @param block The block value to store.
    /// @param combinationStyle The combination style for overwriting existing characters.
    /// @note Writes outside the buffer are ignored.
    virtual void set(
        bgeo::BlockPosition pos, const Block &block, const BlockCombinationStylePtr &combinationStyle) noexcept;
    /// Write a string at the given position.
    /// NL jumps to the next row. Other control and zero-width characters are ignored.
    /// Color (even inherited) overwrites the existing characters. Use `drawBlockText(pos, text)` for a color overlay.
    /// @param pos The coordinates within the buffer.
    /// @param str The string to write.
    virtual void set(bgeo::BlockPosition pos, const BlockString &str) noexcept;
    /// Copy the content from another buffer into this one.
    /// This buffer is completely overwritten but not resized.
    /// If there is a size mismatch, the contents are either cut off or filled using `fillChar`.
    /// @param other The buffer to copy from.
    /// @param fillChar The character to use for filling if the sizes differ.
    void setFrom(const ReadableBuffer &other, Block fillChar = Block::space());

public: // drawing methods
    /// Fill/clear the buffer with the given character.
    /// @param fillBlock The block to use to fill the buffer.
    virtual void fill(const Block &fillBlock) noexcept;
    /// Fill the given rectangle.
    /// Positions outside the buffer are ignored.
    /// @param rect The rectangle to be filled.
    /// @param fillBlock The block for filling.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void fill(
        bgeo::BlockRectangle rect,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Fill the given rectangle using a repeating 9-tile style.
    /// Positions outside the buffer are ignored.
    /// @param rect The rectangle to be filled.
    /// @param style The tile style to repeat across the rectangle.
    /// @param baseColor The base color underneath the tile style.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void fill(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        Color baseColor = {},
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Fill the given rectangle using a repeating 9-tile style.
    /// Positions outside the buffer are ignored.
    /// @param rect The rectangle to be filled.
    /// @param style The tile style to repeat across the rectangle.
    /// @param baseStyle The base style underneath the tile style.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void fill(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        BlockStyle baseStyle,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame inside a given rectangle
    /// This will set all blocks at the edge, *inside* the given rectangle
    /// @param rect The rectangle for the frame.
    /// @param frameBlock The block for the frame.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame inside a given rectangle
    /// This will set all blocks at the edge, *inside* the given rectangle
    /// @param rect The rectangle for the frame.
    /// @param frameStyle A custom frame style.
    /// @param combinationStyle The combination style for overwriting existing characters.
    /// @param frameColor The base frame color. Any color from the frame style overlays this base color.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a frame inside a given rectangle using a repeating 9-tile style.
    /// This will set all blocks at the edge, *inside* the given rectangle.
    /// @param rect The rectangle for the frame.
    /// @param style The tile style for the frame.
    /// @param frameColor The base frame color. Any color from the style overlays this base color.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        Color frameColor = {},
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a frame inside a given rectangle
    /// This will set all blocks at the edge, *inside* the given rectangle
    /// @param rect The rectangle for the frame.
    /// @param frameStyle The predefined frame style.
    /// @param frameColor The base frame color. Any color from the frame style overlays this base color.
    void drawFrame(bgeo::BlockRectangle rect, FrameStyle frameStyle, Color frameColor = {}) noexcept;
    /// Draw a frame inside a given rectangle with configurable style, fill, and animated colors.
    /// This will set all blocks at the edge, *inside* the given rectangle.
    /// If `options.fillBlock()` is empty and no `Tile9Style` is active, the interior is left unchanged.
    /// @param rect The rectangle for the frame.
    /// @param options Frame drawing options.
    /// @param animationCycle Animation cycle for frame and fill color animations.
    void drawFrame(
        bgeo::BlockRectangle rect,
        const FrameDrawOptions &options = FrameDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    /// Draw a grid layout using reusable border styles.
    /// The layout defines only cell geometry; `border` defines which lines are visible and how they are styled.
    /// @param pos The top-left position of the full grid.
    /// @param layout The grid cell layout.
    /// @param border The frame border styling for the grid lines.
    void drawGridLayout(bgeo::BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept;
    /// Draw a box and fill it.
    /// @param rect The rectangle for the frame.
    /// @param frameBlock The block for the frame.
    /// @param fillBlock The block for filling.
    /// @param combinationStyle The combination style for overwriting existing characters.
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {}) noexcept;
    /// Draw a box and fill it.
    /// @param rect The rectangle for the frame.
    /// @param frameStyle A custom frame style.
    /// @param fillBlock The block for filling.
    /// @param combinationStyle The combination style for overwriting existing characters.
    /// @param frameColor The base frame color. Any color from the frame style overlays this base color.
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a box and fill it using a repeating 9-tile style for the frame.
    /// @param rect The rectangle for the frame.
    /// @param style The tile style for the frame.
    /// @param fillBlock The block for filling.
    /// @param combinationStyle The combination style for overwriting existing characters.
    /// @param frameColor The base frame color. Any color from the style overlays this base color.
    void drawFilledFrame(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        const Block &fillBlock,
        const BlockCombinationStylePtr &combinationStyle = {},
        Color frameColor = {}) noexcept;
    /// Draw a box and fill it.
    /// @param rect The rectangle for the frame.
    /// @param frameStyle The predefined frame style.
    /// @param fillBlock The block for filling.
    /// @param frameColor The base frame color. Any color from the frame style overlays this base color.
    void drawFilledFrame(
        bgeo::BlockRectangle rect, FrameStyle frameStyle, const Block &fillBlock, Color frameColor = {}) noexcept;
    /// Draw a text without warping from the given position.
    /// A newline breaks to the next line, starting at `pos.x`.
    /// Characters outside this buffer are cut off.
    /// @param pos The start position (top-left corner).
    /// @param str The text to draw on this buffer.
    virtual void drawBlockText(bgeo::BlockPosition pos, const BlockString &str);
    /// If fg or bg is set to `Inherited`, the current color from the buffer is used.
    /// Draw simple text into a rectangle.
    /// If fg or bg is set to `Inherited`, the current color from the buffer is used.
    /// @param text The text description.
    /// @param animationCycle Animation cycle for animated text.
    void drawBlockText(const BlockText &text, std::size_t animationCycle = 0);
    /// Draw simple text into a rectangle.
    /// If fg or bg is set to `Inherited`, the current color from the buffer is used.
    /// @param text The text to render.
    /// @param rect The target rectangle.
    /// @param alignment The alignment inside the rectangle.
    /// @param style The base text style.
    /// @param animationCycle Animation cycle for animated text.
    /// Invalid UTF-8 bytes are replaced with the Unicode replacement character.
    void drawBlockText(
        const text::String &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    /// @overload
    void drawBlockText(
        const text::U32String &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    /// @overload
    void drawBlockText(
        const BlockString &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        BlockStyle style = {},
        std::size_t animationCycle = 0);
    /// @overload
    void drawBlockText(
        const BlockString &text,
        bgeo::BlockRectangle rect,
        const BlockTextOptions &options,
        std::size_t animationCycle = 0);
    /// Calculate the height required to render wrapped text for a given rectangle width.
    /// The given width is the full target rectangle width, including margins configured in `options`.
    /// @param text The text to measure.
    /// @param width The available rectangle width in terminal cells.
    /// @param options The text options used for paragraph layout.
    /// @return The required rectangle height in terminal cells.
    [[nodiscard]] static auto blockTextHeightForWidth(
        const BlockString &text, bgeo::BlockCoordinate width, const BlockTextOptions &options) noexcept
        -> bgeo::BlockCoordinate;
    /// Draw a bitmap at a given position.
    /// The bitmap is rendered according to `options.scaleMode()`. If `options.block16Style()` is set,
    /// it overrides the scale mode and renders one terminal cell per bitmap pixel.
    /// Pixels or rendered cells outside the buffer are ignored.
    /// @param bitmap The bitmap to draw.
    /// @param pos The position of the top left corner.
    /// @param options Bitmap drawing options.
    /// @param animationCycle Animation cycle for color animations.
    void drawBitmap(
        const Bitmap &bitmap,
        bgeo::BlockPosition pos,
        const BitmapDrawOptions &options = BitmapDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    /// Draw a bitmap into the given rectangle.
    /// The rendered bitmap is aligned inside `rect`. If it is larger than `rect`, it is cropped according to the
    /// alignment.
    /// @note For half-block drawing mode, alignment and cropping happen at rendered cell boundaries, not per pixel.
    /// @param bitmap The bitmap to draw.
    /// @param rect The rectangle to draw the bitmap into.
    /// @param alignment bgeo::Alignment of the bitmap within the rectangle.
    /// @param options Bitmap drawing options.
    /// @param animationCycle Animation cycle for color animations.
    void drawBitmap(
        const Bitmap &bitmap,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft,
        const BitmapDrawOptions &options = BitmapDrawOptions::defaultOptions(),
        std::size_t animationCycle = 0) noexcept;
    /// Draw the contents of another buffer into this one.
    /// Resulting positions outside the target rectangle are clipped.
    /// @param buffer The buffer to draw.
    /// @param targetPos The target position where to draw the top-left corner of the buffer.
    /// @throws err::ParameterError if `buffer` is this buffer.
    void drawBuffer(const ReadableBuffer &buffer, bgeo::BlockPosition targetPos = bgeo::BlockPosition{});
    /// Draw the contents of another buffer into this one.
    /// Resulting positions outside the target rectangle are clipped.
    /// @param buffer The buffer to draw.
    /// @param targetRect The target rectangle where to draw the buffer. Clips `buffer` if larger.
    /// @param alignment The alignment of the buffer within the target rectangle.
    /// @throws err::ParameterError if `buffer` is this buffer.
    void drawBuffer(
        const ReadableBuffer &buffer,
        bgeo::BlockRectangle targetRect,
        bgeo::Alignment alignment = bgeo::Alignment::TopLeft);
    /// Draw the contents of another buffer into this one.
    /// @param buffer The buffer to draw.
    /// @param options The options for drawing the buffer.
    /// @throws err::ParameterError if `buffer` is this buffer.
    virtual void drawBuffer(const ReadableBuffer &buffer, const BufferDrawOptions &options);

protected: // implementation
    /// Implement `setFrom()`.
    /// The public `setFrom()` wrapper forwards to this method.
    /// @param other The buffer to copy from.
    /// @param fillChar The character to use for filling if the sizes differ.
    virtual void setFromImpl(const ReadableBuffer &other, Block fillChar);
    /// Implement `fill(bgeo::BlockRectangle, const Block &, ...)`.
    /// The public overload forwards to this method.
    /// @param rect The rectangle to be filled.
    /// @param fillBlock The block for filling.
    /// @param combinationStyle The combination style for overwriting existing characters.
    virtual void fillImpl(
        bgeo::BlockRectangle rect, const Block &fillBlock, const BlockCombinationStylePtr &combinationStyle) noexcept;
    /// Implement `fill(bgeo::BlockRectangle, const Tile9StylePtr &, ...)`.
    /// The public overload forwards to this method.
    /// @param rect The rectangle to be filled.
    /// @param style The tile style to repeat across the rectangle.
    /// @param baseStyle The base style underneath the tile style.
    /// @param combinationStyle The combination style for overwriting existing characters.
    virtual void fillImpl(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        BlockStyle baseStyle,
        const BlockCombinationStylePtr &combinationStyle) noexcept;
    /// Implement the public frame drawing overloads using explicit frame blocks.
    /// The public block-based overloads forward to this method.
    /// @param rect The rectangle for the frame.
    /// @param frameBlock The block for the frame.
    /// @param fillBlock Optional fill block for the interior.
    /// @param combinationStyle The combination style for overwriting existing characters.
    virtual void drawFrameImpl(
        bgeo::BlockRectangle rect,
        const Block &frameBlock,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &combinationStyle) noexcept;
    /// Implement the public frame drawing overloads using a Char16 style.
    /// The public Char16-based overloads forward to this method.
    /// @param rect The rectangle for the frame.
    /// @param frameStyle The custom frame style.
    /// @param fillBlock Optional fill block for the interior.
    /// @param combinationStyle The combination style for overwriting existing characters.
    /// @param frameColor The base frame color. Any color from the frame style overlays this base color.
    virtual void drawFrameImpl(
        bgeo::BlockRectangle rect,
        const Block16StylePtr &frameStyle,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &combinationStyle,
        Color frameColor) noexcept;
    /// Implement the public frame drawing overloads using a Tile9 style.
    /// The public Tile9-based overloads forward to this method.
    /// @param rect The rectangle for the frame.
    /// @param style The tile style for the frame.
    /// @param fillBlock Optional fill block for the interior.
    /// @param combinationStyle The combination style for overwriting existing characters.
    /// @param frameColor The base frame color. Any color from the style overlays this base color.
    virtual void drawFrameImpl(
        bgeo::BlockRectangle rect,
        const Tile9StylePtr &style,
        std::optional<Block> fillBlock,
        const BlockCombinationStylePtr &combinationStyle,
        Color frameColor) noexcept;
    /// Implement `drawFrame(bgeo::BlockRectangle, const FrameDrawOptions &, ...)`.
    /// The public options overload forwards to this method.
    /// @param rect The rectangle for the frame.
    /// @param options Frame drawing options.
    /// @param animationCycle Animation cycle for frame and fill color animations.
    virtual void drawFrameImpl(
        bgeo::BlockRectangle rect, const FrameDrawOptions &options, std::size_t animationCycle) noexcept;
    /// Implement `drawGridLayout(bgeo::BlockPosition, const GridLayout &, const FrameBorder &)`.
    /// The public overload forwards to this method.
    /// @param pos The top-left position of the full grid.
    /// @param layout The grid cell layout.
    /// @param border The frame border styling for the grid lines.
    virtual void drawGridLayoutImpl(
        bgeo::BlockPosition pos, const GridLayout &layout, const FrameBorder &border) noexcept;
    /// Implement `drawBlockText(const BlockText &, ...)`.
    /// The public overload forwards to this method.
    /// @param text The text description.
    /// @param animationCycle Animation cycle for animated text.
    virtual void drawBlockTextImpl(const BlockText &text, std::size_t animationCycle);
    /// Implement `drawBlockText(BlockString, bgeo::BlockRectangle, ...)`.
    /// The public overload forwards to this method.
    /// @param text The text to render.
    /// @param rect The target rectangle.
    /// @param alignment The alignment inside the rectangle.
    /// @param style The text color.
    /// @param animationCycle Animation cycle for animated text.
    virtual void drawBlockTextImpl(
        const BlockString &text,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment,
        BlockStyle style,
        std::size_t animationCycle);
    /// Implement `drawBlockText(BlockString, bgeo::BlockRectangle, BlockTextOptions, ...)`.
    /// The public overload forwards to this method.
    /// @param text The text to render.
    /// @param rect The target rectangle.
    /// @param options The text drawing options.
    /// @param animationCycle Animation cycle for animated text.
    virtual void drawBlockTextImpl(
        const BlockString &text,
        bgeo::BlockRectangle rect,
        const BlockTextOptions &options,
        std::size_t animationCycle);
    /// Implement `drawBitmap(const Bitmap &, bgeo::BlockPosition, ...)`.
    /// The public overload forwards to this method.
    /// @param bitmap The bitmap to draw.
    /// @param pos The position of the top left corner.
    /// @param options Bitmap drawing options.
    /// @param animationCycle Animation cycle for color animations.
    virtual void drawBitmapImpl(
        const Bitmap &bitmap,
        bgeo::BlockPosition pos,
        const BitmapDrawOptions &options,
        std::size_t animationCycle) noexcept;
    /// Implement `drawBitmap(const Bitmap &, bgeo::BlockRectangle, ...)`.
    /// The public overload forwards to this method.
    /// @param bitmap The bitmap to draw.
    /// @param rect The rectangle to draw the bitmap into.
    /// @param alignment bgeo::Alignment of the bitmap within the rectangle.
    /// @param options Bitmap drawing options.
    /// @param animationCycle Animation cycle for color animations.
    virtual void drawBitmapImpl(
        const Bitmap &bitmap,
        bgeo::BlockRectangle rect,
        bgeo::Alignment alignment,
        const BitmapDrawOptions &options,
        std::size_t animationCycle) noexcept;
};

}
