// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalEmulation.hpp"

#include <algorithm>

namespace erbsland::cterm {

void TerminalEmulation::beginCsi() {
    _state = ParserState::Csi;
    _parameters = {0};
    _privateMode = false;
    _invalidCsi = false;
}

auto TerminalEmulation::consumeControl(const char32_t value) -> bool {
    if (value == 27) {
        _state = ParserState::Escape;
        return true;
    }
    if (value == 0x9b) {
        beginCsi();
        return true;
    }
    if (value == 0x90 || value == 0x98 || value == 0x9d || value == 0x9e || value == 0x9f) {
        _state = ParserState::StringControl;
        _osc = value == 0x9d;
        return true;
    }
    if (value >= 0x80 && value <= 0x9f) {
        return true;
    }
    return false;
}

void TerminalEmulation::consumeStringControl(const char32_t value) {
    if ((_state == ParserState::StringEscape && value == U'\\') || value == 0x9c || (_osc && value == 7)) {
        _state = ParserState::Text;
    } else if (value == 27) {
        _state = ParserState::StringEscape;
    } else {
        _state = ParserState::StringControl;
    }
}

void TerminalEmulation::consumeEscape(const char32_t value) {
    _state = ParserState::Text;
    if (value == U'[') {
        beginCsi();
    } else if (value == U']' || value == U'P' || value == U'X' || value == U'^' || value == U'_') {
        _state = ParserState::StringControl;
        _osc = value == U']';
    } else if (value >= 0x20 && value <= 0x2f) {
        _state = ParserState::EscapeIntermediate;
    } else if (value == U'7') {
        _decSaved[screenIndex()] = savedState();
    } else if (value == U'8') {
        restoreState(_decSaved[screenIndex()]);
    } else if (value == U'D') {
        lineFeed();
    } else if (value == U'E') {
        buffer().writeLineBreak();
    } else if (value == U'M') {
        if (buffer().position().y() > 0) {
            buffer().moveUp(block::Coordinate{1});
        } else {
            buffer().shift(block::Direction::South, Block::space(), 1);
        }
    } else if (value == U'c') {
        resetState();
    }
}

void TerminalEmulation::consumeCsi(const char32_t value) {
    if (value >= U'0' && value <= U'9') {
        auto &number = _parameters.back();
        number = std::min(10'000, number * 10 + static_cast<int>(value - U'0'));
    } else if (value == U';') {
        if (_parameters.size() < 32) {
            _parameters.push_back(0);
        } else {
            _invalidCsi = true;
        }
    } else if (value == U'?' && _parameters.size() == 1 && _parameters.front() == 0 && !_privateMode) {
        _privateMode = true;
    } else if (value >= 0x40 && value <= 0x7e) {
        if (!_invalidCsi) {
            executeCsi(value);
        }
        _state = ParserState::Text;
    } else if (value >= 0x20) {
        _invalidCsi = true;
    }
}

}
