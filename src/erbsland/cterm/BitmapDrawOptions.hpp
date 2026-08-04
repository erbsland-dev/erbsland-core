// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitmapColorMode.hpp"
#include "BitmapScaleMode.hpp"
#include "Block16Style_fwd.hpp"
#include "BlockCombinationStyle.hpp"
#include "BlockString.hpp"
#include "Color.hpp"
#include "ColorPart.hpp"
#include "ColorSequence.hpp"

#include "../text/u32/U32String.hpp"

namespace erbsland::cterm {

/// The options to draw a bitmap.
/// These options define how `Buffer::drawBitmap()` converts bitmap pixels into terminal cells.
/// For color animation and stripe modes, the color position is calculated in the rendered bitmap grid:
/// `FullBlock`, `DoubleBlock`, and `Block16Style` use one logical position per bitmap pixel,
/// while `HalfBlock` uses one logical position per 2x2 pixel cell.
/// The rectangle overload of `drawBitmap()` aligns this rendered grid inside the target rectangle and crops it if
/// needed.
/// @note Creating custom option instances is expensive. For that reason, create them once and
///       keep the instances for multiple `drawBitmap` calls.
class BitmapDrawOptions final {
public:
    /// Create default bitmap draw options.
    BitmapDrawOptions() = default;
    /// Create options for one fixed color.
    /// @param color The base color for the bitmap.
    template <typename tColor>
        requires ColorArg<tColor>
    explicit BitmapDrawOptions(tColor color) : _color{color} {}
    /// Create options from a color sequence.
    /// @param colorSequence The base colors for the bitmap.
    /// @param colorMode The mode used to pick colors from the sequence.
    BitmapDrawOptions(ColorSequence colorSequence, BitmapColorMode colorMode = BitmapColorMode::OneColor);

    // defaults
    ~BitmapDrawOptions() = default;
    BitmapDrawOptions(const BitmapDrawOptions &) = default;
    BitmapDrawOptions(BitmapDrawOptions &&) noexcept = default;
    auto operator=(const BitmapDrawOptions &) -> BitmapDrawOptions & = default;
    auto operator=(BitmapDrawOptions &&) -> BitmapDrawOptions & = default;

public:
    /// The color to use for drawing the bitmap.
    /// The color can be either a single color or a sequence of colors.
    /// If this is an empty sequence, the color is inherited from the buffer.
    /// Colors are applied using the `colorMode()`.
    /// If the characters in `fullBlock()`, `doubleBlocks()` or `halfBlocks()` have colors set,
    /// these colors are overlaid *after* calculating this base color.
    /// @note For full-block and double-block mode, the background color is only applied to set pixels.
    ///       Fill the bitmap area if you need a custom background color for unset pixels.
    [[nodiscard]] auto color() const noexcept -> const ColorSequence &;
    /// Set a single color.
    /// Replaces the current color sequence with one entry.
    void setColor(Color color) noexcept;
    /// Set explicit foreground and background colors.
    void setColor(Foreground foreground, Background background) noexcept;
    /// Set a color sequence.
    /// Pass an empty `ColorSequence{}` to inherit the complete color from the buffer below.
    void setColorSequence(ColorSequence colorSequence, BitmapColorMode colorMode = BitmapColorMode::OneColor) noexcept;
    /// The color mode.
    /// This mode controls how colors are applied to the bitmap.
    /// See `BitmapColorMode` for more information.
    [[nodiscard]] auto colorMode() const noexcept -> BitmapColorMode;
    /// Set the color mode.
    void setColorMode(BitmapColorMode colorMode) noexcept;
    /// The offset for color animations.
    /// This offset is added to the `animationCycle` passed to `drawBitmap`.
    /// Animate `animationCycle` and keep this offset static.
    [[nodiscard]] auto colorAnimationOffset() const noexcept -> std::size_t;
    /// Set the offset for color animations.
    void setColorAnimationOffset(std::size_t offset) noexcept;
    /// The Block16Style instance.
    /// If a Block16Style instance is set, it *overrides* the scale mode and renders one terminal cell for each set
    /// bitmap pixel. The selected block depends on the four direct neighbors of the set pixel:
    /// east=bit 0, south=bit 1, west=bit 2, north=bit 3.
    [[nodiscard]] auto block16Style() const noexcept -> const Block16StylePtr &;
    /// Set a Block16Style instance.
    void setBlock16Style(Block16StylePtr block16Style) noexcept;
    /// The combination style.
    /// If a combination style is set, every block that is set in the buffer is first passed to
    /// this combination style. This happens for every mode used to draw the bitmap.
    [[nodiscard]] auto combinationStyle() const noexcept -> const BlockCombinationStylePtr &;
    /// Set the combination style.
    void setCombinationStyle(BlockCombinationStylePtr combinationStyle) noexcept;
    /// The full block.
    /// The full block is only used when the scale mode `FullBlock` is used and no `Block16Style` is set.
    /// Character colors are overlaid on the color from the color mode.
    [[nodiscard]] auto fullBlock() const noexcept -> const Block &;
    /// Set the full block.
    /// The full block must have a display width of 1.
    void setFullBlock(Block fullBlock);
    /// The double blocks.
    /// The double blocks are only used when the scale mode `DoubleBlock` is used and no `Block16Style` is set.
    /// Character index 0 is used for the left half and index 1 for the right half of each set bitmap pixel.
    /// Character colors are overlaid on the color from the color mode.
    [[nodiscard]] auto doubleBlocks() const noexcept -> const BlockString &;
    /// Set the double block BlockString.
    /// The string must have exactly two characters.
    void setDoubleBlocks(BlockString doubleBlocks);
    /// The string with the half-blocks.
    /// The half-blocks are only used when the scale mode `HalfBlock` is used and no `Block16Style` is set.
    /// Entry `0` is used for an empty 2x2 block and entry `15` for a full 2x2 block.
    /// Character colors are overlaid on the color from the color mode.
    [[nodiscard]] auto halfBlocks() const noexcept -> const BlockString &;
    /// Set the half-blocks string.
    /// The half-blocks string must have exactly 16 characters.
    void setHalfBlocks(BlockString halfBlocks);
    /// The scale mode.
    /// See `BitmapScaleMode` for more details.
    [[nodiscard]] auto scaleMode() const noexcept -> BitmapScaleMode;
    /// Set the scale mode.
    void setScaleMode(BitmapScaleMode scaleMode) noexcept;

public:
    /// Access the shared object with the default options.
    /// Default options use default terminal colors, half-block rendering, and the standard Unicode half-block
    /// characters.
    [[nodiscard]] static auto defaultOptions() noexcept -> const BitmapDrawOptions &;

private:
    ColorSequence _color = {Color{fg::Default, bg::Default}};
    BitmapColorMode _colorMode = BitmapColorMode::OneColor;
    std::size_t _colorAnimationOffset = 0;
    Block16StylePtr _block16Style;
    BlockCombinationStylePtr _combinationStyle;
    Block _fullBlock{text::Char{U'█'}};
    BlockString _doubleBlocks{text::U32String{text::U32StringLiteral{U"██"}}};
    BlockString _halfBlocks{text::U32String{text::U32StringLiteral{U" ▘▝▀▖▌▞▛▗▚▐▜▄▙▟█"}}};
    BitmapScaleMode _scaleMode = BitmapScaleMode::HalfBlock;
};

}
