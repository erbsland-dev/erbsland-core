// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BitmapShowcaseApp.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace demo {

void BitmapShowcaseApp::beforeInitialize() {
    _updateSettings.setMinimumSize(Size{Coordinate{68}, Coordinate{20}});
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
    const auto outerRect = Rectangle{
        Coordinate{0}, Coordinate{0}, Coordinate{_buffer.size().width()}, Coordinate{_buffer.size().height()}};
    const auto titleRect =
        Rectangle{Coordinate{2}, Coordinate{1}, Coordinate{_buffer.size().width() - 4}, Coordinate{1}};
    const auto contentRect = Rectangle{
        Coordinate{2}, Coordinate{3}, Coordinate{_buffer.size().width() - 4}, Coordinate{_buffer.size().height() - 7}};
    const auto footerRect = Rectangle{
        Coordinate{2}, Coordinate{_buffer.size().height() - 3}, Coordinate{_buffer.size().width() - 4}, Coordinate{1}};
    auto selectorWidth = std::clamp(contentRect.width() / 3, Coordinate{22}, Coordinate{30});
    if (contentRect.width() - selectorWidth - 2 < 36) {
        selectorWidth = std::max(Coordinate{18}, contentRect.width() / 4);
    }
    const auto selectorRect = Rectangle{
        Coordinate{contentRect.x1()},
        Coordinate{contentRect.y1()},
        Coordinate{selectorWidth},
        Coordinate{contentRect.height()}};
    const auto previewRect = Rectangle{
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

void BitmapShowcaseApp::drawSelector(const Rectangle rect) {
    const auto count = static_cast<int>(variantCount(_pageIndex));
    if (count <= 0 || rect.height() <= 2) {
        return;
    }
    const auto selected = static_cast<int>(selectedVariantIndex());
    const auto rowStride = rect.height() >= count * 2 + 2 ? Coordinate{2} : Coordinate{1};
    const auto visibleCount = static_cast<int>(std::max(Coordinate{1}, (rect.height() - 1) / rowStride).toRawValue());
    auto firstVisible = std::max(0, selected - visibleCount / 2);
    firstVisible = std::min(firstVisible, std::max(0, count - visibleCount));

    for (auto visibleIndex = 0; visibleIndex < visibleCount; ++visibleIndex) {
        const auto itemIndex = firstVisible + visibleIndex;
        if (itemIndex >= count) {
            break;
        }
        const auto itemIndexAsSize = static_cast<std::size_t>(itemIndex);
        const auto y = rect.y1() + visibleIndex * rowStride;
        const auto lineRect = Rectangle{Coordinate{rect.x1()}, Coordinate{y}, Coordinate{rect.width()}, Coordinate{1}};
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

void BitmapShowcaseApp::drawPreview(const Rectangle rect) {
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

void BitmapShowcaseApp::drawScaleModeVariant(const Rectangle rect, const std::size_t variantIndex) {

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
    const auto demoRect = Rectangle{
        Coordinate{rect.x1() + 3},
        Coordinate{rect.y1() + 2},
        Coordinate{rect.width() - 6},
        Coordinate{rect.height() - 7}};
    const auto descriptionRect =
        Rectangle{Coordinate{rect.x1() + 2}, Coordinate{rect.y2() - 3}, Coordinate{rect.width() - 4}, Coordinate{2}};

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

void BitmapShowcaseApp::drawColorModeVariant(const Rectangle rect, const std::size_t variantIndex) {

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
    const auto demoRect = Rectangle{
        Coordinate{rect.x1() + 3},
        Coordinate{rect.y1() + 2},
        Coordinate{rect.width() - 6},
        Coordinate{rect.height() - 7}};
    const auto descriptionRect =
        Rectangle{Coordinate{rect.x1() + 2}, Coordinate{rect.y2() - 3}, Coordinate{rect.width() - 4}, Coordinate{2}};

    auto options = BitmapDrawOptions{};
    options.setColorSequence(rainbowColors(), cModes[std::min(variantIndex, cModes.size() - 1)]);
    options.setScaleMode(BitmapScaleMode::DoubleBlock);
    options.setColorAnimationOffset(variantIndex * 2);
    _buffer.drawBitmap(waveBitmap(), demoRect, Alignment::Center, options, _animationCycle / 2);
    _buffer.drawBlockText(
        cDescriptions[std::min(variantIndex, cDescriptions.size() - 1)], descriptionRect, Alignment::Center);
}

void BitmapShowcaseApp::drawLayoutVariant(const Rectangle rect, const std::size_t variantIndex) {

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
    const auto frameRect = Rectangle{
        Coordinate{rect.x1() + 6},
        Coordinate{rect.y1() + 3},
        Coordinate{rect.width() - 12},
        Coordinate{rect.height() - 10}};
    const auto viewport = frameRect.insetBy(Margins{1});
    const auto descriptionRect =
        Rectangle{Coordinate{rect.x1() + 2}, Coordinate{rect.y2() - 3}, Coordinate{rect.width() - 4}, Coordinate{2}};
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

void BitmapShowcaseApp::drawStyleVariant(const Rectangle rect, const std::size_t variantIndex) {

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
    const auto demoRect = Rectangle{
        Coordinate{rect.x1() + 3},
        Coordinate{rect.y1() + 2},
        Coordinate{rect.width() - 6},
        Coordinate{rect.height() - 7}};
    const auto descriptionRect =
        Rectangle{Coordinate{rect.x1() + 2}, Coordinate{rect.y2() - 3}, Coordinate{rect.width() - 4}, Coordinate{2}};

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
        const auto viewport = demoRect.insetBy(Margins{2, 1});
        _buffer.drawFrame(viewport, FrameStyle::Light, Color{fg::BrightBlack, bg::BrightBlack});
        for (auto x = viewport.x1() + 4; x < viewport.x2() - 4; x += 5) {
            _buffer.drawFrame(
                Rectangle{
                    Coordinate{x}, Coordinate{viewport.y1() + 1}, Coordinate{4}, Coordinate{viewport.height() - 2}},
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

}
