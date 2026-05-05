// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FontGlyph.hpp"

#include "../text/String.hpp"
#include "../text/StringView.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

namespace erbsland::cterm {

class Font;
/// Shared pointer to a terminal font.
using FontPtr = std::shared_ptr<Font>;

/// A bitmap font used to render stylized terminal text.
class Font final {
public: // public types
    /// Mapping from UTF-8 character strings to glyph bitmaps.
    using GlyphMap = std::unordered_map<text::String, FontGlyph>;

public: // ctors/dtor/assign/move
    /// Create an empty font with height `0`.
    Font() = default;
    /// Create an empty font with the given glyph height.
    /// @param height The glyph height in bitmap rows.
    explicit Font(int height) noexcept;
    /// Create a font with the given glyph height and glyph map.
    /// @param height The glyph height in bitmap rows.
    /// @param glyphs The initial glyph map.
    Font(int height, GlyphMap glyphs) noexcept;

public: // modifiers
    /// Add or replace one glyph in this font.
    /// @param name The UTF-8 encoded character represented by the glyph.
    /// @param glyph The bitmap glyph.
    void addGlyph(const text::StringView &name, FontGlyph glyph);
    /// Set the configured font height in bitmap rows.
    /// @param height The new glyph height.
    void setHeight(int height) noexcept;

public: // accessors
    /// Get the configured font height in bitmap rows.
    [[nodiscard]] auto height() const noexcept -> int;
    /// Access the configured glyph map.
    [[nodiscard]] auto glyphs() const noexcept -> const GlyphMap &;
    /// Find one glyph by its UTF-8 character string.
    /// @param name The UTF-8 encoded character to look up.
    /// @return A pointer to the glyph, or `nullptr` if no glyph exists for `name`.
    [[nodiscard]] auto glyph(const text::StringView &name) const -> const FontGlyph *;
    /// Create a new font instance with the built-in ASCII-centric default glyph set.
    /// @return A new shared font instance containing the default Latin letters, punctuation, and spacing glyphs.
    [[nodiscard]] static auto defaultAscii() -> FontPtr;

private:
    int _height{};
    GlyphMap _glyphs;
};

}
