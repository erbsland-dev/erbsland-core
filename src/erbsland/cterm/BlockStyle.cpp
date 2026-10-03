// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStyle.hpp"

#include "../err/ParseError.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"
#include "../text/StringJoins.hpp"

namespace erbsland::cterm {

using namespace text;
using namespace text::literals;

auto BlockStyle::toString() const -> String {
    if (_attributes == BlockAttributes{}) {
        return _color.toString();
    }
    return text::join(_color.fg().toString(), ":"_el, _color.bg().toString(), ":"_el, _attributes.toString());
}

auto BlockStyle::fromString(const String &str, BlockStyle defaultValue) -> BlockStyle {
    try {
        return fromStringOrThrow(str);
    } catch (const err::ParseError &) {
        return defaultValue;
    }
}

auto BlockStyle::fromStringOrThrow(const String &str) -> BlockStyle {
    static const auto separatorCharacters = CharSet{U':'};
    const auto first = str.splitAtFirst(separatorCharacters);
    if (!first) {
        return BlockStyle{Foreground::fromStringOrThrow(str)};
    }
    const auto second = first->second.splitAtFirst(separatorCharacters);
    if (!second) {
        return BlockStyle{Color::fromStringOrThrow(str)};
    }
    const auto &attributesText = second->second;
    if (!attributesText.findFirstOf(separatorCharacters).isNoIndex()) {
        throw err::ParseError{"A block style must have one, two, or three fields."_el};
    }
    const auto &foregroundText = first->first;
    const auto &backgroundText = second->first;
    return BlockStyle{
        Color{Foreground::fromStringOrThrow(foregroundText), Background::fromStringOrThrow(backgroundText)},
        BlockAttributes::fromStringOrThrow(attributesText)};
}

auto BlockStyle::withOverlay(const BlockStyle overlay) const noexcept -> BlockStyle {
    return BlockStyle{_color.overlayWith(overlay._color), overlay._attributes.withBase(_attributes)};
}

auto BlockStyle::withBase(const BlockStyle base) const noexcept -> BlockStyle {
    return BlockStyle{base._color.overlayWith(_color), _attributes.withBase(base._attributes)};
}

}
