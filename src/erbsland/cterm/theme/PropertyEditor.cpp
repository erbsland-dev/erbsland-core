// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PropertyEditor.hpp"

#include <stdexcept>
#include <utility>

namespace erbsland::cterm::theme {

auto PropertyEditor::setColor(const Color color) noexcept -> PropertyEditor & {
    _properties->setColor(color);
    return *this;
}

auto PropertyEditor::setColor(const Foreground::Hue fg) noexcept -> PropertyEditor & {
    _properties->setColor(Color{fg});
    return *this;
}

auto PropertyEditor::setColor(const Background::Hue bg) noexcept -> PropertyEditor & {
    _properties->setColor(Color{bg});
    return *this;
}

auto PropertyEditor::setColor(const Foreground::Hue fg, const Background::Hue bg) noexcept -> PropertyEditor & {
    _properties->setColor(Color{fg, bg});
    return *this;
}

auto PropertyEditor::setColorSequence(ColorSequence colorSequence) noexcept -> PropertyEditor & {
    _properties->setColorSequence(std::move(colorSequence));
    return *this;
}

auto PropertyEditor::setAttributes(const BlockAttributes attributes) noexcept -> PropertyEditor & {
    _properties->setAttributes(attributes);
    return *this;
}

auto PropertyEditor::setStyle(const BlockStyle style) noexcept -> PropertyEditor & {
    _properties->setColor(style.color());
    _properties->setAttributes(style.attributes());
    return *this;
}

auto PropertyEditor::setBlock(const BlockRole role, const text::Char codePoint) noexcept -> PropertyEditor & {
    _properties->setBlock(role, codePoint);
    return *this;
}

auto PropertyEditor::setInheritedBlock(BlockRole role) noexcept -> PropertyEditor & {
    setBlock(role, Properties::cInheritedBlock);
    return *this;
}

auto PropertyEditor::setBlocks(const text::U32StringView &blocks) -> PropertyEditor & {
    const auto blockCount = blocks.length().toSizeT();
    if (blockCount != 9 && blockCount != 16) {
        throw std::invalid_argument{"Theme block tables require exactly 9 or 16 code points."};
    }
    auto blockTable = Properties::Blocks{};
    blockTable.fill(Properties::cInheritedBlock);
    auto index = std::size_t{0};
    for (const auto character : blocks) {
        blockTable[index] = character;
        index += 1;
    }
    if (blockCount == 9) {
        blockTable[static_cast<std::size_t>(BlockRole::HorizontalWest)] = blockTable[0];
        blockTable[static_cast<std::size_t>(BlockRole::HorizontalCenter)] = blockTable[1];
        blockTable[static_cast<std::size_t>(BlockRole::HorizontalEast)] = blockTable[2];
        blockTable[static_cast<std::size_t>(BlockRole::VerticalNorth)] = blockTable[0];
        blockTable[static_cast<std::size_t>(BlockRole::VerticalCenter)] = blockTable[3];
        blockTable[static_cast<std::size_t>(BlockRole::VerticalSouth)] = blockTable[6];
        blockTable[static_cast<std::size_t>(BlockRole::Single)] = blockTable[0];
    }
    _properties->setBlocks(blockTable);
    return *this;
}

auto PropertyEditor::setBlocks(const text::Char codePoint) -> PropertyEditor & {
    auto blockTable = Properties::Blocks{};
    blockTable.fill(codePoint);
    _properties->setBlocks(blockTable);
    return *this;
}

auto PropertyEditor::setInheritedBlocks() noexcept -> PropertyEditor & {
    setBlocks(Properties::cInheritedBlock);
    return *this;
}

auto PropertyEditor::setBracketBlocks(const text::Char left, const text::Char right, const text::Char middle)
    -> PropertyEditor & {
    _properties->setBlock(BlockRole::LeftBracket, left);
    _properties->setBlock(BlockRole::RightBracket, right);
    _properties->setBlock(BlockRole::MiddleBracket, middle);
    return *this;
}

auto PropertyEditor::setMargins(const bgeo::BlockMargins margins) noexcept -> PropertyEditor & {
    _properties->setMargins(margins);
    return *this;
}

auto PropertyEditor::setPadding(const bgeo::BlockMargins padding) noexcept -> PropertyEditor & {
    _properties->setPadding(padding);
    return *this;
}

}
