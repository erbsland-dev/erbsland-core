// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TextGalleryApp.hpp"

#include <algorithm>

void TextGalleryApp::beforeInitialize() {
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{38}, BlockCoordinate{14}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 38x14 cells for the text gallery."_el, Color{fg::BrightWhite, bg::Black}});
}

auto TextGalleryApp::beforeMain() -> int {
    _font = Font::defaultAscii();
    return 0;
}

void TextGalleryApp::onKey(const Key &key) {
    if (key == Key::Left) {
        _pageIndex = (_pageIndex + 2) % 3;
    } else if (key == Key::Right) {
        _pageIndex = (_pageIndex + 1) % 3;
    } else {
        TerminalApplication::onKey(key);
    }
}

void TextGalleryApp::onRenderToBuffer() {
    _buffer.fill(Block{U' ', bg::Black});
    const auto outerRect = BlockRectangle{
        BlockCoordinate{0},
        BlockCoordinate{0},
        BlockCoordinate{_buffer.size().width()},
        BlockCoordinate{_buffer.size().height()}};
    _buffer.drawFrame(outerRect, FrameStyle::LightWithRoundedCorners);
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
    _buffer.drawBlockText(
        "BlockText Gallery  |  alignment, wrapping, wide characters, and bitmap fonts"_el,
        titleRect,
        Alignment::Center,
        Color{fg::BrightWhite, bg::Black});
    switch (_pageIndex) {
    case 0:
        drawOverviewPage(contentRect);
        break;
    case 1:
        drawMixedWidthPage(contentRect);
        break;
    case 2:
    default:
        drawBitmapFontPage(contentRect);
        break;
    }
    drawFooter(footerRect);
}

void TextGalleryApp::drawOverviewPage(const BlockRectangle contentRect) {
    const auto gap = BlockCoordinate{1};
    const auto columnWidth = std::max(BlockCoordinate{12}, (contentRect.width() - gap) / 2);
    const auto rowHeight = std::max(BlockCoordinate{5}, (contentRect.height() - gap) / 2);
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1()},
            BlockCoordinate{contentRect.y1()},
            BlockCoordinate{columnWidth},
            BlockCoordinate{rowHeight}},
        "Top Left"_el,
        "Small panels are an easy way to compare alignment and wrapping side by side."_el,
        Alignment::TopLeft,
        FrameStyle::Light,
        Color{fg::White, bg::BrightBlack},
        Color{fg::White, bg::BrightBlack});
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1() + columnWidth + gap},
            BlockCoordinate{contentRect.y1()},
            BlockCoordinate{columnWidth},
            BlockCoordinate{rowHeight}},
        "Center"_el,
        "The same paragraph can be centered without a custom layout engine."_el,
        Alignment::Center,
        FrameStyle::Heavy,
        Color{fg::BrightBlue, bg::Blue},
        Color{fg::BrightBlue, bg::Blue});
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1()},
            BlockCoordinate{contentRect.y1() + rowHeight + gap},
            BlockCoordinate{columnWidth},
            BlockCoordinate{rowHeight}},
        "Bottom Right"_el,
        "Right and bottom alignment stay readable even inside narrow frames."_el,
        Alignment::BottomRight,
        FrameStyle::Double,
        Color{fg::BrightMagenta, bg::Magenta},
        Color{fg::BrightMagenta, bg::Magenta});
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1() + columnWidth + gap},
            BlockCoordinate{contentRect.y1() + rowHeight + gap},
            BlockCoordinate{columnWidth},
            BlockCoordinate{rowHeight}},
        "Top Center"_el,
        "Wrapping respects the box width, while alignment still decides where each line starts."_el,
        Alignment::TopCenter,
        FrameStyle::LightWithRoundedCorners,
        Color{fg::BrightCyan, bg::Cyan},
        Color{fg::BrightCyan, bg::Cyan});
}

void TextGalleryApp::drawMixedWidthPage(const BlockRectangle contentRect) {
    const auto topHeight = std::max(BlockCoordinate{5}, contentRect.height() / 2 - 1);
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1()},
            BlockCoordinate{contentRect.y1()},
            BlockCoordinate{contentRect.width()},
            BlockCoordinate{topHeight}},
        "Mixed Width Layout"_el,
        "English meets 日本語 and 漢字 in the same wrapped paragraph. The frame and alignment stay stable because "
        "the renderer measures each character width instead of counting bytes."_el,
        Alignment::Center,
        FrameStyle::Light,
        bg::BrightBlack,
        Color{fg::BrightWhite, bg::BrightBlack});

    const auto lowerY = contentRect.y1() + topHeight + 1;
    const auto lowerHeight = contentRect.y2() - lowerY;
    const auto leftWidth = std::max(BlockCoordinate{12}, (contentRect.width() - 1) / 2);
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1()},
            BlockCoordinate{lowerY},
            BlockCoordinate{leftWidth},
            BlockCoordinate{lowerHeight}},
        "Center"_el,
        "A界B  C東京D  E文字F\nCentered text keeps the wide glyphs balanced."_el,
        Alignment::Center,
        FrameStyle::Heavy,
        bg::Blue,
        Color{fg::BrightWhite, bg::Blue});
    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1() + leftWidth + 1},
            BlockCoordinate{lowerY},
            BlockCoordinate{leftWidth},
            BlockCoordinate{lowerHeight}},
        "Right"_el,
        "右寄せ with ASCII, kana, and kanji.\nZürich, 東京, Kyoto, and 大阪 all line up cleanly."_el,
        Alignment::BottomRight,
        FrameStyle::Double,
        bg::Magenta,
        Color{fg::BrightWhite, bg::Magenta});
}

