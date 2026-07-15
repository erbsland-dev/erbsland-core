// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BitmapDrawOptions.hpp"

#include "../err/ParameterError.hpp"

namespace erbsland::cterm {

BitmapDrawOptions::BitmapDrawOptions(ColorSequence colorSequence, BitmapColorMode colorMode) :
    _color{colorSequence}, _colorMode{colorMode} {
}

auto BitmapDrawOptions::color() const noexcept -> const ColorSequence & {
    return _color;
}

auto BitmapDrawOptions::colorMode() const noexcept -> BitmapColorMode {
    return _colorMode;
}

void BitmapDrawOptions::setColorMode(BitmapColorMode colorMode) noexcept {
    _colorMode = colorMode;
}

auto BitmapDrawOptions::colorAnimationOffset() const noexcept -> std::size_t {
    return _colorAnimationOffset;
}

auto BitmapDrawOptions::combinationStyle() const noexcept -> const BlockCombinationStylePtr & {
    return _combinationStyle;
}

void BitmapDrawOptions::setColor(Color color) noexcept {
    _color = {color};
}

void BitmapDrawOptions::setColor(Foreground foreground, Background background) noexcept {
    _color = {Color{foreground, background}};
}

void BitmapDrawOptions::setColorSequence(ColorSequence colorSequence, BitmapColorMode colorMode) noexcept {
    _color = colorSequence;
    _colorMode = colorMode;
}

void BitmapDrawOptions::setColorAnimationOffset(std::size_t offset) noexcept {
    _colorAnimationOffset = offset;
}

auto BitmapDrawOptions::block16Style() const noexcept -> const Block16StylePtr & {
    return _block16Style;
}

void BitmapDrawOptions::setBlock16Style(Block16StylePtr block16Style) noexcept {
    _block16Style = block16Style;
}

void BitmapDrawOptions::setCombinationStyle(BlockCombinationStylePtr combinationStyle) noexcept {
    _combinationStyle = combinationStyle;
}

auto BitmapDrawOptions::fullBlock() const noexcept -> const Block & {
    return _fullBlock;
}

void BitmapDrawOptions::setFullBlock(Block fullBlock) {
    if (fullBlock.displayWidth() != 1) {
        throw err::ParameterError{"Full block character must have display width of 1.", "fullBlock"};
    }
    _fullBlock = fullBlock;
}

auto BitmapDrawOptions::doubleBlocks() const noexcept -> const BlockString & {
    return _doubleBlocks;
}

void BitmapDrawOptions::setDoubleBlocks(BlockString doubleBlocks) {
    if (doubleBlocks.length() != BlockCount{2U}) {
        throw err::ParameterError{"Double blocks string must contain exactly 2 characters.", "doubleBlocks"};
    }
    for (const auto &character : doubleBlocks) {
        if (character.displayWidth() != 1) {
            throw err::ParameterError{
                "Double blocks string must contain characters with display width of 1.", "doubleBlocks"};
        }
    }
    _doubleBlocks = std::move(doubleBlocks);
}

auto BitmapDrawOptions::halfBlocks() const noexcept -> const BlockString & {
    return _halfBlocks;
}

void BitmapDrawOptions::setHalfBlocks(BlockString halfBlocks) {
    if (halfBlocks.length() != BlockCount{16U}) {
        throw err::ParameterError{"Half blocks string must contain exactly 16 characters.", "halfBlocks"};
    }
    for (const auto &character : halfBlocks) {
        if (character.displayWidth() != 1) {
            throw err::ParameterError{
                "Half blocks string must contain characters with display width of 1.", "halfBlocks"};
        }
    }
    _halfBlocks = std::move(halfBlocks);
}

auto BitmapDrawOptions::scaleMode() const noexcept -> BitmapScaleMode {
    return _scaleMode;
}

void BitmapDrawOptions::setScaleMode(BitmapScaleMode scaleMode) noexcept {
    _scaleMode = scaleMode;
}

auto BitmapDrawOptions::defaultOptions() noexcept -> const BitmapDrawOptions & {
    static const BitmapDrawOptions options;
    return options;
}

}
