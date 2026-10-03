// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalEmulation.hpp"

#include "../err/LogicFailure.hpp"
#include "../text/Literals.hpp"
#include "../unit/ByteRange.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::cterm {

using namespace text::literals;
using block::Coordinate;
using block::Position;

TerminalEmulation::TerminalEmulation() : TerminalEmulation{TerminalEmulationOptions{}} {
}

TerminalEmulation::TerminalEmulation(TerminalEmulationOptions options) :
    _options{std::move(options)}, _regular{_options.size()}, _alternate{_options.size()} {
    reset();
}

auto TerminalEmulation::activeBuffer() const noexcept -> const CursorBuffer & {
    if (_alternateActive) {
        return _alternate;
    }
    return _regular;
}

auto TerminalEmulation::buffer() noexcept -> CursorBuffer & {
    if (_alternateActive) {
        return _alternate;
    }
    return _regular;
}

void TerminalEmulation::reset() {
    _decoder.reset();
    resetState();
}

void TerminalEmulation::resetState() {
    _regular = CursorBuffer{_options.size()};
    _alternate = CursorBuffer{_options.size()};
    clearScrollback();
    _regular.setScrollOutCallback([this](const ReadableBuffer &line) noexcept { appendScrollback(line); });
    _regular.setAutoWrap(_options.autoWrap());
    _alternate.setAutoWrap(_options.autoWrap());
    _state = ParserState::Text;
    _parameters.clear();
    _decSaved = {};
    _scoSaved = {};
    _regularSaved = {};
    _alternateActive = false;
    _privateMode = false;
    _invalidCsi = false;
    _osc = false;
    _finished = false;
}

void TerminalEmulation::write(const text::String &input) {
    if (_finished) {
        throw err::LogicFailure{"Cannot write after finishing terminal emulation."_el};
    }
    auto offset = unit::ByteIndex{0};
    while (offset.distanceFromZero() < input.length()) {
        const auto count = std::min(_decoder.availableSpace(), input.length() - offset.distanceFromZero());
        _decoder.writeStringBytes(input.slice(unit::ByteRange{offset, count}));
        offset += count;
        drain();
    }
}

void TerminalEmulation::finish() {
    if (_finished) {
        return;
    }
    _decoder.finish();
    drain();
    _state = ParserState::Text;
    _parameters.clear();
    _finished = true;
}

void TerminalEmulation::drain() {
    while (const auto character = _decoder.readChar()) {
        consume(*character);
    }
}

void TerminalEmulation::consume(const text::Char character) {
    const auto value = character.toRawValue();
    if (value == 24 || value == 26) {
        _state = ParserState::Text;
        return;
    }
    if (_state == ParserState::StringControl || _state == ParserState::StringEscape) {
        consumeStringControl(value);
        return;
    }
    if (consumeControl(value)) {
        return;
    }
    switch (_state) {
    case ParserState::Text:
        consumeText(character);
        break;
    case ParserState::Escape:
        consumeEscape(value);
        break;
    case ParserState::Csi:
        consumeCsi(value);
        break;
    case ParserState::EscapeIntermediate:
        if (value >= 0x30 && value <= 0x7e) {
            _state = ParserState::Text;
        }
        break;
    case ParserState::StringControl:
    case ParserState::StringEscape:
        break;
    }
}

void TerminalEmulation::clearScrollback() {
    _scrollback = RemappedBuffer{block::Size{_options.size().width(), block::Coordinate{1}}};
    _hasScrollback = false;
}

void TerminalEmulation::appendScrollback(const ReadableBuffer &line) noexcept {
    if (_hasScrollback) {
        if (_scrollback.size().height() < _options.scrollbackLimit()) {
            _scrollback.resize(
                _scrollback.size() + block::Size{0, 1}, BufferResizeMode::PreserveContent, Block::space());
        } else {
            _scrollback.shift(block::Direction::North);
        }
    }
    const auto row = _scrollback.size().height() - 1;
    for (auto column = block::Coordinate{0}; column < line.size().width(); ++column) {
        _scrollback.set({column, row}, line.get({column, block::Coordinate{0}}));
    }
    _hasScrollback = true;
}

void TerminalEmulation::consumeText(const text::Char character) {
    const auto value = character.toRawValue();
    auto &target = buffer();
    if (value == 13) {
        target.moveTo({Coordinate{0}, target.position().y()});
    } else if (value == 10 || value == 11 || value == 12) {
        lineFeed();
    } else if (value == 8) {
        target.moveLeft(Coordinate{1});
    } else if (value == 9) {
        const auto tab = _options.tabWidth().toRawValue();
        target.moveTo({Coordinate{(target.position().x().toRawValue() / tab + 1) * tab}, target.position().y()});
    } else if (value >= 32 && value != 127) {
        if (character.displayWidth() == 0) {
            auto previous = target.position();
            if (!target.wrapPending()) {
                previous -= Position{1, 0};
            }
            if (previous.x() >= 0) {
                if (target.get(previous).displayWidth() == 0) {
                    previous -= Position{1, 0};
                }
                if (previous.x() >= 0) {
                    target.set(previous, target.get(previous).withCombining(character));
                }
            }
        } else {
            target.write(Block{character});
        }
    }
}

void TerminalEmulation::lineFeed() {
    const auto column = buffer().position().x();
    buffer().writeLineBreak();
    if (!_options.lineFeedReturnsToColumnZero()) {
        buffer().moveTo({column, buffer().position().y()});
    }
}

auto TerminalEmulation::screenIndex() const noexcept -> std::size_t {
    if (_alternateActive) {
        return 1;
    }
    return 0;
}

auto TerminalEmulation::savedState() const -> impl::EmulationCursorState {
    const auto &target = activeBuffer();
    return {target.position(), target.style(), target.wrapPending()};
}

void TerminalEmulation::restoreState(const impl::EmulationCursorState &state) {
    buffer().restorePosition(state.position, state.wrapPending);
    buffer().setStyle(state.style);
}

auto TerminalEmulation::parameter(const std::size_t index, const int fallback) const -> int {
    if (index >= _parameters.size() || _parameters[index] == 0) {
        return fallback;
    }
    return _parameters[index];
}

}
