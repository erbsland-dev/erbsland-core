// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BitmapShowcaseApp.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace demo {
void BitmapShowcaseApp::drawPreviewPanel(const BlockRectangle rect, const el::String title, const Color fillColor) {

    if (rect.width() <= 4 || rect.height() <= 4) {
        return;
    }
    _buffer.drawFilledFrame(rect, FrameStyle::LightWithRoundedCorners, Block{U' ', fillColor});
    _buffer.drawBlockText(
        title,
        BlockRectangle{
            BlockCoordinate{rect.x1() + 2},
            BlockCoordinate{rect.y1()},
            BlockCoordinate{rect.width() - 4},
            BlockCoordinate{1}},
        Alignment::Center,
        Color{fg::BrightWhite, fillColor.bg()});
}

void BitmapShowcaseApp::drawFooter(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', bg::BrightBlack});
    auto footer = BlockText{footerText(), rect, Alignment::CenterLeft};
    _buffer.drawBlockText(footer);
}

auto BitmapShowcaseApp::footerText() const -> BlockString {
    auto result = BlockStringEditor{};
    result.append(
        fg::BrightCyan,
        "[←][→]"_el,
        fg::BrightWhite,
        " switch pages  "_el,
        fg::BrightYellow,
        "[↑][↓]"_el,
        fg::BrightWhite,
        " switch modes  "_el,
        fg::BrightGreen,
        "[Q]"_el,
        fg::BrightWhite,
        " quit  "_el,
        fg::BrightGreen,
        el::StringFormat{"page {}/4"_el}.build(_pageIndex + 1));
    return result;
}

auto BitmapShowcaseApp::pageTitle() const -> el::String {
    switch (_pageIndex) {
    case 0:
        return "scale modes"_el;
    case 1:
        return "color modes"_el;
    case 2:
        return "alignment and cropping"_el;
    case 3:
    default:
        return "custom blocks and combinations"_el;
    }
}

auto BitmapShowcaseApp::variantCount(const std::size_t pageIndex) const noexcept -> std::size_t {
    switch (pageIndex) {
    case 0:
        return 4;
    case 1:
        return 5;
    case 2:
        return 5;
    case 3:
    default:
        return 5;
    }
}

auto BitmapShowcaseApp::variantTitle(const std::size_t pageIndex, const std::size_t variantIndex) const -> el::String {

    switch (pageIndex) {
    case 0:
        switch (variantIndex) {
        case 0:
            return "HalfBlock"_el;
        case 1:
            return "FullBlock"_el;
        case 2:
            return "DoubleBlock"_el;
        case 3:
        default:
            return "Block16Style"_el;
        }
    case 1:
        switch (variantIndex) {
        case 0:
            return "OneColor"_el;
        case 1:
            return "VerticalStripes"_el;
        case 2:
            return "HorizontalStripes"_el;
        case 3:
            return "ForwardDiagonal"_el;
        case 4:
        default:
            return "BackwardDiagonal"_el;
        }
    case 2:
        switch (variantIndex) {
        case 0:
            return "TopLeft Crop"_el;
        case 1:
            return "Center Crop"_el;
        case 2:
            return "BottomRight Crop"_el;
        case 3:
            return "HalfBlock Crop"_el;
        case 4:
        default:
            return "DoubleBlock Crop"_el;
        }
    case 3:
    default:
        switch (variantIndex) {
        case 0:
            return "Custom Full Block"_el;
        case 1:
            return "Custom Double Blocks"_el;
        case 2:
            return "Custom Half Blocks"_el;
        case 3:
            return "Rounded Char16"_el;
        case 4:
        default:
            return "Combination Style"_el;
        }
    }
}

auto BitmapShowcaseApp::selectedVariantIndex() const noexcept -> std::size_t {
    return _selectedVariantByPage[_pageIndex];
}

void BitmapShowcaseApp::selectVariantDelta(const int delta) noexcept {
    const auto count = static_cast<int>(variantCount(_pageIndex));
    if (count <= 0) {
        return;
    }
    const auto current = static_cast<int>(_selectedVariantByPage[_pageIndex]);
    const auto next = (current + delta % count + count) % count;
    _selectedVariantByPage[_pageIndex] = static_cast<std::size_t>(next);
}

auto BitmapShowcaseApp::ringBitmap() -> const Bitmap & {
    static const auto bitmap = Bitmap::fromPattern({
        "..########.."_el,
        ".##......##."_el,
        "##........##"_el,
        "##........##"_el,
        "##........##"_el,
        "##........##"_el,
        ".##......##."_el,
        "..########.."_el,
    });
    return bitmap;
}

auto BitmapShowcaseApp::rocketBitmap() -> const Bitmap & {
    static const auto bitmap = Bitmap::fromPattern({
        "....##....."_el,
        "...#.#....."_el,
        "..#..#....."_el,
        ".#...#....."_el,
        "#....#....."_el,
        "######....."_el,
        ".....#....."_el,
        "###########"_el,
        "#.#.#.#.#.#"_el,
        ".#.#.#.#.#."_el,
        "..#######.."_el,
    });
    return bitmap;
}

auto BitmapShowcaseApp::waveBitmap() -> const Bitmap & {
    static const auto bitmap = Bitmap::fromPattern({
        "......####....."_el,
        "#...##....##..."_el,
        ".#.#...#.#..##."_el,
        "..#...........#"_el,
        ".#.#....#######"_el,
        "#...#......#..."_el,
        ".....######...."_el,
    });
    return bitmap;
}

auto BitmapShowcaseApp::circuitBitmap() -> const Bitmap & {
    static const auto bitmap = Bitmap::fromPattern({
        "#...#####...."_el,
        "#...#...#...."_el,
        "#############"_el,
        "...#.....#..#"_el,
        "...#######..#."_el,
        "....#...#...."_el,
        "..###...###.."_el,
    });
    return bitmap;
}

auto BitmapShowcaseApp::rainbowColors() -> const ColorSequence & {
    static const auto colors = ColorSequence{
        {Color{fg::BrightBlue, bg::Black}, 3},
        {Color{fg::BrightCyan, bg::Black}, 3},
        {Color{fg::BrightGreen, bg::Black}, 3},
        {Color{fg::BrightYellow, bg::Black}, 3},
        {Color{fg::BrightRed, bg::Black}, 3},
        {Color{fg::BrightMagenta, bg::Black}, 3},
    };
    return colors;
}

}
