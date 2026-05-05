// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Properties.hpp"

#include "../BlockStyle.hpp"

#include "../../text/Char.hpp"
#include "../../text/u32/U32StringView.hpp"

namespace erbsland::cterm::theme {

/// Mutable editor for one property sheet while a theme is being built.
class PropertyEditor final {
public:
    /// Create an empty editor.
    PropertyEditor() = default;
    /// Create an editor for a property sheet.
    /// @param properties The edited properties. Must outlive this editor.
    explicit PropertyEditor(Properties &properties) noexcept : _properties{&properties} {}

public:
    /// Set the color replacement.
    auto setColor(Color color) noexcept -> PropertyEditor &;
    /// @overload
    auto setColor(Foreground::Hue fg) noexcept -> PropertyEditor &;
    /// @overload
    auto setColor(Background::Hue bg) noexcept -> PropertyEditor &;
    /// @overload
    auto setColor(Foreground::Hue fg, Background::Hue bg) noexcept -> PropertyEditor &;
    /// Set the color sequence.
    auto setColorSequence(ColorSequence colorSequence) noexcept -> PropertyEditor &;
    /// Set the "attributes" replacement.
    auto setAttributes(BlockAttributes attributes) noexcept -> PropertyEditor &;
    /// Set color and attributes in one call.
    auto setStyle(BlockStyle style) noexcept -> PropertyEditor &;
    /// Set one block code point.
    auto setBlock(BlockRole role, text::Char codePoint) noexcept -> PropertyEditor &;
    /// Set one block to inherit from its parent.
    auto setInheritedBlock(BlockRole role) noexcept -> PropertyEditor &;
    /// Set a 9-block or 16-block table from UTF-32 code points.
    auto setBlocks(const text::U32StringView &blocks) -> PropertyEditor &;
    /// Set all blocks to a given code point.
    auto setBlocks(text::Char codePoint) -> PropertyEditor &;
    /// Set all blocks to inherit from its parent.
    auto setInheritedBlocks() noexcept -> PropertyEditor &;
    /// Set all bracket blocks to a given code point.
    auto setBracketBlocks(text::Char left, text::Char right, text::Char middle = text::Char{U' '}) -> PropertyEditor &;
    /// Set the margins.
    /// bgeo::BlockMargins describe parent-owned space around a themed part. The part does not paint this area.
    /// bgeo::BlockMargins can never be negative. Negative margins are set to zero.
    auto setMargins(bgeo::BlockMargins margins) noexcept -> PropertyEditor &;
    /// Set the padding.
    /// Padding describes part-owned space inside the part. BlockText-focused UI elements paint this area with the
    /// part's background block and style before drawing the text content. Padding can never be negative. Negative
    /// padding is set to zero.
    auto setPadding(bgeo::BlockMargins padding) noexcept -> PropertyEditor &;

private:
    Properties *_properties{}; ///< The edited properties.
};

}
