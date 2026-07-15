// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RetroPlasmaApp.hpp"

#include <algorithm>

namespace demo {

void RetroPlasmaApp::beforeInitialize() {
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{28}, BlockCoordinate{8}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{
            "Resize the terminal to at least 28x8 cells for the plasma demo."_el, Color{fg::BrightWhite, bg::Black}});
}

auto RetroPlasmaApp::beforeMain() -> int {
    _lastFrameTime = std::chrono::steady_clock::now();
    return 0;
}

void RetroPlasmaApp::onKey(const Key &key) {
    if (key == U'f') {
        _speed = std::min(_speed + 0.25, 4.0);
    } else if (key == U's') {
        _speed = std::max(_speed - 0.25, 0.25);
    } else if (key == U'p' || key == Key::Space) {
        _paused = !_paused;
    } else if (key == U'c') {
        ++_paletteIndex;
    } else {
        TerminalApplication::onKey(key);
    }
}

void RetroPlasmaApp::onRenderToBuffer() {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsedSeconds = std::chrono::duration<double>{now - _lastFrameTime}.count();
    _lastFrameTime = now;
    if (!_paused) {
        _phase += elapsedSeconds * (_speed * 2.7);
    }
    _buffer.fill(Block{U' ', fg::Default, bg::Black});
    const auto contentHeight = std::max(BlockCoordinate{0}, _buffer.size().height() - 1);
    _renderer.render(
        _buffer,
        BlockRectangle{BlockCoordinate{0}, BlockCoordinate{0}, _buffer.size().width(), contentHeight},
        _phase,
        _paletteIndex);
    drawPrompt();
}

void RetroPlasmaApp::drawPrompt() noexcept {
    if (_buffer.size().height() <= 0) {
        return;
    }
    const auto promptRow = _buffer.size().height() - 1;
    const auto promptRect = BlockRectangle{BlockCoordinate{0}, promptRow, _buffer.size().width(), BlockCoordinate{1}};
    _buffer.fill(promptRect, Block{U' ', bg::BrightBlack});
    auto text = BlockText{buildPrompt(), promptRect, Alignment::CenterLeft};
    _buffer.drawBlockText(text);
}

auto RetroPlasmaApp::buildPrompt() const -> BlockString {
    auto result = BlockString{};
    result.append(
        bg::BrightBlack,
        fg::BrightWhite,
        "Retro Plasma  "_el,
        fg::BrightYellow,
        "[Q]"_el,
        fg::BrightWhite,
        " quit  "_el,
        fg::BrightCyan,
        "[F]"_el,
        fg::BrightWhite,
        " faster  "_el,
        fg::BrightCyan,
        "[S]"_el,
        fg::BrightWhite,
        " slower  "_el,
        fg::BrightMagenta,
        "[P]"_el,
        fg::BrightWhite,
        (_paused ? " resume  "_el : " pause  "_el),
        fg::BrightGreen,
        "[C]"_el,
        fg::BrightWhite,
        el::StringFormat{" palette  speed {}%"_el}.build(_speed * 100.0));
    return result;
}

}
