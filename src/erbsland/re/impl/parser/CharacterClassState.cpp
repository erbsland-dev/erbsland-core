// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharacterClassState.hpp"

namespace erbsland::re::impl::parser {

auto CharClassHandlerState::hasFeature(const Feature feature) const noexcept -> bool {
    return _state.hasFeature(feature);
}

void CharClassHandlerState::readNext() {
    _state.readNext();
}

auto CharClassHandlerState::isAtEnd() const noexcept -> bool {
    return _state.isAtEnd();
}

auto CharClassHandlerState::currentChar() const noexcept -> text::Char {
    return _state.currentChar();
}

auto CharClassHandlerState::currentFlags() const noexcept -> GroupFlags {
    return _state.currentFlags();
}

auto CharClassHandlerState::hasContent() const noexcept -> bool {
    return !_ranges.empty() || !_categories.empty();
}

void CharClassHandlerState::addCharacter(text::Char character) {
    _state.validatePatternCharacter(character);
    if (_state.currentFlags().isSet(GroupFlag::IgnoreCase)) {
        character = character.caseFolded();
    }
    _ranges.emplace_back(character, character);
}

void CharClassHandlerState::addRange(text::Char start, text::Char end) {
    _state.validatePatternCharacter(start);
    _state.validatePatternCharacter(end);
    // In ignore-case mode, use the folded (normalized to lower-case) endpoints only.
    if (_state.currentFlags().isSet(GroupFlag::IgnoreCase)) {
        // Validate mismatched ranges (e.g. [A-z], [Ä-ö], [A-ü])
        const auto cfStart = start.caseFolded();
        const auto cfEnd = end.caseFolded();
        const bool startChanges = (cfStart != start);
        const bool endChanges = (cfEnd != end);
        if (startChanges != endChanges) {
            // Exactly one endpoint changes under case folding → likely a typo/mismatched range
            throwParsingError(
                "Case folding of character range failed. Only one character of the range was case-folded"_el);
        }
        start = cfStart;
        end = cfEnd;
    }
    if (start > end) {
        throwParsingError("Character range is invalid. End character must be greater than start character"_el);
    }
    _ranges.emplace_back(start, end);
}

auto CharClassHandlerState::hasStartLiteral() const noexcept -> bool {
    return !_startLiteral.isNoCodePoint();
}

auto CharClassHandlerState::startLiteral() const noexcept -> text::Char {
    return _startLiteral;
}

void CharClassHandlerState::setStartLiteral(const text::Char literal) noexcept {
    _startLiteral = literal;
}

void CharClassHandlerState::clearStartLiteral() noexcept {
    _startLiteral = text::Char::noCodePoint();
}

auto CharClassHandlerState::isRange() const noexcept -> bool {
    return _isRange;
}

void CharClassHandlerState::setRange(const bool isRange) noexcept {
    _isRange = isRange;
}

auto CharClassHandlerState::isNegated() const noexcept -> bool {
    return _isNegated;
}

void CharClassHandlerState::setNegated(const bool negated) noexcept {
    _isNegated = negated;
}

void CharClassHandlerState::expectMore() {
    if (isAtEnd()) {
        throwParsingError("Unexpected end of pattern in '[...]' character class"_el);
    }
}

void CharClassHandlerState::handleNegation() {
    readNext();
    if (hasStartLiteral() || hasContent()) {
        throwParsingError(
            "Unexpected '^' inside of a character class. "_el
            "Please escape '^' to use it as a literal");
    }
    setNegated(true);
}

void CharClassHandlerState::handleRangeChar() {
    if (!hasStartLiteral()) { // no previous literal?
        if (hasContent()) {
            throwParsingError(
                "Unexpected '-' without preceding literal to form a range. "_el
                "Please escape '-' to use it as literal");
        }
        addCharacter(currentChar());
        readNext();
        return;
    }
    if (isRange()) {
        throwParsingError(
            "Unexpected double '--' inside of a character class range. "_el
            "Please escape '-' to use it as literal");
    }
    readNext();
    expectMore();
    if (currentChar() == U']') {
        throwParsingError(
            "Unexpected '-' at the end of the character class. "_el
            "Please escape '-' to use it as literal");
    }
    setRange(true);
}

void CharClassHandlerState::handleRangeList(const std::vector<CharRange> &ranges) {
    processStartLiteralBeforeRange();
    for (const auto &range : ranges) {
        addRange(range.first(), range.last());
    }
}

void CharClassHandlerState::addCategory(const Category category) {
    processStartLiteralBeforeRange();
    // Only add new classes
    if (std::ranges::find(_categories, category) == _categories.end()) {
        _categories.push_back(category);
    }
}

void CharClassHandlerState::processStartLiteralBeforeRange() {
    if (hasStartLiteral()) { // Process any open literals
        if (isRange()) {
            throwParsingError("A character range must not end in a character class"_el);
        }
        addCharacter(startLiteral());
        clearStartLiteral();
    }
}

void CharClassHandlerState::processNewLiteral(const text::Char character) {
    _state.validatePatternCharacter(character);
    if (hasStartLiteral()) {
        // if we already have a start...
        if (isRange()) { // we already got a `xxx-` range
            addRange(startLiteral(), character);
            clearStartLiteral();
            setRange(false);
        } else {
            // If we have a start literal, and it wasn't a range, add it as a single character.
            addCharacter(startLiteral());
            setStartLiteral(character);
        }
    } else {
        // if we have no start yet, set this as a new start.
        setStartLiteral(character);
    }
}

void CharClassHandlerState::parse() {
    while (currentChar() != U']') {
        if (currentChar() == U'^') {
            handleNegation();
            continue;
        }
        if (currentChar() == U'-') {
            handleRangeChar();
            continue;
        }
        if (currentChar() == U'[') {
            handlePosixCharacterClass();
            continue;
        }

        if (currentChar() == U'\\') {
            handleEscapeSequence();
        } else {
            processNewLiteral(currentChar());
            readNext();
        }
    }
    if (hasStartLiteral()) {
        addCharacter(startLiteral());
    }
    readNext(); // consume the ending `]`
}

}
