// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PiView.hpp"

#include <erbsland/Alignment.hpp>
#include <erbsland/block/Coordinate.hpp>
#include <erbsland/block/Position.hpp>
#include <erbsland/block/Rectangle.hpp>
#include <erbsland/block/Size.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/cterm/BlockText.hpp>
#include <erbsland/cterm/Input.hpp>
#include <erbsland/IntegerFormat.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringCharReader.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>
#include <utility>

namespace demo {

using namespace el::text::literals;
using namespace el::cterm;
using el::block::Coordinate;
using el::block::Rectangle;

PiView::PiView(TerminalPtr terminal, std::shared_ptr<DigitHistory> history) :
    _terminal{std::move(terminal)}, _history{std::move(history)} {
    if (!_terminal || !_history) {
        throw el::ParameterFailure{"A pi view needs a terminal and history."_el, "view"_el};
    }
    _settings.setMinimumSize(el::block::Size{72, 10});
    _settings.setMinimumSizeMessage(BlockString{"Resize to at least 72x10 cells."_el});
}

void PiView::setSession(std::weak_ptr<PiSession> session) {
    _session = std::move(session);
}

void PiView::onUpdated() {
    _dirty = true;
}

auto PiView::pollKeys() -> Action {
    for (int count = 0; count < 16; ++count) {
        const auto key = _terminal->input().readKey(el::Milliseconds{0});
        if (!key.valid()) {
            break;
        }
        const auto action = handleKey(key);
        if (action != Action::None) {
            return action;
        }
    }
    return Action::None;
}

auto PiView::handleKey(const Key &key) -> Action {
    _dirty = true;
    if (key == U'q' || key == U'Q' || key == Key::Escape) {
        return Action::Quit;
    }
    if (key == U's' || key == U'S') {
        return Action::Stop;
    }
    if (key == Key::Up) {
        _viewport.up(Coordinate{1});
    } else if (key == Key::Down) {
        _viewport.down(Coordinate{1});
    } else if (key == Key::PageUp) {
        _viewport.up(_viewport.visibleLines());
    } else if (key == Key::PageDown) {
        _viewport.down(_viewport.visibleLines());
    } else if (key == Key::Home) {
        _viewport.home();
    } else if (key == Key::End) {
        _viewport.end();
    } else if (key == U'f' || key == U'F') {
        _viewport.follow();
    }
    return Action::None;
}

auto PiView::statusText() const -> el::String {
    if (const auto session = _session.lock()) {
        if (session->isFinished()) {
            return "Stopped - browse the retained digits"_el;
        }
        if (session->isStopping()) {
            return "Stopping calculation"_el;
        }
    }
    return "Calculating"_el;
}

auto PiView::keyboardHelp() const -> BlockString {
    auto mode = "manual"_el;
    if (_viewport.isFollowing()) {
        mode = "follow"_el;
    }
    auto result = BlockStringEditor{};
    result.append(
        colorizeKeys("[S] Stop "_el),
        colorizeKeys("[Q/Esc] Quit "_el),
        colorizeKeys("[↑/↓/PgUp/PgDown/Home/End] Scroll "_el),
        colorizeKeys("[F] Follow "_el),
        fg::BrightBlack,
        "("_el,
        fg::Yellow,
        mode,
        fg::BrightBlack,
        ")"_el);
    return result;
}

auto PiView::colorizeKeys(const el::String &keyboardString) -> BlockString {
    auto reader = el::StringCharReader{keyboardString};
    auto result = BlockStringEditor{};
    bool insideKey = false;
    auto style = BlockStyle{};
    while (!reader.isAtEnd()) {
        auto c = reader.read();
        if (c == U'[') {
            insideKey = true;
            style.setFg(fg::BrightBlack);
        } else if (c == U']') {
            insideKey = false;
            style.setFg(fg::BrightBlack);
        } else if (c == U'/') {
            style.setFg(insideKey ? fg::BrightBlack : fg::White);
        } else {
            style.setFg(insideKey ? fg::BrightGreen : fg::White);
        }
        result.append(BlockCount{1}, c, style);
    }
    return result;
}

void PiView::render() {
    // prepare the formats.
    static const auto headerFormat = el::StringFormat{"Pi = 3. │ {} fractional digits │ {}"_el};
    static const auto lineNumberFormat = el::IntegerFormat{}.setFieldWidth(el::CpCount{8});
    // React on screen resize.
    _terminal->testScreenSize();
    const auto size = _terminal->size();
    if (_buffer.size() != size) {
        _buffer.resize(size);
        _dirty = true;
    }
    if (!_dirty) {
        return;
    }
    _dirty = false;
    _buffer.fill(Block{U' ', fg::White, bg::Black});
    const auto rows = std::max(size.height() - Coordinate{4}, Coordinate{1});
    _viewport.update(_history->count(), rows);
    _buffer.drawBlockText(
        el::block::Position{},
        BlockString{headerFormat.build(_history->count(), statusText())}.withBase({fg::BrightCyan, bg::Black}));
    // draw all rows to the view
    for (Coordinate row = {}; row < rows; ++row) {
        const auto viewPortOffset = _viewport.firstLine() + row;
        const auto begin = el::CpIndex::end(DigitViewport::cDigitsPerLine * viewPortOffset.toSizeT());
        const auto digits = _history->range({begin, DigitViewport::cDigitsPerLine});
        auto line = BlockStringEditor{};
        // add the line number.
        auto lineNumber = el::String::fromInteger(begin.incremented().toRawValue(), lineNumberFormat)
                              .aligned(el::CpCount{8}, el::Alignment::Right);
        line.append(fg::Green, lineNumber, fg::BrightBlack, " | "_el);
        auto index = el::CpIndex{};
        const auto digitsLength = digits.characterLength();
        const auto groupSize = el::CpCount{10U};
        while (index.isWithin(digitsLength)) {
            if (!index.isZero()) {
                line.append(" "_el);
            }
            line.appendStyled(digits.slice(el::CpRange{index, groupSize}), {fg::BrightYellow, bg::Inherited});
            index += groupSize;
        }
        _buffer.drawBlockText({Coordinate{0}, Coordinate{row + 2}}, line);
    }
    _buffer.drawBlockText({Coordinate{0}, size.height() - Coordinate{1}}, keyboardHelp());
    _terminal->updateScreen(_buffer, _settings);
}

}
