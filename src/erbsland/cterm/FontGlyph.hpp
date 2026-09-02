// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Bitmap.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <vector>

namespace erbsland::cterm {

/// A bitmap glyph that can be used by a terminal text font.
class FontGlyph final : public Bitmap {
public:
    /// Maximum supported glyph width when importing numeric row masks.
    constexpr static auto cMaxGlyphWidth = 64;

public:
    /// Create an empty glyph.
    FontGlyph() = default;
    /// Create an empty glyph with the given size.
    /// @param size The glyph dimensions.
    explicit FontGlyph(const block::Size size) noexcept : Bitmap{size} {}
    /// Create a glyph from numeric row masks.
    /// @param data One 64-bit mask per bitmap row.
    template <typename T>
        requires std::is_integral_v<T> && std::is_unsigned_v<T>
    explicit FontGlyph(const std::vector<T> &data) :
        Bitmap{block::Size{calculateGlyphWidth(data), block::Coordinate{data.size()}}} {
        draw(block::Position{0, 0}, data);
        flipHorizontal();
    }

private:
    /// Calculate the required glyph width from bit-mask rows.
    template <typename T>
        requires std::is_integral_v<T> && std::is_unsigned_v<T>
    [[nodiscard]] constexpr static auto calculateGlyphWidth(const std::vector<T> &data) noexcept -> block::Coordinate {
        auto width = block::Coordinate{0};
        for (const auto &mask : data) {
            width = std::max(width, block::Coordinate{std::bit_width(mask)});
        }
        return width;
    }
};

}
