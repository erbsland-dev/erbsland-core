// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TerminalApplication.hpp"

namespace demo {

void TerminalApplication::initialize() {
    enableTerminal();
    terminal()->input().setMode(Input::Mode::Key);
    _updateSettings.setMinimumSize(BlockSize{BlockCoordinate{60}, BlockCoordinate{20}});
    _updateSettings.setMinimumSizeBackground(Block{U' ', bg::Black});
    _updateSettings.setMinimumSizeMessage(
        BlockString{"Resize the terminal to at least 60x20 cells for this demo."_el, Color{fg::BrightRed, bg::Black}});
    beforeInitialize();
}

auto TerminalApplication::main() -> erbsland::ExitCode {
    if (auto exitCode = beforeMain(); exitCode != 0) {
        return el::ExitCode{exitCode};
    }
    while (!_quitRequested) {
        const auto key = terminal()->input().readKey(loopInterval());
        if (key.valid()) {
            onKey(key);
        }
        terminal()->testScreenSize();
        if (_buffer.size() != terminal()->size()) {
            _buffer.resize(terminal()->size());
            onResize();
        }
        onRenderToBuffer();
        const auto updateStarted = std::chrono::steady_clock::now();
        terminal()->updateScreen(_buffer, _updateSettings);
        onAfterUpdateScreen(std::chrono::steady_clock::now() - updateStarted);
        ++_animationCycle;
    }
    return el::ExitCode::success();
}

void TerminalApplication::beforeInitialize() {
    // empty
}

auto TerminalApplication::beforeMain() -> int {
    return 0; // empty
}

void TerminalApplication::onShutdown() {
    // empty
}

void TerminalApplication::onResize() {
    // empty
}

void TerminalApplication::onRenderToBuffer() {
    // empty
}

void TerminalApplication::onAfterUpdateScreen([[maybe_unused]] const std::chrono::nanoseconds duration) {
    // empty
}

void TerminalApplication::onKey(const Key &key) {
    if (key == U'q') {
        _quitRequested = true;
    }
}

auto TerminalApplication::loopInterval() const noexcept -> std::chrono::milliseconds {
    return std::chrono::milliseconds{90};
}

}
