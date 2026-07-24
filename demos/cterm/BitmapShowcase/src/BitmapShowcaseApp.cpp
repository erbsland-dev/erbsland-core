// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BitmapShowcaseApp.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace demo {

void BitmapShowcaseApp::beforeInitialize() {
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{68}, BlockCoordinate{20}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 68x20 cells for the bitmap showcase."_el,
            Color{fg::BrightWhite, bg::Black}});
}

void BitmapShowcaseApp::onKey(const Key &key) {
    if (key == Key::Left) {
        _pageIndex = (_pageIndex + 3) % 4;
    } else if (key == Key::Right) {
        _pageIndex = (_pageIndex + 1) % 4;
    } else if (key == Key::Up) {
        selectVariantDelta(-1);
    } else if (key == Key::Down) {
        selectVariantDelta(1);
    } else {
        TerminalApplication::onKey(key);
    }
}

void BitmapShowcaseApp::onRenderToBuffer() {
    _buffer.fill(Block{U' ', bg::Black});
    const auto outerRect = BlockRectangle{
        BlockCoordinate{0},
        BlockCoordinate{0},
        BlockCoordinate{_buffer.size().width()},
        BlockCoordinate{_buffer.size().height()}};
    const auto titleRect = BlockRectangle{
        BlockCoordinate{2}, BlockCoordinate{1}, BlockCoordinate{_buffer.size().width() - 4}, BlockCoordinate{1}};
    const auto contentRect = BlockRectangle{
        BlockCoordinate{2},
        BlockCoordinate{3},
        BlockCoordinate{_buffer.size().width() - 4},
        BlockCoordinate{_buffer.size().height() - 7}};
    const auto footerRect = BlockRectangle{
        BlockCoordinate{2},
        BlockCoordinate{_buffer.size().height() - 3},
        BlockCoordinate{_buffer.size().width() - 4},
        BlockCoordinate{1}};
    auto selectorWidth = std::clamp(contentRect.width() / 3, BlockCoordinate{22}, BlockCoordinate{30});
    if (contentRect.width() - selectorWidth - 2 < 36) {
        selectorWidth = std::max(BlockCoordinate{18}, contentRect.width() / 4);
    }
    const auto selectorRect = BlockRectangle{
        BlockCoordinate{contentRect.x1()},
        BlockCoordinate{contentRect.y1()},
        BlockCoordinate{selectorWidth},
        BlockCoordinate{contentRect.height()}};
    const auto previewRect = BlockRectangle{
        selectorRect.x2() + 2, contentRect.y1(), contentRect.x2() - selectorRect.x2() - 2, contentRect.height()};

    _buffer.drawFrame(outerRect, FrameStyle::LightWithRoundedCorners);
    _buffer.drawBlockText(
        el::StringFormat{"Bitmap Showcase  |  {}"_el}.build(pageTitle()),
        titleRect,
        Alignment::Center,
        Color{fg::BrightWhite, bg::Black});
    drawSelector(selectorRect);
    drawPreview(previewRect);
    drawFooter(footerRect);
}

void BitmapShowcaseApp::drawSelector(const BlockRectangle rect) {
    const auto count = static_cast<int>(variantCount(_pageIndex));
    if (count <= 0 || rect.height() <= 2) {
        return;
    }
    const auto selected = static_cast<int>(selectedVariantIndex());
    const auto rowStride = rect.height() >= count * 2 + 2 ? BlockCoordinate{2} : BlockCoordinate{1};
    const auto visibleCount =
        static_cast<int>(std::max(BlockCoordinate{1}, (rect.height() - 1) / rowStride).toRawValue());
    auto firstVisible = std::max(0, selected - visibleCount / 2);
    firstVisible = std::min(firstVisible, std::max(0, count - visibleCount));

    for (auto visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
        const auto itemIndex = firstVisible + visibleIndex;
        if (itemIndex >= count) {
            break;
        }
        const auto itemIndexAsSize = static_cast<std::size_t>(itemIndex);
        const auto y = rect.y1() + visibleIndex * rowStride;
        const auto lineRect = BlockRectangle{
            BlockCoordinate{rect.x1()}, BlockCoordinate{y}, BlockCoordinate{rect.width()}, BlockCoordinate{1}};
        const auto isSelected = itemIndex == selected;
        if (isSelected) {
            _buffer.fill(lineRect, Block{U' ', bg::BrightBlack});
        }
        auto label = BlockStringEditor{};
        if (isSelected) {
            label.append(fg::BrightYellow, "▶ "_el, fg::BrightWhite, variantTitle(_pageIndex, itemIndexAsSize));
        } else {
            label.append(fg::BrightBlack, "  "_el, fg::White, variantTitle(_pageIndex, itemIndexAsSize));
        }
        _buffer.drawBlockText(BlockText{label, lineRect, Alignment::CenterLeft});
    }
}