void TextGalleryApp::drawBitmapFontPage(const BlockRectangle contentRect) {
    const auto titleHeight = std::min(BlockCoordinate{6}, contentRect.height());
    auto title = BlockText{
        BlockString{titleForWidth(contentRect.width())},
        BlockRectangle{
            BlockCoordinate{contentRect.x1()},
            BlockCoordinate{contentRect.y1()},
            BlockCoordinate{contentRect.width()},
            BlockCoordinate{titleHeight}},
        Alignment::Center};
    title.setFont(_font);
    title.setColorSequence(titleColors());
    title.setAnimation(BlockTextAnimation::ColorDiagonal);
    _buffer.drawBlockText(title, _animationCycle);

    drawPanel(
        BlockRectangle{
            BlockCoordinate{contentRect.x1()},
            BlockCoordinate{contentRect.y1() + titleHeight},
            BlockCoordinate{contentRect.width()},
            BlockCoordinate{contentRect.height() - titleHeight}},
        "Bitmap Font"_el,
        "The large title above uses the built-in default ASCII bitmap font. Regular text still fits naturally "
        "around it, so a page can mix dramatic headlines with practical terminal UI copy."_el,
        Alignment::TopCenter,
        FrameStyle::LightWithRoundedCorners,
        bg::BrightBlack,
        Color{fg::BrightWhite, bg::BrightBlack});
}

void TextGalleryApp::drawPanel(
    const BlockRectangle rect,
    const el::StringView title,
    const el::StringView text,
    const Alignment alignment,
    const FrameStyle frameStyle,
    const Color fillColor,
    const Color textColor) {

    if (rect.width() <= 2 || rect.height() <= 2) {
        return;
    }
    _buffer.drawFilledFrame(rect, frameStyle, Block{U' ', fillColor});
    _buffer.drawBlockText(
        title,
        BlockRectangle{
            BlockCoordinate{rect.x1() + 2},
            BlockCoordinate{rect.y1()},
            BlockCoordinate{rect.width() - 4},
            BlockCoordinate{1}},
        Alignment::Center,
        textColor);
    _buffer.drawBlockText(
        text,
        BlockRectangle{
            BlockCoordinate{rect.x1() + 1},
            BlockCoordinate{rect.y1() + 1},
            BlockCoordinate{rect.width() - 2},
            BlockCoordinate{rect.height() - 2}},
        alignment,
        textColor);
}

void TextGalleryApp::drawFooter(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', bg::BrightBlack});
    auto footer = BlockText{buildFooterText(), rect, Alignment::CenterLeft};
    _buffer.drawBlockText(footer);
}

auto TextGalleryApp::buildFooterText() const -> BlockString {
    auto result = BlockString{};
    result.append(
        fg::BrightCyan,
        "[←][→]"_el,
        fg::BrightWhite,
        " switch pages  "_el,
        fg::BrightYellow,
        "[Q]"_el,
        fg::BrightWhite,
        " quit  "_el,
        fg::BrightGreen,
        el::StringFormat{"page {}/3"_el}.build(_pageIndex + 1));
    return result;
}

auto TextGalleryApp::titleColors() -> ColorSequence {
    return ColorSequence{
        {Color{fg::BrightBlue, bg::Black}, 10},
        {Color{fg::BrightCyan, bg::Black}, 5},
        {Color{fg::BrightMagenta, bg::Black}, 3},
        {Color{fg::BrightYellow, bg::Black}, 2},
        {Color{fg::BrightGreen, bg::Black}, 5},
    };
}

auto TextGalleryApp::titleForWidth(const BlockCoordinate width) -> el::StringView {
    if (width >= 60) {
        return "-+[ COLOR TERM ]+-"_el;
    }
    if (width >= 36) {
        return "COLOR TERM"_el;
    }
    if (width >= 22) {
        return "COLOR"_el;
    }
    return "TERM"_el;
}
