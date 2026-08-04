// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParagraphOptions.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"
#include "../text/u32/U32StringConstIterator.hpp"

namespace erbsland::cterm {

using namespace text::literals;

auto ParagraphOptions::alignment() const noexcept -> bgeo::Alignment {
    return _alignment;
}

void ParagraphOptions::setAlignment(const bgeo::Alignment alignment) noexcept {
    _alignment = alignment;
}

auto ParagraphOptions::indents() const noexcept -> const ParagraphIndents & {
    return _indents;
}

void ParagraphOptions::setIndents(const ParagraphIndents &indents) noexcept {
    _indents = indents;
}

auto ParagraphOptions::lineIndent() const noexcept -> int {
    return _indents.lineIndent();
}

void ParagraphOptions::setLineIndent(const int indent) noexcept {
    _indents.setLineIndent(indent);
}

auto ParagraphOptions::firstLineIndent() const noexcept -> int {
    return _indents.firstLineIndent();
}

void ParagraphOptions::setFirstLineIndent(const int indent) noexcept {
    _indents.setFirstLineIndent(indent);
}

auto ParagraphOptions::wrappedLineIndent() const noexcept -> int {
    return _indents.wrappedLineIndent();
}

void ParagraphOptions::setWrappedLineIndent(const int indent) noexcept {
    _indents.setWrappedLineIndent(indent);
}

auto ParagraphOptions::margins() const noexcept -> const bgeo::BlockMargins & {
    return _indents.margins();
}

void ParagraphOptions::setMargins(const bgeo::BlockMargins margins) noexcept {
    _indents.setMargins(margins);
}

auto ParagraphOptions::backgroundMode() const noexcept -> ParagraphBackgroundMode {
    return _backgroundMode;
}

void ParagraphOptions::setBackgroundMode(const ParagraphBackgroundMode backgroundMode) noexcept {
    _backgroundMode = backgroundMode;
}

auto ParagraphOptions::lineBreakEndMark() const noexcept -> const BlockString & {
    return _lineBreakEndMark;
}

void ParagraphOptions::setLineBreakEndMark(BlockString mark) {
    if (mark.length() > BlockCount{2U}) {
        throw err::ParameterError{"Line break end mark must not exceed two characters."_el, "mark"_el};
    }
    if (mark.containsControlCharacters()) {
        throw err::ParameterError{"Line break end mark must not contain control characters."_el, "mark"_el};
    }
    _lineBreakEndMark = std::move(mark);
}

auto ParagraphOptions::lineBreakStartMark() const noexcept -> const BlockString & {
    return _lineBreakStartMark;
}

void ParagraphOptions::setLineBreakStartMark(BlockString mark) {
    if (mark.containsControlCharacters()) {
        throw err::ParameterError{"Line break start mark must not contain control characters."_el, "mark"_el};
    }
    _lineBreakStartMark = std::move(mark);
}

auto ParagraphOptions::paragraphSpacing() const noexcept -> ParagraphSpacing {
    return _paragraphSpacing;
}

void ParagraphOptions::setParagraphSpacing(const ParagraphSpacing spacing) noexcept {
    _paragraphSpacing = spacing;
}

auto ParagraphOptions::wordSeparators() const -> text::U32String {
    return _wordSeparators.toU32String();
}

auto ParagraphOptions::wordSeparatorSet() const noexcept -> const text::CharSet & {
    return _wordSeparators;
}

void ParagraphOptions::setWordSeparators(const text::U32String &separators) {
    auto characters = text::CharSet{};
    for (const auto character : separators) {
        characters.add(character);
    }
    _wordSeparators = std::move(characters);
}

auto ParagraphOptions::wordBreakMark() const noexcept -> const Block & {
    return _wordBreakMark;
}

void ParagraphOptions::setWordBreakMark(Block mark) noexcept {
    _wordBreakMark = mark;
}

auto ParagraphOptions::maximumLineWraps() const noexcept -> int {
    return _maximumLineWraps;
}

void ParagraphOptions::setMaximumLineWraps(const int lines) noexcept {
    _maximumLineWraps = std::max(lines, 0);
}

auto ParagraphOptions::paragraphEllipsisMark() const noexcept -> const BlockString & {
    return _paragraphEllipsisMark;
}

void ParagraphOptions::setParagraphEllipsisMark(BlockString mark) noexcept {
    _paragraphEllipsisMark = std::move(mark);
}

auto ParagraphOptions::tabStops() const noexcept -> const std::vector<int> & {
    return _tabStops;
}

void ParagraphOptions::setTabStops(std::vector<int> tabStops) noexcept {
    _tabStops = std::move(tabStops);
}

auto ParagraphOptions::tabOverflowBehavior() const noexcept -> TabOverflowBehavior {
    return _tabOverflowBehavior;
}

void ParagraphOptions::setTabOverflowBehavior(const TabOverflowBehavior behavior) noexcept {
    _tabOverflowBehavior = behavior;
}

auto ParagraphOptions::onError() const noexcept -> ParagraphOnError {
    return _onError;
}

void ParagraphOptions::setOnError(const ParagraphOnError onError) noexcept {
    _onError = onError;
}

auto ParagraphOptions::defaultOptions() noexcept -> const ParagraphOptions & {
    static const auto cDefaultOptions = ParagraphOptions{};
    return cDefaultOptions;
}

}