void BitmapShowcaseApp::drawPreview(const BlockRectangle rect) {
    switch (_pageIndex) {
    case 0:
        drawScaleModeVariant(rect, selectedVariantIndex());
        break;
    case 1:
        drawColorModeVariant(rect, selectedVariantIndex());
        break;
    case 2:
        drawLayoutVariant(rect, selectedVariantIndex());
        break;
    case 3:
    default:
        drawStyleVariant(rect, selectedVariantIndex());
        break;
    }
}

void BitmapShowcaseApp::drawScaleModeVariant(const BlockRectangle rect, const std::size_t variantIndex) {

    static constexpr auto cPanelColors = std::array<Color, 4>{
        Color{fg::BrightWhite, bg::BrightBlack},
        Color{fg::BrightWhite, bg::Blue},
        Color{fg::BrightWhite, bg::Magenta},
        Color{fg::BrightWhite, bg::Green},
    };
    static const auto cDescriptions = std::array<el::String, 4>{
        "2x2 pixels are packed into one terminal cell. Even the empty table entry is drawn, so the bitmap can paint a "
        "background."_el,
        "One full block is emitted for each set pixel. Unset pixels are left untouched, which keeps the panel fill "
        "visible below."_el,
        "Each set pixel is stretched to two terminal cells, which usually looks more natural on regular terminal "
        "character grids."_el,
        "A Block16Style ignores the scale mode and derives a block from each set pixel plus its four direct neighbors."_el,
    };

    const auto panelColor = cPanelColors[std::min(variantIndex, cPanelColors.size() - 1)];
    drawPreviewPanel(rect, variantTitle(_pageIndex, variantIndex), panelColor);
    const auto demoRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 3},
        BlockCoordinate{rect.y1() + 2},
        BlockCoordinate{rect.width() - 6},
        BlockCoordinate{rect.height() - 7}};
    const auto descriptionRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 2},
        BlockCoordinate{rect.y2() - 3},
        BlockCoordinate{rect.width() - 4},
        BlockCoordinate{2}};

    switch (variantIndex) {
    case 0: {
        auto options = BitmapDrawOptions{Color{fg::BrightCyan, bg::Blue}};
        _buffer.drawBitmap(ringBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 1: {
        auto options = BitmapDrawOptions{Color{fg::BrightYellow, bg::Blue}};
        options.setScaleMode(BitmapScaleMode::FullBlock);
        _buffer.drawBitmap(rocketBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 2: {
        auto options = BitmapDrawOptions{Color{fg::BrightWhite, bg::Magenta}};
        options.setScaleMode(BitmapScaleMode::DoubleBlock);
        _buffer.drawBitmap(waveBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 3:
    default: {
        auto options = BitmapDrawOptions{Color{fg::BrightWhite, bg::Green}};
        options.setBlock16Style(Block16Style::lightRoundedFrame());
        _buffer.drawBitmap(circuitBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    }
    _buffer.drawBlockText(
        cDescriptions[std::min(variantIndex, cDescriptions.size() - 1)], descriptionRect, Alignment::Center);
}

void BitmapShowcaseApp::drawColorModeVariant(const BlockRectangle rect, const std::size_t variantIndex) {

    static constexpr auto cModes = std::array<BitmapColorMode, 5>{
        BitmapColorMode::OneColor,
        BitmapColorMode::VerticalStripes,
        BitmapColorMode::HorizontalStripes,
        BitmapColorMode::ForwardDiagonalStripes,
        BitmapColorMode::BackwardDiagonalStripes,
    };
    static const auto cDescriptions = std::array<el::String, 5>{
        "OneColor locks the whole bitmap to the same entry in the ColorSequence."_el,
        "Vertical stripes advance the sequence with the rendered x position."_el,
        "Horizontal stripes advance the sequence with the rendered y position."_el,
        "Forward diagonals use x + y, which makes the palette drift from top-left to bottom-right."_el,
        "Backward diagonals use -x + y, which makes the palette drift in the opposite direction."_el,
    };

    drawPreviewPanel(rect, variantTitle(_pageIndex, variantIndex), Color{fg::BrightWhite, bg::BrightBlack});
    const auto demoRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 3},
        BlockCoordinate{rect.y1() + 2},
        BlockCoordinate{rect.width() - 6},
        BlockCoordinate{rect.height() - 7}};
    const auto descriptionRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 2},
        BlockCoordinate{rect.y2() - 3},
        BlockCoordinate{rect.width() - 4},
        BlockCoordinate{2}};

    auto options = BitmapDrawOptions{};
    options.setColorSequence(rainbowColors(), cModes[std::min(variantIndex, cModes.size() - 1)]);
    options.setScaleMode(BitmapScaleMode::DoubleBlock);
    options.setColorAnimationOffset(variantIndex * 2);
    _buffer.drawBitmap(waveBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
    _buffer.drawBlockText(
        cDescriptions[std::min(variantIndex, cDescriptions.size() - 1)], descriptionRect, Alignment::Center);
}

void BitmapShowcaseApp::drawLayoutVariant(const BlockRectangle rect, const std::size_t variantIndex) {

    static constexpr auto cAlignments = std::array<Alignment, 5>{
        Alignment::TopLeft,
        Alignment::Center,
        Alignment::BottomRight,
        Alignment::Center,
        Alignment::Center,
    };
    static const auto cDescriptions = std::array<el::String, 5>{
        "The viewport keeps the top-left part of the rendered bitmap when it must crop."_el,
        "Center alignment crops evenly around the preview whenever the rendered bitmap is larger than the viewport."_el,
        "Bottom-right alignment keeps the last visible part of the rendered bitmap."_el,
        "HalfBlock layout crops the rendered cells, not the original pixels."_el,
        "A centered viewport keeps the bitmap readable on narrower screens."_el,
    };

    drawPreviewPanel(rect, variantTitle(_pageIndex, variantIndex), Color{fg::BrightWhite, bg::BrightBlack});
    const auto frameRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 6},
        BlockCoordinate{rect.y1() + 3},
        BlockCoordinate{rect.width() - 12},
        BlockCoordinate{rect.height() - 10}};
    const auto viewport = frameRect.insetBy(BlockMargins{1});
    const auto descriptionRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 2},
        BlockCoordinate{rect.y2() - 3},
        BlockCoordinate{rect.width() - 4},
        BlockCoordinate{2}};
    _buffer.drawFrame(frameRect, FrameStyle::Double, Color{fg::BrightCyan, bg::BrightBlack});

    auto options = BitmapDrawOptions{};
    options.setColorSequence(rainbowColors(), BitmapColorMode::ForwardDiagonalStripes);
    options.setColorAnimationOffset(variantIndex);
    switch (variantIndex) {
    case 0:
    case 1:
    case 2:
        options.setScaleMode(BitmapScaleMode::FullBlock);
        _buffer.drawBitmap(rocketBitmap(), viewport, cAlignments[variantIndex], options, _animationCycle / 2);
        break;
    case 3:
        _buffer.drawBitmap(ringBitmap(), viewport, cAlignments[variantIndex], options, _animationCycle / 2);
        break;
    case 4:
    default:
        options.setScaleMode(BitmapScaleMode::DoubleBlock);
        _buffer.drawBitmap(waveBitmap(), viewport, cAlignments[variantIndex], options, _animationCycle / 2);
        break;
    }
    _buffer.drawBlockText(
        cDescriptions[std::min(variantIndex, cDescriptions.size() - 1)], descriptionRect, Alignment::Center);
}

