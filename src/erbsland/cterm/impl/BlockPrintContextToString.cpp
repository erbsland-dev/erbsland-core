// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockPrintContextToString.hpp"

namespace erbsland::cterm::impl {

BlockPrintContextToString::BlockPrintContextToString(
    const BlockStyle style, const bool normalizeInheritedColorArguments) noexcept :
    _style{style}, _normalizeInheritedColorArguments{normalizeInheritedColorArguments} {
}

void BlockPrintContextToString::print(const Color color) noexcept {
    if (_normalizeInheritedColorArguments) {
        _style.setColor(Color::reset().overlayWith(color));
    } else {
        _style.setColor(color);
    }
}

void BlockPrintContextToString::print(const Foreground color) noexcept {
    _style.setFg(_normalizeInheritedColorArguments && color == Foreground::Inherited ? Foreground::Default : color);
}

void BlockPrintContextToString::print(const Background color) noexcept {
    _style.setBg(_normalizeInheritedColorArguments && color == Background::Inherited ? Background::Default : color);
}

void BlockPrintContextToString::print(const BlockStyle style) noexcept {
    _style = _style.withOverlay(style);
}

void BlockPrintContextToString::print(const BlockAttributes attributes) noexcept {
    _style.setAttributes(attributes.withBase(_style.attributes()));
}

void BlockPrintContextToString::print(const Block &character) noexcept {
    _builder.append(character.withBase(_style));
}

void BlockPrintContextToString::print(const BlockStringEditor &text) noexcept {
    print(BlockString{text});
}

void BlockPrintContextToString::print(const BlockString &text) noexcept {
    _builder.appendWithBaseStyle(text, _style);
}

void BlockPrintContextToString::print(const text::String &text) noexcept {
    _builder.appendStyled(text, _style);
}

void BlockPrintContextToString::print(const text::U32String &text) noexcept {
    _builder.appendStyled(text, _style);
}

}
