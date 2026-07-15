// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "GridLayoutApp.hpp"

#include <algorithm>

namespace demo {

void GridLayoutApp::beforeInitialize() {
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{60}, BlockCoordinate{20}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 60x20 cells for the grid layout demo."_el,
            Color{fg::BrightWhite, bg::Black}});
}

void GridLayoutApp::onKey(const Key &key) {
    if (key == U'c') {
        _columnCount = _columnCount % 5 + 1;
    } else if (key == U'r') {
        _rowCount = _rowCount % 3 + 1;
    } else {
        for (const auto &info : borderElements()) {
            if (key == static_cast<char32_t>(info.key)) {
                cycleBorder(info.element);
                return;
            }
        }
        TerminalApplication::onKey(key);
    }
}

void GridLayoutApp::onRenderToBuffer() {
    _buffer.fill(Block{U' ', bg::Black});

    const auto titleRect = BlockRectangle{
        BlockCoordinate{0}, BlockCoordinate{0}, BlockCoordinate{_buffer.size().width()}, BlockCoordinate{1}};
    const auto gridArea = BlockRectangle{
        BlockCoordinate{2},
        BlockCoordinate{2},
        BlockCoordinate{_buffer.size().width() - 4},
        BlockCoordinate{_buffer.size().height() - 9}};
    const auto statusRect = BlockRectangle{
        BlockCoordinate{2},
        BlockCoordinate{_buffer.size().height() - 6},
        BlockCoordinate{_buffer.size().width() - 4},
        BlockCoordinate{3}};
    const auto footerRect = BlockRectangle{
        BlockCoordinate{0},
        BlockCoordinate{_buffer.size().height() - 2},
        BlockCoordinate{_buffer.size().width()},
        BlockCoordinate{2}};

    _buffer.fill(titleRect, Block{U' ', bg::Blue});
    _buffer.drawBlockText(
        el::StringFormat{"FrameBorder + GridLayout  |  {} columns x {} rows"_el}.build(
            static_cast<int>(_columnCount), static_cast<int>(_rowCount)),
        titleRect,
        Alignment::CenterLeft,
        Color{fg::BrightWhite, bg::Blue});

    renderGrid(gridArea);
    renderStatus(statusRect);
    renderFooter(footerRect);
}

void GridLayoutApp::cycleBorder(const FrameBorderElement element) noexcept {
    _border.set(element, nextStyle(_border.style(element)), Color{fg::BrightWhite, bg::Black});
}

void GridLayoutApp::renderGrid(const BlockRectangle gridArea) {
    const auto layout = createLayout(gridArea.size());
    const auto gridSize = layout.size(_border);
    const auto origin = gridArea.alignmentOffset(gridSize, Alignment::Center);

    renderCellContent(layout, origin);
    _buffer.drawGridLayout(origin, layout, _border);
}

void GridLayoutApp::renderCellContent(const GridLayout &layout, const BlockPosition origin) {
    for (std::size_t row = 0; row < _rowCount; ++row) {
        for (std::size_t column = 0; column < _columnCount; ++column) {
            const auto rect = layout.cellRect(row, column, origin, _border);
            const auto background = ((row + column) % 2 == 0) ? bg::BrightBlack : bg::Black;
            _buffer.fill(rect, Block{U' ', background});
            _buffer.drawBlockText(
                el::StringFormat{"R{} C{}"_el}.build(static_cast<int>(row + 1), static_cast<int>(column + 1)),
                rect,
                Alignment::Center,
                Color{fg::BrightCyan, background});
        }
    }
}

void GridLayoutApp::renderStatus(const BlockRectangle statusRect) {
    auto status = BlockString{};
    appendBorderStatusLine(status, 0, 2);
    status += BlockString{U"\n"_el};
    appendBorderStatusLine(status, 2, 4);
    status += BlockString{U"\n"_el};
    appendBorderStatusLine(status, 4, 6);
    _buffer.drawBlockText(BlockText{status, statusRect, Alignment::CenterLeft});
}

void GridLayoutApp::appendBorderStatusLine(BlockString &status, const std::size_t begin, const std::size_t end) const {
    const auto &elements = borderElements();
    for (auto index = begin; index < end; ++index) {
        const auto &info = elements[index];
        status.append(
            fg::BrightYellow,
            el::StringFormat{"[{}]"_el}.build(info.key),
            fg::BrightWhite,
            el::StringFormat{" {}:{}  "_el}.build(info.label, styleName(_border.style(info.element))));
    }
}

void GridLayoutApp::renderFooter(const BlockRectangle footerRect) {
    _buffer.fill(footerRect, Block{U' ', bg::BrightBlack});
    auto prompt = BlockString{};
    prompt.append(
        bg::BrightBlack,
        fg::BrightGreen,
        "[1-6]"_el,
        fg::BrightWhite,
        " cycle border styles  "_el,
        fg::BrightCyan,
        "[C]"_el,
        fg::BrightWhite,
        " columns  "_el,
        fg::BrightCyan,
        "[R]"_el,
        fg::BrightWhite,
        " rows  "_el,
        fg::BrightGreen,
        "[Q]"_el,
        fg::BrightWhite,
        " quit"_el);
    _buffer.drawBlockText(BlockText{prompt, footerRect, Alignment::CenterLeft});
}

