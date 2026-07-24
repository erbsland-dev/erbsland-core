// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Block16Style.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::cterm {

using namespace text::literals;

Block16Style::Block16Style(const text::String &tiles) : _tiles(toTiles(BlockStringEditor{tiles})) {
}

Block16Style::Block16Style(const text::U32String &tiles) : _tiles(toTiles(BlockStringEditor{tiles})) {
}

auto Block16Style::block(const uint32_t bitMask) const noexcept -> Block {
    if (bitMask > 15) {
        return Block{};
    }
    return _tiles[static_cast<std::size_t>(bitMask)];
}

auto Block16Style::lightFrame() -> Block16StylePtr {
    static auto style = create("∙╶╷┌╴─┐┬╵└│├┘┴┤┼"_el);
    return style;
}

auto Block16Style::lightDoubleDashFrame() -> Block16StylePtr {
    static auto style = create("∙╶╷┌╴╌┐┬╵└╎├┘┴┤┼"_el);
    return style;
}

auto Block16Style::lightTripleDashFrame() -> Block16StylePtr {
    static auto style = create("∙╶╷┌╴┄┐┬╵└┆├┘┴┤┼"_el);
    return style;
}

auto Block16Style::lightQuadrupleDashFrame() -> Block16StylePtr {
    static auto style = create("∙╶╷┌╴┈┐┬╵└┊├┘┴┤┼"_el);
    return style;
}

auto Block16Style::lightRoundedFrame() -> Block16StylePtr {
    static auto style = create("∙╶╷╭╴─╮┬╵╰│├╯┴┤┼"_el);
    return style;
}

auto Block16Style::heavyFrame() -> Block16StylePtr {
    static auto style = create("▪╺╻┏╸━┓┳╹┗┃┣┛┻┫╋"_el);
    return style;
}

auto Block16Style::heavyDoubleDashFrame() -> Block16StylePtr {
    static auto style = create("▪╺╻┏╸╍┓┳╹┗╏┣┛┻┫╋"_el);
    return style;
}

auto Block16Style::heavyTripleDashFrame() -> Block16StylePtr {
    static auto style = create("▪╺╻┏╸┅┓┳╹┗┇┣┛┻┫╋"_el);
    return style;
}

auto Block16Style::heavyQuadrupleDashFrame() -> Block16StylePtr {
    static auto style = create("▪╺╻┏╸┉┓┳╹┗┋┣┛┻┫╋"_el);
    return style;
}

auto Block16Style::doubleFrame() -> Block16StylePtr {
    static auto style = create("▫╒╖╔╕═╗╦╜╚║╠╝╩╣╬"_el);
    return style;
}

auto Block16Style::fullBlockFrame() -> Block16StylePtr {
    static auto style = create(" ███████████████"_el);
    return style;
}

auto Block16Style::fullBlockWithChamferFrame() -> Block16StylePtr {
    static auto style = create(" ██◢██◣██◥██◤███"_el);
    return style;
}

auto Block16Style::noneFrame() -> Block16StylePtr {
    static auto style = create("                "_el);
    return style;
}

auto Block16Style::create(const text::String &tiles) -> Block16StylePtr {
    return std::make_shared<Block16Style>(tiles);
}

auto Block16Style::create(const text::U32String &tiles) -> Block16StylePtr {
    return std::make_shared<Block16Style>(tiles);
}

auto Block16Style::forStyle(const FrameStyle frameStyle) -> Block16StylePtr {
    switch (frameStyle) {
    case FrameStyle::Light:
        return lightFrame();
    case FrameStyle::LightDoubleDash:
        return lightDoubleDashFrame();
    case FrameStyle::LightTripleDash:
        return lightTripleDashFrame();
    case FrameStyle::LightQuadrupleDash:
        return lightQuadrupleDashFrame();
    case FrameStyle::LightWithRoundedCorners:
        return lightRoundedFrame();
    case FrameStyle::Heavy:
        return heavyFrame();
    case FrameStyle::HeavyDoubleDash:
        return heavyDoubleDashFrame();
    case FrameStyle::HeavyTripleDash:
        return heavyTripleDashFrame();
    case FrameStyle::HeavyQuadrupleDash:
        return heavyQuadrupleDashFrame();
    case FrameStyle::Double:
        return doubleFrame();
    case FrameStyle::FullBlock:
        return fullBlockFrame();
    case FrameStyle::FullBlockWithChamfer:
        return fullBlockWithChamferFrame();
    case FrameStyle::None:
        return noneFrame();
    case FrameStyle::OuterHalfBlock:
    case FrameStyle::InnerHalfBlock:
        return {};
    default:
        return lightFrame();
    }
}

auto Block16Style::toTiles(const BlockString &tiles) -> std::array<Block, 16> {
    if (tiles.length() != BlockCount{16U}) {
        throw err::ParameterError{"Block16Style requires exactly 16 terminal characters.", "tiles"};
    }
    auto result = std::array<Block, 16>{};
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index] = tiles[BlockIndex::fromSizeT(index)];
    }
    return result;
}

}
