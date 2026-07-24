// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadLine.hpp"

#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../text/UnicodeCategory.hpp"
#include "../../unit/ElementCount.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::cterm::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

ReadLine::ReadLine(TerminalPtr terminal, ReadLineOptions options) :
    ReadLine{
        std::move(terminal), std::move(options), []() noexcept -> time::TimePoint { return time::TimePoint::now(); }} {
}

ReadLine::ReadLine(TerminalPtr terminal, ReadLineOptions options, NowFn nowFn) :
    ReadLineBase{std::move(terminal), std::move(options), std::move(nowFn)} {
    normalizeConfiguredText();
}

ReadLine::~ReadLine() {
    stop();
}

auto ReadLine::update() -> ReadLineResult {
    return result(updateBase());
}

auto ReadLine::waitForInput() -> ReadLineResult {
    return result(waitForInputBase());
}

void ReadLine::normalizeConfiguredText() {
    _initialText = normalizedText(options().currentText());
    _history.clear();
    _history.reserve(options().history().count());
    for (const auto &entry : options().history()) {
        _history.append(U32String{normalizedText(entry)});
    }
}

auto ReadLine::normalizedText(const String &value) const -> U32StringEditor {
    auto source = U32StringEditor{StringConverter{value}.toU32String()};
    const auto lineBreaks = CharSet{Char{U'\n'}};
    source.removeAll(CharSet::from(UnicodeCategory::Control) - lineBreaks);
    auto lines = U32StringList::fromSplit(U32String{source}, lineBreaks, ElementCount::infinite(), true);
    const auto maximumLineCount = ElementCount::fromSizeT(options().maximumLines().toSizeT());
    if (lines.count() > maximumLineCount) {
        lines.resize(maximumLineCount);
    }
    auto result = lines.join(U"\n"_el);
    if (result.length() > options().maximumLength()) {
        result = result.slice(StringSide::Front, options().maximumLength());
    }
    return U32StringEditor{result};
}

auto ReadLine::textAsString() const -> String {
    return StringConverter{_text}.toString();
}

void ReadLine::resetText() {
    _text = _initialText;
    _historyIndex.reset();
    _historyDraft = _text;
    _historyDraftCursor = CpIndex::end(_text.length());
    _committedText = {};
}

void ReadLine::discardText() noexcept {
    _text = {};
    _historyIndex.reset();
    _historyDraft = {};
    _historyDraftCursor = {};
}

void ReadLine::commitText() {
    _committedText = textAsString();
    appendCommittedHistory();
}

auto ReadLine::insertKeyText(const Key &key, const CpIndex index) -> CpLength {
    if (!key.modifiers().empty()) {
        return {};
    }
    auto value = U32String{};
    if (key.type() == Key::Space) {
        value = U32StringEditor{U" "_el};
    } else if (key.type() == Key::Character || key.type() == Key::Combined) {
        value = key.combined();
    } else {
        return {};
    }
    if (value.isEmpty() || _text.length() + value.length() > options().maximumLength()) {
        return {};
    }
    for (auto characterIndex = CpIndex{}; characterIndex < CpIndex::end(value.length()); ++characterIndex) {
        if (value.charAt(characterIndex).isControl()) {
            return {};
        }
    }
    _text.insert(index, value);
    _historyIndex.reset();
    return value.length();
}

auto ReadLine::insertNewLine(const CpIndex index) -> bool {
    if (logicalLineCount() >= options().maximumLines().toSizeT() || _text.length() >= options().maximumLength()) {
        return false;
    }
    _text.insert(index, U32StringEditor{U"\n"_el});
    _historyIndex.reset();
    return true;
}

void ReadLine::eraseText(const CpIndex index, const CpLength length) noexcept {
    _text.remove(CpRange{index, length});
    _historyIndex.reset();
}

auto ReadLine::previousUnitStart(CpIndex index) const noexcept -> CpIndex {
    if (index.isZero()) {
        return {};
    }
    --index;
    if (_text.charAt(index) == U'\n') {
        return index;
    }
    while (!index.isZero() && _text.charAt(index).displayWidth() == 0) {
        --index;
    }
    return index;
}

auto ReadLine::nextUnitEnd(CpIndex index) const noexcept -> CpIndex {
    const auto end = CpIndex::end(_text.length());
    if (index >= end) {
        return end;
    }
    ++index;
    while (index < end && _text.charAt(index) != U'\n' && _text.charAt(index).displayWidth() == 0) {
        ++index;
    }
    return index;
}

auto ReadLine::logicalLineCount() const noexcept -> std::size_t {
    auto result = std::size_t{1U};
    for (auto index = CpIndex{}; index < CpIndex::end(_text.length()); ++index) {
        if (_text.charAt(index) == U'\n') {
            ++result;
        }
    }
    return result;
}

auto ReadLine::selectPreviousHistory() -> bool {
    if (_history.isEmpty()) {
        return false;
    }
    if (!_historyIndex.has_value()) {
        _historyDraft = _text;
        _historyDraftCursor = cursorIndex();
        _historyIndex = ElementIndex::end(_history.count()) - ElementCount::one();
    } else if (!_historyIndex->isZero()) {
        --*_historyIndex;
    } else {
        return false;
    }
    loadHistoryEntry(*_historyIndex);
    return true;
}

auto ReadLine::selectNextHistory() -> bool {
    if (!_historyIndex.has_value()) {
        return false;
    }
    if (*_historyIndex + ElementCount::one() < ElementIndex::end(_history.count())) {
        ++*_historyIndex;
        loadHistoryEntry(*_historyIndex);
    } else {
        _historyIndex.reset();
        _text = _historyDraft;
        setCursorIndex(std::min(_historyDraftCursor, CpIndex::end(_text.length())));
    }
    resetPreferredColumn();
    return true;
}

void ReadLine::loadHistoryEntry(const ElementIndex index) {
    _text = U32StringEditor{_history[index]};
    setCursorIndex(CpIndex::end(_text.length()));
    resetPreferredColumn();
}

void ReadLine::appendCommittedHistory() {
    if (_committedText.isEmpty()) {
        return;
    }
    const auto committed = U32String{_text};
    if (_history.isEmpty() || _history.last() != committed) {
        _history.append(committed);
    }
}

auto ReadLine::result(const ReadLineStatus status) const -> ReadLineResult {
    return status == ReadLineStatus::Committed ? ReadLineResult{status, _committedText}
                                               : ReadLineResult{status, String{}};
}

}
