// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Block.hpp"

#include "../text/StringCharReader.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::cterm {

auto Block::withCombining(const text::Char codePoint) const noexcept -> Block {
    auto result = *this;
    result._character = _character.withCombining(codePoint);
    return result;
}

auto Block::withOverlay(const BlockStyle style) const noexcept -> Block {
    auto result = *this;
    result._style = _style.withOverlay(style);
    return result;
}

auto Block::withColorReplaced(const Color color) const noexcept -> Block {
    auto result = *this;
    result._style.setColor(color);
    return result;
}

auto Block::withStyleReplaced(const BlockStyle style) const noexcept -> Block {
    auto result = *this;
    result._style = style;
    return result;
}

auto Block::withAttributes(const BlockAttributes attributes) const noexcept -> Block {
    auto result = *this;
    result._style.setAttributes(attributes);
    return result;
}

auto Block::withBase(const BlockStyle style) const noexcept -> Block {
    auto result = *this;
    result._style = _style.withBase(style);
    return result;
}

auto Block::withBase(const Block &base) const noexcept -> Block {
    auto result = *this;
    result._style = _style.withBase(base.style());
    return result;
}

auto Block::isSpacing() const noexcept -> bool {
    return _character.isSpacing();
}

auto Block::isControl() const noexcept -> bool {
    return _character.isControl();
}

auto Block::renderedEquals(const Block &other, const bool colorEnabled, const bool attributeEnabled) const noexcept
    -> bool {
    if (_character != other._character) {
        return false;
    }
    if (colorEnabled) {
        if (color().fg().ansiCode() != other.color().fg().ansiCode() ||
            color().bg().ansiCode() != other.color().bg().ansiCode()) {
            return false;
        }
    }
    if (attributeEnabled) {
        const auto resolvedAttributes = attributes().withBase(BlockAttributes::reset());
        const auto otherResolvedAttributes = other.attributes().withBase(BlockAttributes::reset());
        if (resolvedAttributes != otherResolvedAttributes) {
            return false;
        }
    }
    return true;
}

auto Block::space() noexcept -> const Block & {
    static const Block space{U' '};
    return space;
}

auto Block::empty() noexcept -> const Block & {
    static const Block empty{text::CombinedChar{}, {}};
    return empty;
}

auto Block::emptyBlock(const BlockStyle style) noexcept -> Block {
    return Block{text::CombinedChar{}, style};
}

}
