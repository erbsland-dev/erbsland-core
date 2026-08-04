// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "AnchorAndSpecialHandler.hpp"
#include "CharacterClassHandler.hpp"
#include "EscapeSequenceHandler.hpp"
#include "GroupHandler.hpp"
#include "RegularCharHandler.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

void Parser::handleHashCharacter(ParserState &state) {
    if (state.currentFlags().isSet(GroupFlag::Verbose)) {
        parser::handleVerboseComment(state);
    } else {
        parser::handleRegularChar(state);
    }
}

void Parser::handleSpacing(ParserState &state) {
    if (state.currentFlags().isSet(GroupFlag::Verbose)) {
        state.readNext(); // consume and ignore.
    } else {
        parser::handleRegularChar(state);
    }
}

void Parser::throwUnexpectedCharacterError(ParserState &state) {
    state.throwParsingError(
        text::StringFormat{"Found a '{}' character at an unexpected position"}.build(
            text::String::fromCharacter(state.currentChar())));
}

Parser::Parser(text::StringCharReader reader, const GroupFlags flags, Settings settings) :
    _state{std::move(reader), flags, std::move(settings)} {
}

auto Parser::parse() -> PatternNodePtr {
    preFlightChecks();
    // Start parsing.
    _state.readNext(); // initialize the current character with the first one.
    while (!_state.isAtEnd()) {
        switch (_state.currentChar().toRawValue()) {
        case U'(':
            parser::handleGroupOpen(_state);
            break;
        case U'|':
            parser::handleAlternative(_state);
            break;
        case U')':
            parser::handleGroupClose(_state);
            break;
        case U'\\':
            parser::handleEscapeSequence(_state);
            break;
        case U'[':
            parser::handleCharacterClass(_state);
            break;
        case U'.':
            parser::handleDot(_state);
            break;
        case U'^':
            parser::handleBeginOfLine(_state);
            break;
        case U'$':
            parser::handleEndOfLine(_state);
            break;
        case U'#':
            handleHashCharacter(_state);
            break;
        case U' ':
        case U'\t':
        case U'\r':
        case U'\n':
        case U'\f':
        case U'\v':
            handleSpacing(_state);
            break;
        case U'*':
        case U'+':
        case U'?':
        case U'{':
        case U'}':
        case U']':
            throwUnexpectedCharacterError(_state);
        default:
            parser::handleRegularChar(_state);
            break;
        }
    }
    if (_state.currentGroup() != _state.rootNode()) {
        _state.throwParsingError("Unclosed group at the end of the pattern"_el);
    }

    if (_state.rootNode()->size() > 1) {
        _state.checkEmptyAlternative(true);
    } else {
        _state.checkEmptyGroup();
    }

    return _state.rootNode();
}

void Parser::preFlightChecks() const {
    // atomic isn't allowed in flags.
    if (_state.currentFlags().isSet(GroupFlag::Atomic)) {
        _state.throwParsingError("The atomic flag is not allowed as initial flag"_el);
    }
}

}
