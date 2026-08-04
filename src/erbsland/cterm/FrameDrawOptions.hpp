// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block16Style_fwd.hpp"
#include "BlockCombinationStyle.hpp"
#include "Color.hpp"
#include "ColorPart.hpp"
#include "ColorSequence.hpp"
#include "FrameColorMode.hpp"
#include "FrameStyle.hpp"
#include "Tile9Style_fwd.hpp"

namespace erbsland::cterm {

/// The options to draw a frame.
/// These options define the frame style, optional fill, combination style, and animated color behavior for
/// `Buffer::drawFrame()`.
/// The style priority is `tile9Style()`, then `block16Style()`, and finally `style()`.
/// If a `Tile9Style` is active, it also controls the fill area and overrides `fillBlock()`.
/// Frame colors are overlaid as `buffer -> frameColor -> tile/block color`.
/// Fill colors are overlaid as `buffer -> frameColor -> fillColor -> tile/block color`.
/// @note Creating custom option instances is expensive. Reuse them across multiple `drawFrame()` calls.
class FrameDrawOptions final {
public:
    /// Create default frame draw options.
    FrameDrawOptions() = default;
    /// Create options for one fixed frame color.
    /// @param frameColor The base frame color.
    template <typename tColor>
        requires ColorArg<tColor>
    explicit FrameDrawOptions(tColor frameColor) : _frameColor{frameColor} {}
    /// Create options from a frame color sequence.
    /// @param frameColor The base frame colors.
    /// @param frameColorMode The mode used to pick colors from the frame sequence.
    explicit FrameDrawOptions(ColorSequence frameColor, FrameColorMode frameColorMode = FrameColorMode::OneColor);

    // defaults
    ~FrameDrawOptions() = default;
    FrameDrawOptions(const FrameDrawOptions &) = default;
    FrameDrawOptions(FrameDrawOptions &&) noexcept = default;
    auto operator=(const FrameDrawOptions &) -> FrameDrawOptions & = default;
    auto operator=(FrameDrawOptions &&) -> FrameDrawOptions & = default;

public:
    /// The frame color sequence.
    /// If this contains only inherited colors, the frame inherits the full color from the buffer below.
    [[nodiscard]] auto frameColor() const noexcept -> const ColorSequence &;
    /// Set a single frame color.
    void setFrameColor(Color frameColor) noexcept;
    /// Set explicit foreground and background colors for the frame.
    void setFrameColor(Foreground foreground, Background background) noexcept;
    /// Set the frame color sequence.
    void setFrameColorSequence(
        ColorSequence frameColor, FrameColorMode frameColorMode = FrameColorMode::OneColor) noexcept;
    /// The fill color sequence.
    /// The fill colors are applied after `frameColor()`, which allows subtle adjustments on top of animated borders.
    [[nodiscard]] auto fillColor() const noexcept -> const ColorSequence &;
    /// Set a single fill color.
    void setFillColor(Color fillColor) noexcept;
    /// Set explicit foreground and background colors for the fill.
    void setFillColor(Foreground foreground, Background background) noexcept;
    /// Set the fill color sequence.
    void setFillColorSequence(
        ColorSequence fillColor, FrameColorMode fillColorMode = FrameColorMode::OneColor) noexcept;
    /// The fill block.
    /// An empty block disables filling when no `Tile9Style` is active.
    [[nodiscard]] auto fillBlock() const noexcept -> const Block &;
    /// Set the fill block.
    /// Use `Block{}` to disable the fill.
    void setFillBlock(Block fillBlock) noexcept;
    /// The predefined frame style.
    [[nodiscard]] auto style() const noexcept -> FrameStyle;
    /// Set the predefined frame style.
    void setStyle(FrameStyle style) noexcept;
    /// The custom Char16 frame style.
    /// This overrides `style()` unless `tile9Style()` is also set.
    [[nodiscard]] auto block16Style() const noexcept -> const Block16StylePtr &;
    /// Set the custom Char16 frame style.
    void setBlock16Style(Block16StylePtr block16Style) noexcept;
    /// The custom Tile9 frame style.
    /// This overrides both `block16Style()` and `style()`, and also controls the fill area.
    [[nodiscard]] auto tile9Style() const noexcept -> const Tile9StylePtr &;
    /// Set the custom Tile9 frame style.
    void setTile9Style(Tile9StylePtr tile9Style) noexcept;
    /// The combination style used for writing the frame and fill blocks.
    [[nodiscard]] auto combinationStyle() const noexcept -> const BlockCombinationStylePtr &;
    /// Set the combination style.
    void setCombinationStyle(BlockCombinationStylePtr combinationStyle) noexcept;
    /// The shared animation offset for frame and fill colors.
    /// This value is added to the `animationCycle` passed to `drawFrame()`.
    [[nodiscard]] auto animationOffset() const noexcept -> std::size_t;
    /// Set the animation offset for frame and fill colors.
    void setAnimationOffset(std::size_t offset) noexcept;
    /// The frame color mode.
    [[nodiscard]] auto frameColorMode() const noexcept -> FrameColorMode;
    /// Set the frame color mode.
    void setFrameColorMode(FrameColorMode frameColorMode) noexcept;
    /// The fill color mode.
    /// `ChasingBorderCW` and `ChasingBorderCCW` only affect frame cells.
    [[nodiscard]] auto fillColorMode() const noexcept -> FrameColorMode;
    /// Set the fill color mode.
    void setFillColorMode(FrameColorMode fillColorMode) noexcept;

public:
    /// Access the shared object with the default options.
    /// Default options use a light Unicode frame, inherited colors, no fill, and the common box frame combiner.
    [[nodiscard]] static auto defaultOptions() noexcept -> const FrameDrawOptions &;

private:
    ColorSequence _frameColor = {Color{}};
    ColorSequence _fillColor = {Color{}};
    Block _fillBlock;
    FrameStyle _style = FrameStyle::Light;
    Block16StylePtr _block16Style;
    Tile9StylePtr _tile9Style;
    BlockCombinationStylePtr _combinationStyle = BlockCombinationStyle::commonBoxFrame();
    std::size_t _animationOffset = 0;
    FrameColorMode _frameColorMode = FrameColorMode::OneColor;
    FrameColorMode _fillColorMode = FrameColorMode::OneColor;
};

}
