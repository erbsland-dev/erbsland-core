// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStyle.hpp"

#include "../err/ParseError.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"

namespace erbsland::cterm {

using namespace text::literals;

auto BlockStyle::toString() const -> text::String {
    if (_attributes == BlockAttributes{}) {
        return _color.toString();
    }
    return text::String::fromJoined(
        {_color.fg().toString(), ":"_el, _color.bg().toString(), ":"_el, _attributes.toString()});
}

auto BlockStyle::fromString(const text::String &str, BlockStyle defaultValue) -> BlockStyle {
    try {
        return fromStringOrThrow(str);
    } catch (const err::ParseError &) {
        return defaultValue;
    }
}

auto BlockStyle::fromStringOrThrow(const text::String &str) -> BlockStyle {
    static const auto separatorCharacters = text::CharSet{U':'};
    const auto firstSeparator = str.findFirstOf(separatorCharacters);
    if (firstSeparator.isNoIndex()) {
        return BlockStyle{Foreground::fromStringOrThrow(str)};
    }
    const auto secondSeparator = str.findFirstOf(separatorCharacters, firstSeparator + unit::ByteLength::one());
    if (secondSeparator.isNoIndex()) {
        return BlockStyle{Color::fromStringOrThrow(str)};
    }
    const auto attributesText =
        str.slice(unit::ByteRange{secondSeparator + unit::ByteLength::one(), unit::ByteLength::infinite()});
    if (!attributesText.findFirstOf(separatorCharacters).isNoIndex()) {
        throw err::ParseError{"A block style must have one, two, or three fields."_el};
    }
    const auto foregroundText = str.slice(unit::ByteRange{unit::ByteIndex::zero(), firstSeparator});
    const auto backgroundText = str.slice(
        unit::ByteRange{
            firstSeparator + unit::ByteLength::one(),
            firstSeparator.absoluteDistanceTo(secondSeparator) - unit::ByteLength::one()});
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
