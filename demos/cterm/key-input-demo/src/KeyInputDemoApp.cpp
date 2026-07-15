// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "KeyInputDemoApp.hpp"

#include <algorithm>

namespace demo {

void KeyInputDemoApp::beforeInitialize() {
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{56}, BlockCoordinate{10}});
    _updateSettings.setMinimumSizeBackground(backgroundChar());
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 56x10 cells for the key input demo."_el,
            Color{fg::BrightWhite, bg::Black}});
}

auto KeyInputDemoApp::beforeMain() -> int {
    initializeScrollBuffer();
    _firstFrame = true;
    return 0;
}

auto KeyInputDemoApp::canvasSize() noexcept -> BlockSize {
    if (_buffer.size().isZero()) {
        return terminal()->size().expandedWith(_updateSettings.minimumSize());
    }
    return _buffer.size();
}

auto KeyInputDemoApp::fieldRectForCanvas(const BlockSize canvasSize) const noexcept -> BlockRectangle {
    const auto availableHeight = std::max(BlockCoordinate{1}, canvasSize.height() - 4);
    const auto fieldSize = BlockSize{
        std::min(canvasSize.width(), cScrollBufferSize.width()),
        std::min(availableHeight, cScrollBufferSize.height()),
    };
    const auto x = std::max(BlockCoordinate{0}, (canvasSize.width() - fieldSize.width()) / 2);
    return {x, BlockCoordinate{2}, fieldSize.width(), fieldSize.height()};
}

auto KeyInputDemoApp::visibleFieldSize() noexcept -> BlockSize {
    return fieldRectForCanvas(canvasSize()).size();
}

void KeyInputDemoApp::initializeScrollBuffer() noexcept {
    _scrollBuffer.fill(backgroundChar());
    for (auto column = 0; column < cScrollBufferSize.width(); column += 20) {
        _scrollBuffer.fill(
            BlockRectangle{
                BlockCoordinate{column},
                BlockCoordinate{0},
                BlockCoordinate{1},
                BlockCoordinate{cScrollBufferSize.height()}},
            guideColumnChar());
    }
    _insertedColumnCount = cScrollBufferSize.width().toSizeT();
}

void KeyInputDemoApp::advanceScroll() noexcept {
    const auto fillChar = (_insertedColumnCount % 20 == 0) ? guideColumnChar() : backgroundChar();
    _scrollBuffer.shift(BlockDirection::East, fillChar, 1);
    _insertedColumnCount += 1;
}

void KeyInputDemoApp::onKey(const Key &key) {
    if (key == Key::Escape) {
        _quitRequested = true;
        return;
    }
    stampKeyBlock(key);
}

void KeyInputDemoApp::stampKeyBlock(const Key &key) noexcept {
    const auto fieldSize = visibleFieldSize();
    if (fieldSize.width() <= 0 || fieldSize.height() <= 0) {
        return;
    }
    const auto colorIndex = static_cast<std::size_t>(
        std::uniform_int_distribution<int>{0, static_cast<int>(stampColors().size()) - 1}(_random));
    auto block = BlockString{key.toDisplayText(), stampColors()[colorIndex]};
    const auto maximumX =
        std::min(BlockCoordinate{20}, std::max(BlockCoordinate{0}, fieldSize.width() - block.displayWidth()));
    const auto x = std::uniform_int_distribution<int>{0, maximumX.toRawValue()}(_random);
    const auto y = std::uniform_int_distribution<int>{0, (fieldSize.height() - 1).toRawValue()}(_random);
    _scrollBuffer.drawBlockText(BlockPosition{BlockCoordinate{x}, BlockCoordinate{y}}, block);
}

void KeyInputDemoApp::onRenderToBuffer() {
    const auto visibleCanvas = canvasSize();
    if (_firstFrame) {
        _firstFrame = false;
    } else {
        advanceScroll();
    }
    _buffer.fill(backgroundChar());

    const auto headerRect = BlockRectangle{
        BlockCoordinate{0}, BlockCoordinate{0}, BlockCoordinate{visibleCanvas.width()}, BlockCoordinate{1}};
    const auto fieldRect = fieldRectForCanvas(visibleCanvas);
    const auto footerRect = BlockRectangle{
        BlockCoordinate{0},
        BlockCoordinate{_buffer.size().height() - 1},
        BlockCoordinate{_buffer.size().width()},
        BlockCoordinate{1}};

    drawHeader(headerRect);
    drawField(fieldRect);
    drawFooter(footerRect);
}

void KeyInputDemoApp::drawHeader(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', Color{fg::BrightWhite, bg::BrightBlack}});
    _buffer.drawBlockText(
        "Key Input Demo"_el,
        rect.insetBy(BlockMargins{2, 0}),
        Alignment::CenterLeft,
        Color{fg::BrightWhite, bg::BrightBlack});
    _buffer.drawBlockText(
        el::StringFormat{"horizontal RemappedBuffer {}x{}"_el}.build(
            cScrollBufferSize.width(), cScrollBufferSize.height()),
        rect.insetBy(BlockMargins{2, 0}),
        Alignment::CenterRight,
        Color{fg::BrightCyan, bg::BrightBlack});
}

void KeyInputDemoApp::drawField(const BlockRectangle rect) {
    _buffer.fill(rect, backgroundChar());
    auto view = BufferConstRefView{_scrollBuffer, rect.size()};
    _buffer.drawBuffer(view, rect);
}

void KeyInputDemoApp::drawFooter(const BlockRectangle rect) {
    _buffer.fill(rect, Block{U' ', Color{fg::BrightWhite, bg::BrightBlack}});
    _buffer.drawBlockText(BlockText{footerText(), rect.insetBy(BlockMargins{1, 0}), Alignment::CenterLeft});
}

auto KeyInputDemoApp::footerText() const -> BlockString {
    auto result = BlockString{};
    result.append(
        fg::BrightYellow,
        Key{Key::Escape}.toDisplayText(),
        fg::BrightWhite,
        " quit  "_el,
        fg::BrightCyan,
        "Press any other key to stamp a colored key block into the scrolling field."_el);
    return result;
}

auto KeyInputDemoApp::backgroundChar() noexcept -> Block {
    return Block{U' ', Color{fg::Default, bg::Black}};
}

auto KeyInputDemoApp::guideColumnChar() noexcept -> Block {
    return Block{U'∙', Color{fg::BrightBlack, bg::Black}};
}

auto KeyInputDemoApp::stampColors() noexcept -> const std::array<Color, 10> & {
    static const auto cStampColors = std::array<Color, 10>{
        Color{fg::Black, bg::BrightYellow},
        Color{fg::Black, bg::BrightRed},
        Color{fg::Black, bg::BrightCyan},
        Color{fg::Black, bg::BrightGreen},
        Color{fg::Black, bg::BrightBlue},
        Color{fg::BrightWhite, bg::Green},
        Color{fg::BrightWhite, bg::Blue},
        Color{fg::BrightWhite, bg::Cyan},
        Color{fg::BrightWhite, bg::Red},
        Color{fg::BrightWhite, bg::Yellow},
    };
    return cStampColors;
}

}
