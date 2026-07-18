// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Color.hpp"

#include "../text/CharSet.hpp"

namespace erbsland::cterm {

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

auto Color::fromString(const text::String &str) -> Color {
    if (const auto splitPos = str.findFirstOf(text::CharSet{text::Char{U':'}}); !splitPos.isNoIndex()) {
        const auto fgStr = str.slice(unit::ByteRange{unit::ByteIndex::zero(), splitPos});
        const auto bgStr = str.slice(unit::ByteRange{splitPos + unit::ByteLength::one(), unit::ByteLength::infinite()});
        return {Foreground::fromString(fgStr), Background::fromString(bgStr)};
    }
    return {Foreground::fromString(str)};
}

}