void BitmapShowcaseApp::drawStyleVariant(const BlockRectangle rect, const std::size_t variantIndex) {

    static constexpr auto cPanelColors = std::array<Color, 5>{
        Color{fg::BrightWhite, bg::Blue},
        Color{fg::BrightWhite, bg::Magenta},
        Color{fg::BrightWhite, bg::Green},
        Color{fg::BrightWhite, bg::BrightBlack},
        Color{fg::BrightWhite, bg::BrightBlack},
    };
    static const auto cDescriptions = std::array<el::String, 5>{
        "fullBlock() can be replaced with any single-width character. Its own colors are still overlaid on the base "
        "bitmap color."_el,
        "doubleBlocks() picks one character for the left cell and one for the right cell of every rendered bitmap "
        "pixel."_el,
        "halfBlocks() accepts any table of sixteen single-width characters. Entry 0 stays the empty 2x2 bitmap cell."_el,
        "A custom Block16Style is useful when the bitmap should turn into lines, traces, or a schematic instead of "
        "solid blocks."_el,
        "When a combination style is set, the rendered bitmap can merge into an existing frame grid instead of "
        "overwriting it."_el,
    };

    const auto panelColor = cPanelColors[std::min(variantIndex, cPanelColors.size() - 1)];
    drawPreviewPanel(rect, variantTitle(_pageIndex, variantIndex), panelColor);
    const auto demoRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 3},
        BlockCoordinate{rect.y1() + 2},
        BlockCoordinate{rect.width() - 6},
        BlockCoordinate{rect.height() - 7}};
    const auto descriptionRect = BlockRectangle{
        BlockCoordinate{rect.x1() + 2},
        BlockCoordinate{rect.y2() - 3},
        BlockCoordinate{rect.width() - 4},
        BlockCoordinate{2}};

    switch (variantIndex) {
    case 0: {
        auto options = BitmapDrawOptions{Color{fg::BrightWhite, bg::Blue}};
        options.setScaleMode(BitmapScaleMode::FullBlock);
        options.setFullBlock(Block{U'●', fg::BrightYellow});
        _buffer.drawBitmap(ringBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 1: {
        auto options = BitmapDrawOptions{Color{fg::BrightWhite, bg::Magenta}};
        options.setScaleMode(BitmapScaleMode::DoubleBlock);
        options.setDoubleBlocks(BlockString{"<>"_el});
        _buffer.drawBitmap(waveBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 2: {
        auto options = BitmapDrawOptions{Color{fg::BrightWhite, bg::Green}};
        options.setHalfBlocks(BlockString{" 123456789ABCDEF"_el});
        _buffer.drawBitmap(ringBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 3: {
        auto options = BitmapDrawOptions{Color{fg::BrightCyan, bg::BrightBlack}};
        options.setBlock16Style(Block16Style::lightRoundedFrame());
        _buffer.drawBitmap(circuitBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    case 4:
    default: {
        const auto viewport = demoRect.insetBy(BlockMargins{2, 1});
        _buffer.drawFrame(viewport, FrameStyle::Light, Color{fg::BrightBlack, bg::BrightBlack});
        for (auto x = viewport.x1() + 4; x < viewport.x2() - 4; x += 5) {
            _buffer.drawFrame(
                BlockRectangle{
                    BlockCoordinate{x},
                    BlockCoordinate{viewport.y1() + 1},
                    BlockCoordinate{4},
                    BlockCoordinate{viewport.height() - 2}},
                FrameStyle::Light);
        }
        auto options = BitmapDrawOptions{Color{fg::BrightCyan, bg::BrightBlack}};
        options.setBlock16Style(Block16Style::lightFrame());
        options.setCombinationStyle(BlockCombinationStyle::commonBoxFrame());
        _buffer.drawBitmap(circuitBitmap(), viewport, Alignment::Center, options, _animationCycle / 2);
        break;
    }
    }
    _buffer.drawBlockText(
        cDescriptions[std::min(variantIndex, cDescriptions.size() - 1)], descriptionRect, Alignment::Center);
}

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
