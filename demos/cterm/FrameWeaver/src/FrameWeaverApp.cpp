// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FrameWeaverApp.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace demo {

void FrameWeaverApp::beforeInitialize() {
    _updateSettings.setMinimumSize(Size{Coordinate{32}, Coordinate{10}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 32x10 cells for the frame demo."_el, Color{fg::BrightWhite, bg::Black}});
}

auto FrameWeaverApp::beforeMain() -> int {
    _lastTick = std::chrono::steady_clock::now();
    return 0;
}

void FrameWeaverApp::onKey(const Key &key) {
    if (key == U'c') {
        _frames.clear();
        _accumulator = std::chrono::milliseconds{};
    } else if (key == U'f') {
        _frameDelay = std::max(std::chrono::milliseconds{250}, _frameDelay - std::chrono::milliseconds{150});
    } else if (key == U's') {
        _frameDelay = std::min(std::chrono::milliseconds{2000}, _frameDelay + std::chrono::milliseconds{150});
    } else if (key == U'1') {
        _styleMode = StyleMode::Light;
    } else if (key == U'2') {
        _styleMode = StyleMode::Double;
    } else if (key == U'3') {
        _styleMode = StyleMode::Heavy;
    } else if (key == U'4') {
        _styleMode = StyleMode::Mixed;
    } else if (key == U'5') {
        _styleMode = StyleMode::Block;
    } else if (key == U'6') {
        _styleMode = StyleMode::Custom;
    } else if (key == U'7') {
        _styleMode = StyleMode::All;
    } else {
        TerminalApplication::onKey(key);
    }
}

void FrameWeaverApp::updateAnimation(const std::chrono::milliseconds elapsed) noexcept {
    _accumulator += elapsed;
    while (_accumulator >= _frameDelay) {
        _accumulator -= _frameDelay;
        addFrame();
    }
}

void FrameWeaverApp::addFrame() noexcept {
    static constexpr auto cMaxFrames = std::size_t{20};

    if (_frames.size() >= cMaxFrames) {
        _frames.clear();
    }
    _frames.push_back(createRandomFrame());
}

void FrameWeaverApp::onRenderToBuffer() {
    const auto now = std::chrono::steady_clock::now();
    updateAnimation(std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastTick));
    _lastTick = now;
    _buffer.fill(Block{U' ', bg::Black});
    const auto titleRect = Rectangle{Coordinate{0}, Coordinate{0}, Coordinate{_buffer.size().width()}, Coordinate{1}};
    const auto contentRect = Rectangle{
        Coordinate{0}, Coordinate{1}, Coordinate{_buffer.size().width()}, Coordinate{_buffer.size().height() - 2}};
    const auto footerRect = Rectangle{
        Coordinate{0}, Coordinate{_buffer.size().height() - 1}, Coordinate{_buffer.size().width()}, Coordinate{1}};
    _buffer.fill(titleRect, Block{U' ', bg::Blue});
    _buffer.fill(footerRect, Block{U' ', bg::BrightBlack});
    _buffer.drawBlockText(
        el::StringFormat{"Frame Weaver  |  frames {:02d}/20  |  interval {:.2f}s  |  mode {}"_el}.build(
            static_cast<int>(_frames.size()), static_cast<double>(_frameDelay.count()) / 1000.0, modeName()),
        titleRect,
        Alignment::CenterLeft,
        Color{fg::BrightWhite, bg::Blue});
    renderFrames(contentRect);
    auto prompt = BlockText{buildPrompt(), footerRect, Alignment::CenterLeft};
    _buffer.drawBlockText(prompt);
}

void FrameWeaverApp::renderFrames(const Rectangle contentRect) {
    for (const auto &frame : _frames) {
        if (frame.customStyle != nullptr) {
            _buffer.drawFrame(
                frameRectangle(frame, contentRect),
                frame.customStyle,
                BlockCombinationStyle::commonBoxFrame(),
                frame.color);
        } else {
            _buffer.drawFrame(frameRectangle(frame, contentRect), frame.style, frame.color);
        }
    }
}

auto FrameWeaverApp::createRandomFrame() -> FrameSpec {
    auto factorDistribution = std::uniform_real_distribution<double>{0.0, 1.0};
    auto widthDistribution = std::uniform_real_distribution<double>{0.20, 0.78};
    auto heightDistribution = std::uniform_real_distribution<double>{0.20, 0.72};
    auto styles = availableStyles();
    auto styleDistribution = std::uniform_int_distribution<std::size_t>{0, styles.size() - 1};
    const auto &colorPalette = colors();
    auto colorDistribution = std::uniform_int_distribution<std::size_t>{0, colorPalette.sequenceLength() - 1};
    auto frame = FrameSpec{
        .x = factorDistribution(_rng),
        .y = factorDistribution(_rng),
        .width = widthDistribution(_rng),
        .height = heightDistribution(_rng),
        .color = colorPalette.color(colorDistribution(_rng)),
    };
    const auto &style = styles[styleDistribution(_rng)];
    frame.style = style.style;
    frame.customStyle = style.customStyle;
    return frame;
}

auto FrameWeaverApp::frameRectangle(const FrameSpec frame, const Rectangle contentRect) -> Rectangle {
    const auto width = std::clamp(
        static_cast<int>(std::lround(frame.width * static_cast<double>(contentRect.width().toRawValue()))),
        4,
        std::max(4, contentRect.width().toRawValue()));
    const auto height = std::clamp(
        static_cast<int>(std::lround(frame.height * static_cast<double>(contentRect.height().toRawValue()))),
        3,
        std::max(3, contentRect.height().toRawValue()));
    const auto maxX = std::max(0, contentRect.width().toRawValue() - width);
    const auto maxY = std::max(0, contentRect.height().toRawValue() - height);
    const auto x = contentRect.x1() + static_cast<int>(std::lround(frame.x * static_cast<double>(maxX)));
    const auto y = contentRect.y1() + static_cast<int>(std::lround(frame.y * static_cast<double>(maxY)));
    return {Coordinate{x}, Coordinate{y}, Coordinate{width}, Coordinate{height}};
}

auto FrameWeaverApp::prismFrameStyle() -> const Block16StylePtr & {
    static const auto style = std::make_shared<Block16Style>(std::array<Block, 16>{
        Block{U'∙'},
        Block{U'╶'},
        Block{U'╷'},
        Block{U'←', fg::BrightRed, bg::Red},
        Block{U'╴'},
        Block{U'─'},
        Block{U'→', fg::BrightBlue, bg::Blue},
        Block{U'┬'},
        Block{U'╵'},
        Block{U'↓', fg::BrightRed, bg::Red},
        Block{U'│'},
        Block{U'├'},
        Block{U'↓', fg::BrightBlue, bg::Blue},
        Block{U'┴'},
        Block{U'┤'},
        Block{U'┼'},
    });
    return style;
}

auto FrameWeaverApp::colors() -> const ColorSequence & {
    static const auto cColors = ColorSequence{
        {fg::BrightCyan, bg::Black},
        {fg::BrightMagenta, bg::Black},
        {fg::BrightYellow, bg::Black},
        {fg::BrightGreen, bg::Black},
        {fg::BrightBlue, bg::Black},
    };
    return cColors;
}

auto FrameWeaverApp::availableStyles() const -> std::vector<FrameSpec> {
    switch (_styleMode) {
    case StyleMode::Light:
        return {FrameSpec{.style = FrameStyle::Light}};
    case StyleMode::Double:
        return {FrameSpec{.style = FrameStyle::Double}};
    case StyleMode::Heavy:
        return {FrameSpec{.style = FrameStyle::Heavy}};
    case StyleMode::Mixed:
        return {
            FrameSpec{.style = FrameStyle::Light},
            FrameSpec{.style = FrameStyle::Double},
            FrameSpec{.style = FrameStyle::Heavy},
        };
    case StyleMode::Block:
        return {
            FrameSpec{.style = FrameStyle::FullBlock},
            FrameSpec{.style = FrameStyle::FullBlockWithChamfer},
            FrameSpec{.style = FrameStyle::OuterHalfBlock},
            FrameSpec{.style = FrameStyle::InnerHalfBlock},
        };
    case StyleMode::Custom:
        return {FrameSpec{.customStyle = prismFrameStyle()}};
    case StyleMode::All:
    default:
        return {
            FrameSpec{.style = FrameStyle::Light},
            FrameSpec{.style = FrameStyle::LightWithRoundedCorners},
            FrameSpec{.style = FrameStyle::LightDoubleDash},
            FrameSpec{.style = FrameStyle::LightTripleDash},
            FrameSpec{.style = FrameStyle::LightQuadrupleDash},
            FrameSpec{.style = FrameStyle::Heavy},
            FrameSpec{.style = FrameStyle::HeavyDoubleDash},
            FrameSpec{.style = FrameStyle::HeavyTripleDash},
            FrameSpec{.style = FrameStyle::HeavyQuadrupleDash},
            FrameSpec{.style = FrameStyle::Double},
            FrameSpec{.style = FrameStyle::FullBlock},
            FrameSpec{.style = FrameStyle::FullBlockWithChamfer},
            FrameSpec{.style = FrameStyle::OuterHalfBlock},
            FrameSpec{.style = FrameStyle::InnerHalfBlock},
            FrameSpec{.customStyle = prismFrameStyle()},
        };
    }
}

auto FrameWeaverApp::modeName() const -> el::String {
    switch (_styleMode) {
    case StyleMode::Light:
        return "light"_el;
    case StyleMode::Double:
        return "double"_el;
    case StyleMode::Heavy:
        return "heavy"_el;
    case StyleMode::Mixed:
        return "mixed"_el;
    case StyleMode::Block:
        return "block"_el;
    case StyleMode::Custom:
        return "custom"_el;
    case StyleMode::All:
    default:
        return "all"_el;
    }
}

auto FrameWeaverApp::buildPrompt() const -> BlockString {
    auto result = BlockStringEditor{};
    result.append(
        fg::BrightYellow,
        bg::BrightBlack,
        "[Q]"_el,
        fg::BrightWhite,
        " quit  "_el,
        fg::BrightCyan,
        "[F]/[S]"_el,
        fg::BrightWhite,
        " speed  "_el,
        fg::BrightMagenta,
        "[C]"_el,
        fg::BrightWhite,
        " clear  "_el,
        fg::BrightGreen,
        "[1]"_el,
        fg::BrightWhite,
        " light  "_el,
        fg::BrightGreen,
        "[2]"_el,
        fg::BrightWhite,
        " double  "_el,
        fg::BrightGreen,
        "[3]"_el,
        fg::BrightWhite,
        " heavy  "_el,
        fg::BrightGreen,
        "[4]"_el,
        fg::BrightWhite,
        " mixed  "_el,
        fg::BrightGreen,
        "[5]"_el,
        fg::BrightWhite,
        " block  "_el,
        fg::BrightGreen,
        "[6]"_el,
        fg::BrightWhite,
        " custom  "_el,
        fg::BrightGreen,
        "[7]"_el,
        fg::BrightWhite,
        " all"_el);
    return result;
}

}
