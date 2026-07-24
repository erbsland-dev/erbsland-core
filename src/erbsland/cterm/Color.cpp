// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Color.hpp"

#include "../err/ParseError.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"

namespace erbsland::cterm {

using namespace text::literals;

auto Color::overlayWith(const Color &overlay) const -> Color {
    auto result = *this;
    if (overlay.fg() != Foreground::Inherited) {
        result.setFg(overlay.fg());
    }
    if (overlay.bg() != Background::Inherited) {
        result.setBg(overlay.bg());
    }
    return result;
}

auto Color::toString() const -> text::String {
    if (_background == Background::Inherited) {
        return _foreground.toString();
    }
    return text::String::fromJoined({_foreground.toString(), ":"_el, _background.toString()});
}

auto Color::fromString(const text::String &str, Color defaultValue) -> Color {
    try {
        return fromStringOrThrow(str);
    } catch (const err::ParseError &) {
        return defaultValue;
    }
}

auto Color::fromStringOrThrow(const text::String &str) -> Color {
    static const auto separatorCharacters = text::CharSet{U':'};
    if (const auto splitPos = str.findFirstOf(separatorCharacters); !splitPos.isNoIndex()) {
        const auto fgStr = str.slice(unit::ByteRange{unit::ByteIndex::zero(), splitPos});
        const auto bgStr = str.slice(unit::ByteRange{splitPos + unit::ByteLength::one(), unit::ByteLength::infinite()});
        if (!bgStr.findFirstOf(separatorCharacters).isNoIndex()) {
            throw err::ParseError{"A terminal color must have one or two fields."_el};
        }
        return {Foreground::fromStringOrThrow(fgStr), Background::fromStringOrThrow(bgStr)};
    }
    return {Foreground::fromStringOrThrow(str)};
}

}
