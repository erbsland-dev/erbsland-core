// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tile9Style.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <algorithm>

namespace erbsland::cterm {

using namespace text::literals;

Tile9Style::Tile9Style(const std::array<Block, 9> tiles) noexcept : Tile9Style(toParsedTiles(tiles)) {
}

Tile9Style::Tile9Style(const std::array<Block, 16> tiles) noexcept : Tile9Style(toParsedTiles(tiles)) {
}

Tile9Style::Tile9Style(const std::array<text::Char, 16> tiles, const BlockStyle style) noexcept {
    for (auto index = std::size_t{0}; index < tiles.size(); ++index) {
        _tiles[index] = Block{tiles[index], style};
    }
    _hasExtendedTiles = true;
}

Tile9Style::Tile9Style(const text::String &tiles) : Tile9Style(parseTiles(BlockStringEditor{tiles})) {
}

Tile9Style::Tile9Style(const text::U32String &tiles) : Tile9Style(parseTiles(BlockStringEditor{tiles})) {
}

auto Tile9Style::block(const bgeo::BlockRectangle rect, const bgeo::BlockPosition pos) const noexcept -> Block {
    if (!rect.contains(pos)) {
        return Block{};
    }
    if (rect.width() == 1 && rect.height() == 1) {
        return _hasExtendedTiles ? _tiles[15] : _tiles[0];
    }
    if (rect.height() == 1) {
        if (_hasExtendedTiles) {
            if (pos.x() == rect.x1()) {
                return _tiles[9];
            }
            if (pos.x() == rect.x2() - 1) {
                return _tiles[11];
            }
            return _tiles[10];
        }
        if (pos.x() == rect.x1()) {
            return _tiles[0];
        }
        if (pos.x() == rect.x2() - 1) {
            return _tiles[2];
        }
        return _tiles[1];
    }
    if (rect.width() == 1) {
        if (_hasExtendedTiles) {
            if (pos.y() == rect.y1()) {
                return _tiles[12];
            }
            if (pos.y() == rect.y2() - 1) {
                return _tiles[14];
            }
            return _tiles[13];
        }
        if (pos.y() == rect.y1()) {
            return _tiles[0];
        }
        if (pos.y() == rect.y2() - 1) {
            return _tiles[6];
        }
        return _tiles[3];
    }
    const auto row = pos.y() == rect.y1() ? 0 : pos.y() == rect.y2() - 1 ? 2 : 1;
    const auto column = pos.x() == rect.x1() ? 0 : pos.x() == rect.x2() - 1 ? 2 : 1;
    return _tiles[static_cast<std::size_t>(row * 3 + column)];
}

auto Tile9Style::block(const Element element) const noexcept -> Block {
    const auto index = static_cast<std::size_t>(element);
    if (_hasExtendedTiles || index < 9) {
        return _tiles[index];
    }
    switch (element) {
    case Element::HorizontalWest:
    case Element::VerticalNorth:
    case Element::Single:
        return _tiles[0];
    case Element::HorizontalCenter:
        return _tiles[1];
    case Element::HorizontalEast:
        return _tiles[2];
    case Element::VerticalCenter:
        return _tiles[3];
    case Element::VerticalSouth:
        return _tiles[6];
    default:
        return {};
    }
}

auto Tile9Style::create(const text::String &tiles) -> Tile9StylePtr {
    return std::make_shared<Tile9Style>(tiles);
}

auto Tile9Style::create(const text::U32String &tiles) -> Tile9StylePtr {
    return std::make_shared<Tile9Style>(tiles);
}

auto Tile9Style::outerHalfBlockFrame() -> Tile9StylePtr {
    static auto style = create("▛▀▜▌ ▐▙▄▟███████"_el);
    return style;
}

auto Tile9Style::innerHalfBlockFrame() -> Tile9StylePtr {
    static auto style = create("▗▄▖▐ ▌▝▀▘███████"_el);
    return style;
}

auto Tile9Style::forStyle(const FrameStyle frameStyle) -> Tile9StylePtr {
    switch (frameStyle) {
    case FrameStyle::OuterHalfBlock:
        return outerHalfBlockFrame();
    case FrameStyle::InnerHalfBlock:
        return innerHalfBlockFrame();
    default:
        return {};
    }
}

Tile9Style::Tile9Style(const ParsedTiles &parsed) noexcept :
    _tiles{parsed.tiles}, _hasExtendedTiles{parsed.hasExtendedTiles} {
}

auto Tile9Style::parseTiles(const BlockString &tiles) -> ParsedTiles {
    if (tiles.length() != BlockCount{9U} && tiles.length() != BlockCount{16U}) {
        throw err::ParameterError{"Tile9Style requires exactly 9 or 16 terminal characters."_el, "tiles"_el};
    }
    auto result = ParsedTiles{};
    result.hasExtendedTiles = tiles.length() == BlockCount{16U};
    for (std::size_t index = 0; index < tiles.length().toSizeT(); ++index) {
        result.tiles[index] = tiles[BlockIndex::fromSizeT(index)];
    }
    return result;
}

auto Tile9Style::toParsedTiles(const std::array<Block, 9> &tiles) noexcept -> ParsedTiles {
    auto result = ParsedTiles{};
    std::copy_n(tiles.begin(), tiles.size(), result.tiles.begin());
    return result;
}

auto Tile9Style::toParsedTiles(const std::array<Block, 16> &tiles) noexcept -> ParsedTiles {
    return ParsedTiles{.tiles = tiles, .hasExtendedTiles = true};
}

}
