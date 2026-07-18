// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockPrintContextToTerminal.hpp"

namespace erbsland::cterm::impl {

void BlockPrintContextToTerminal::print(const Color color) noexcept {
    _terminal.setColor(color);
}

void BlockPrintContextToTerminal::print(const Foreground color) noexcept {
    _terminal.setForeground(color);
}

void BlockPrintContextToTerminal::print(const Background color) noexcept {
    _terminal.setBackground(color);
}

void BlockPrintContextToTerminal::print(const BlockStyle style) noexcept {
    _terminal.setStyle(_terminal.style().withOverlay(style));
}

void BlockPrintContextToTerminal::print(const BlockAttributes attributes) noexcept {
    _terminal.setBlockAttributes(attributes.withBase(_terminal.blockAttributes()));
}

void BlockPrintContextToTerminal::print(const Block &character) noexcept {
    _terminal.write(character);
}

void BlockPrintContextToTerminal::print(const BlockStringEditor &text) noexcept {
    print(BlockString{text});
}

void BlockPrintContextToTerminal::print(const BlockString &text) noexcept {
    _terminal.write(text);
}

void BlockPrintContextToTerminal::print(const text::String &text) noexcept {
    _terminal.write(text);
}

void BlockPrintContextToTerminal::print(const text::U32String &text) noexcept {
    _terminal.write(text);
}

}