auto GridLayoutApp::createLayout(const BlockSize availableSize) const -> GridLayout {
    const auto separatorWidth =
        static_cast<BlockCoordinate>(_columnCount - 1) * borderLineSize(FrameBorderElement::VLine);
    const auto separatorHeight =
        static_cast<BlockCoordinate>(_rowCount - 1) * borderLineSize(FrameBorderElement::HLine);
    const auto lineWidth =
        borderLineSize(FrameBorderElement::Left) + borderLineSize(FrameBorderElement::Right) + separatorWidth;
    const auto lineHeight =
        borderLineSize(FrameBorderElement::Top) + borderLineSize(FrameBorderElement::Bottom) + separatorHeight;
    const auto contentWidth =
        std::max<BlockCoordinate>(static_cast<BlockCoordinate>(_columnCount), availableSize.width() - lineWidth);
    const auto contentHeight =
        std::max<BlockCoordinate>(static_cast<BlockCoordinate>(_rowCount), availableSize.height() - lineHeight);
    return GridLayout{distribute(contentWidth, _columnCount), distribute(contentHeight, _rowCount)};
}

auto GridLayoutApp::borderLineSize(const FrameBorderElement element) const noexcept -> BlockCoordinate {
    const auto style = _border.style(element);
    return style != FrameStyle::None && FrameBorder::isLineStyle(style) ? BlockCoordinate{1} : BlockCoordinate{0};
}

auto GridLayoutApp::nextStyle(const FrameStyle style) noexcept -> FrameStyle {
    switch (style) {
    case FrameStyle::Light:
        return FrameStyle::LightDoubleDash;
    case FrameStyle::LightDoubleDash:
        return FrameStyle::LightTripleDash;
    case FrameStyle::LightTripleDash:
        return FrameStyle::LightQuadrupleDash;
    case FrameStyle::LightQuadrupleDash:
        return FrameStyle::LightWithRoundedCorners;
    case FrameStyle::LightWithRoundedCorners:
        return FrameStyle::Heavy;
    case FrameStyle::Heavy:
        return FrameStyle::HeavyDoubleDash;
    case FrameStyle::HeavyDoubleDash:
        return FrameStyle::HeavyTripleDash;
    case FrameStyle::HeavyTripleDash:
        return FrameStyle::HeavyQuadrupleDash;
    case FrameStyle::HeavyQuadrupleDash:
        return FrameStyle::Double;
    case FrameStyle::Double:
        return FrameStyle::None;
    case FrameStyle::None:
    default:
        return FrameStyle::Light;
    }
}

auto GridLayoutApp::styleName(const FrameStyle style) noexcept -> el::StringView {
    switch (style) {
    case FrameStyle::Light:
        return "Light"_el;
    case FrameStyle::LightDoubleDash:
        return "LightDoubleDash"_el;
    case FrameStyle::LightTripleDash:
        return "LightTripleDash"_el;
    case FrameStyle::LightQuadrupleDash:
        return "LightQuadDash"_el;
    case FrameStyle::LightWithRoundedCorners:
        return "Rounded"_el;
    case FrameStyle::Heavy:
        return "Heavy"_el;
    case FrameStyle::HeavyDoubleDash:
        return "HeavyDoubleDash"_el;
    case FrameStyle::HeavyTripleDash:
        return "HeavyTripleDash"_el;
    case FrameStyle::HeavyQuadrupleDash:
        return "HeavyQuadDash"_el;
    case FrameStyle::Double:
        return "Double"_el;
    case FrameStyle::None:
    default:
        return "None"_el;
    }
}

auto GridLayoutApp::borderElements() noexcept -> const std::array<BorderElementInfo, 6> & {
    static const auto cElements = std::array{
        BorderElementInfo{FrameBorderElement::Top, '1', "Top"_el},
        BorderElementInfo{FrameBorderElement::Bottom, '2', "Bottom"_el},
        BorderElementInfo{FrameBorderElement::Left, '3', "Left"_el},
        BorderElementInfo{FrameBorderElement::Right, '4', "Right"_el},
        BorderElementInfo{FrameBorderElement::HLine, '5', "HLine"_el},
        BorderElementInfo{FrameBorderElement::VLine, '6', "VLine"_el},
    };
    return cElements;
}

auto GridLayoutApp::distribute(const BlockCoordinate total, const std::size_t count) -> std::vector<BlockCoordinate> {
    auto result = std::vector<BlockCoordinate>(count, total / static_cast<BlockCoordinate>(count));
    const auto remainder = total % static_cast<BlockCoordinate>(count);
    for (auto index = BlockCoordinate{0}; index < remainder; ++index) {
        ++result[index.toSizeT()];
    }
    return result;
}

}
