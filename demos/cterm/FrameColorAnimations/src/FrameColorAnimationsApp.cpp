// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FrameColorAnimationsApp.hpp"

#include <array>

namespace demo {

void FrameColorAnimationsApp::beforeInitialize() {
    _updateSettings.setMinimumSize(Size{Coordinate{78}, Coordinate{22}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 78x22 cells for the frame color animation demo."_el,
            Color{fg::BrightWhite, bg::Black}});
}

void FrameColorAnimationsApp::onRenderToBuffer() {
    _buffer.fill(Block{U' ', bg::Black});

    const auto outerRect = Rectangle{
        Coordinate{0}, Coordinate{0}, Coordinate{_buffer.size().width()}, Coordinate{_buffer.size().height()}};
    const auto headerRect =
        Rectangle{Coordinate{2}, Coordinate{1}, Coordinate{_buffer.size().width() - 4}, Coordinate{3}};
    const auto contentRect = Rectangle{
        Coordinate{2}, Coordinate{5}, Coordinate{_buffer.size().width() - 4}, Coordinate{_buffer.size().height() - 9}};
    const auto footerRect = Rectangle{
        Coordinate{2}, Coordinate{_buffer.size().height() - 3}, Coordinate{_buffer.size().width() - 4}, Coordinate{1}};

    auto outerOptions = FrameDrawOptions{};
    outerOptions.setStyle(FrameStyle::LightWithRoundedCorners);
    outerOptions.setFrameColorSequence(outerFrameColors(), FrameColorMode::ChasingBorderCW);
    outerOptions.setFillBlock(Block{U' '});
    outerOptions.setFillColorSequence(fillColors(), FrameColorMode::ForwardDiagonalStripes);
    _buffer.drawFrame(outerRect, outerOptions, _animationCycle);

    drawHeader(headerRect);

    const auto cells = contentRect.gridCells(2, 4);
    const auto &panels = panelSpecs();
    for (std::size_t index = 0; index < panels.size(); ++index) {
        drawPanel(cells[index].insetBy(Margins{1, 0}), panels[index]);
    }
    drawFooter(footerRect);
}

void FrameColorAnimationsApp::drawPanel(const Rectangle rect, const PanelSpec &panel) {
    auto options = FrameDrawOptions{};
    options.setStyle(panel.style);
    options.setFrameColorSequence(colorSequence(panel.sequenceIndex), panel.mode);
    options.setFillBlock(Block{U' '});
    options.setFillColorSequence(ColorSequence{Color{fg::Inherited, bg::Black}, 1}, FrameColorMode::OneColor);

    if (panel.mode == FrameColorMode::ForwardDiagonalStripes || panel.mode == FrameColorMode::BackwardDiagonalStripes) {
        _buffer.drawFrame(rect, options, _animationCycle * 6);
    } else {
        _buffer.drawFrame(rect, options, _animationCycle);
    }
    _buffer.drawBlockText(
        panel.title,
        Rectangle{Coordinate{rect.x1() + 2}, Coordinate{rect.y1() + 1}, Coordinate{rect.width() - 4}, Coordinate{1}},
        Alignment::Center,
        Color{fg::BrightWhite, bg::Inherited});
    _buffer.drawBlockText(
        "animated frame"_el,
        Rectangle{Coordinate{rect.x1() + 2}, Coordinate{rect.y1() + 3}, Coordinate{rect.width() - 4}, Coordinate{1}},
        Alignment::Center,
        Color{fg::BrightBlack, bg::Inherited});
}

void FrameColorAnimationsApp::drawHeader(const Rectangle rect) {
    _buffer.drawBlockText(
        "Frame Color Animations"_el,
        Rectangle{Coordinate{rect.x1()}, Coordinate{rect.y1()}, Coordinate{rect.width()}, Coordinate{1}},
        Alignment::Center,
        Color{fg::BrightWhite, bg::Black});
    _buffer.drawBlockText(
        "This is a demo of various frame animation modes."_el,
        Rectangle{Coordinate{rect.x1()}, Coordinate{rect.y1() + 2}, Coordinate{rect.width()}, Coordinate{1}},
        Alignment::Center,
        Color{fg::BrightYellow, bg::Black});
}

void FrameColorAnimationsApp::drawFooter(const Rectangle rect) {
    _buffer.drawBlockText("Press q to quit."_el, rect, Alignment::CenterRight, Color{fg::BrightBlack, bg::Inherited});
}

auto FrameColorAnimationsApp::panelSpecs() -> const std::array<PanelSpec, 7> & {
    static const auto cPanels = std::array<PanelSpec, 7>{
        PanelSpec{"OneColor"_el, FrameStyle::LightWithRoundedCorners, FrameColorMode::OneColor, 6},
        PanelSpec{"Vertical Stripes"_el, FrameStyle::Light, FrameColorMode::VerticalStripes, 1},
        PanelSpec{"Horizontal Stripes"_el, FrameStyle::Heavy, FrameColorMode::HorizontalStripes, 3},
        PanelSpec{"Forward Diagonal"_el, FrameStyle::Double, FrameColorMode::ForwardDiagonalStripes, 0},
        PanelSpec{"Backward Diagonal"_el, FrameStyle::OuterHalfBlock, FrameColorMode::BackwardDiagonalStripes, 4},
        PanelSpec{"Chasing Border CW"_el, FrameStyle::Heavy, FrameColorMode::ChasingBorderCW, 5},
        PanelSpec{"Chasing Border CCW"_el, FrameStyle::LightWithRoundedCorners, FrameColorMode::ChasingBorderCCW, 2},
    };
    return cPanels;
}

auto FrameColorAnimationsApp::colorSequence(const std::size_t index) -> const ColorSequence & {
    static const auto cSequences = std::array<ColorSequence, 7>{
        ColorSequence{
            {Color{fg::BrightWhite, bg::Blue}, 2},
            {Color{fg::BrightWhite, bg::Black}, 4},
            {Color{fg::BrightCyan, bg::Black}, 6},
            {Color{fg::BrightBlue, bg::Black}, 80},
        },
        ColorSequence{
            {Color{fg::BrightCyan, bg::Black}, 1},
            {Color{fg::BrightWhite, bg::Black}, 2},
            {Color{fg::BrightCyan, bg::Black}, 1},
            {Color{fg::Cyan, bg::Black}, 4},
        },
        ColorSequence{
            {Color{fg::BrightGreen, bg::Black}, 1},
            {Color{fg::BrightWhite, bg::Black}, 2},
            {Color{fg::BrightGreen, bg::Black}, 1},
            {Color{fg::Green, bg::Black}, 4},
        },
        ColorSequence{
            {Color{fg::BrightYellow, bg::Black}, 2},
            {Color{fg::BrightWhite, bg::Black}, 3},
            {Color{fg::BrightGreen, bg::Black}, 10},
            {Color{fg::Green, bg::Black}, 84},
        },
        ColorSequence{
            {Color{fg::Yellow, bg::Black}, 2},
            {Color{fg::BrightWhite, bg::Black}, 3},
            {Color{fg::Yellow, bg::Black}, 10},
            {Color{fg::Red, bg::Black}, 84},
        },
        ColorSequence{
            {Color{fg::BrightWhite, bg::Black}, 2},
            {Color{fg::BrightYellow, bg::Black}, 4},
            {Color{fg::Yellow, bg::Black}, 10},
            {Color{fg::BrightYellow, bg::Black}, 84},
        },
        ColorSequence{
            {Color{fg::BrightMagenta, bg::Black}, 1},
            {Color{fg::BrightWhite, bg::Black}, 2},
            {Color{fg::BrightMagenta, bg::Black}, 1},
            {Color{fg::Magenta, bg::Black}, 4},
        },
    };
    return cSequences[index % cSequences.size()];
}

auto FrameColorAnimationsApp::outerFrameColors() -> const ColorSequence & {
    return colorSequence(2);
}

auto FrameColorAnimationsApp::fillColors() -> const ColorSequence & {
    static const auto cFillColors = ColorSequence{
        {Color{fg::Inherited, bg::Black}, 15},
        {Color{fg::Inherited, bg::BrightBlack}, 1},
    };
    return cFillColors;
}

}
