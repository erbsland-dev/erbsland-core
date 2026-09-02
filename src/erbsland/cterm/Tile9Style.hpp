// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockString.hpp"
#include "FrameStyle.hpp"
#include "Tile9Style_fwd.hpp"

#include "../block/Rectangle.hpp"
#include "../text/Char.hpp"
#include "../text/String.hpp"
#include "../text/u32/U32String.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cterm {

/// Defines a style for repeating a 3x3 tile pattern across a rectangle.
///
/// The 9-tile layout uses this arrangement:
/// top-left, top, top-right,
/// left, center, right,
/// bottom-left, bottom, bottom-right.
///
/// Optionally, 7 additional tiles can be provided for degenerate rectangles:
/// single-row left, single-row center, single-row right,
/// single-column top, single-column center, single-column bottom,
/// and the single-cell tile.
class Tile9Style {
public:
    /// Named elements of the 16-tile style table.
    enum class Element : uint8_t {
        NorthWest = 0,         ///< Top-left tile.
        North = 1,             ///< Top edge tile.
        NorthEast = 2,         ///< Top-right tile.
        West = 3,              ///< Left edge tile.
        Center = 4,            ///< Center tile.
        East = 5,              ///< Right edge tile.
        SouthWest = 6,         ///< Bottom-left tile.
        South = 7,             ///< Bottom edge tile.
        SouthEast = 8,         ///< Bottom-right tile.
        HorizontalWest = 9,    ///< Left tile for one-row rectangles.
        HorizontalCenter = 10, ///< Center tile for one-row rectangles.
        HorizontalEast = 11,   ///< Right tile for one-row rectangles.
        VerticalNorth = 12,    ///< Top tile for one-column rectangles.
        VerticalCenter = 13,   ///< Center tile for one-column rectangles.
        VerticalSouth = 14,    ///< Bottom tile for one-column rectangles.
        Single = 15,           ///< Single-cell tile.
    };

public:
    /// Create a new 9-tile style from the repeating 3x3 tile layout.
    /// @param tiles The 3x3 tiles in row-major order.
    explicit Tile9Style(std::array<Block, 9> tiles) noexcept;
    /// Create a new 9-tile style with explicit tiles for degenerate rectangles.
    /// @param tiles The 3x3 tiles followed by 7 degenerate tiles.
    explicit Tile9Style(std::array<Block, 16> tiles) noexcept;
    /// Create a new 9-tile style from code points and one shared style.
    /// @param tiles The 3x3 tiles followed by 7 degenerate tiles.
    /// @param style The shared style for all tiles.
    explicit Tile9Style(std::array<text::Char, 16> tiles, BlockStyle style) noexcept;
    /// Create a new 9-tile style from 9 or 16 terminal characters.
    /// @param tiles A sequence of 9 tiles, or 16 tiles including the degenerate cases.
    /// @throws err::ParameterError If `tiles` does not contain exactly 9 or 16 terminal characters.
    explicit Tile9Style(const text::String &tiles);
    /// Create a new 9-tile style from 9 or 16 terminal characters.
    /// @param tiles A sequence of 9 tiles, or 16 tiles including the degenerate cases.
    /// @throws err::ParameterError If `tiles` does not contain exactly 9 or 16 terminal characters.
    explicit Tile9Style(const text::U32String &tiles);

public: // accessors
    /// Resolve the tile for a given position inside a rectangle.
    /// The center and edge tiles are repeated as needed.
    /// @param rect The styled rectangle.
    /// @param pos A position inside `rect`.
    /// @return The resolved tile, or an empty character if `pos` is outside `rect`.
    [[nodiscard]] auto block(block::Rectangle rect, block::Position pos) const noexcept -> Block;
    /// Access one element of the 16-tile table by name.
    /// @param element The tile element to read.
    /// @return The configured tile. Extended-only elements fall back to their base-tile counterpart for 9-tile styles.
    [[nodiscard]] auto block(Element element) const noexcept -> Block;

public:
    /// Create a new shared style from 9 or 16 terminal characters.
    /// @param tiles A sequence of 9 tiles, or 16 tiles including the degenerate cases.
    /// @return A shared style instance.
    /// @throws err::ParameterError If `tiles` does not contain exactly 9 or 16 terminal characters.
    [[nodiscard]] static auto create(const text::String &tiles) -> Tile9StylePtr;
    /// Create a new shared style from 9 or 16 terminal characters.
    /// @param tiles A sequence of 9 tiles, or 16 tiles including the degenerate cases.
    /// @return A shared style instance.
    /// @throws err::ParameterError If `tiles` does not contain exactly 9 or 16 terminal characters.
    [[nodiscard]] static auto create(const text::U32String &tiles) -> Tile9StylePtr;
    /// For drawing half-block frames on the outer cell edges.
    [[nodiscard]] static auto outerHalfBlockFrame() -> Tile9StylePtr;
    /// For drawing half-block frames on the inner cell edges.
    [[nodiscard]] static auto innerHalfBlockFrame() -> Tile9StylePtr;
    /// Get the tile-9 style for a predefined frame style.
    /// @param frameStyle The frame style to resolve.
    /// @return The matching tile-9 style, or `nullptr` if the frame style uses `Block16Style`.
    [[nodiscard]] static auto forStyle(FrameStyle frameStyle) -> Tile9StylePtr;

private:
    /// Parsed 9- or 16-tile style data.
    struct ParsedTiles {
        std::array<Block, 16> tiles{};
        bool hasExtendedTiles = false;
    };

private:
    /// Create a style from validated parsed tiles.
    explicit Tile9Style(const ParsedTiles &parsed) noexcept;
    /// Parse a text sequence into style tiles.
    [[nodiscard]] static auto parseTiles(const BlockString &tiles) -> ParsedTiles;
    /// Expand nine base tiles into parsed style data.
    [[nodiscard]] static auto toParsedTiles(const std::array<Block, 9> &tiles) noexcept -> ParsedTiles;
    /// Store sixteen explicit tiles as parsed style data.
    [[nodiscard]] static auto toParsedTiles(const std::array<Block, 16> &tiles) noexcept -> ParsedTiles;

private:
    std::array<Block, 16> _tiles{};
    bool _hasExtendedTiles = false;
};

}
