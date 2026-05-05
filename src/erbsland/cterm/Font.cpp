// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Font.hpp"

#include <algorithm>

namespace erbsland::cterm {

Font::Font(const int height) noexcept : _height{height} {
}

Font::Font(const int height, GlyphMap glyphs) noexcept : _height{height}, _glyphs{std::move(glyphs)} {
}

void Font::addGlyph(const text::StringView &name, FontGlyph glyph) {
    _glyphs[name.copy()] = std::move(glyph);
}

void Font::setHeight(const int height) noexcept {
    _height = height;
}

auto Font::height() const noexcept -> int {
    return _height;
}

auto Font::glyphs() const noexcept -> const GlyphMap & {
    return _glyphs;
}

auto Font::glyph(const text::StringView &name) const -> const FontGlyph * {
    if (const auto iterator = _glyphs.find(name.copy()); iterator != _glyphs.end()) {
        return &iterator->second;
    }
    return nullptr;
}

}
