// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserState.hpp"
#include "QuantifierHandler.hpp"

#include "../Limits.hpp"

namespace erbsland::re::impl::parser {

inline auto isAtPatternStart(ParserState &state) noexcept -> bool {
    return state.currentGroup() == state.rootNode() && state.currentGroup()->size() == 1U &&
        state.currentSequence()->isEmpty();
}

inline void handleGroupFlags(ParserState &state) {
    auto flags = state.inheritGroupFlags();
    bool isMinus = false;
    std::size_t flagsCount = 0;
    while (isGroupFlag(state.currentChar())) {
        if (flagsCount > limits::maximumFlagCount) {
            state.throwParsingError("Too many group flags"_el);
        }
        if (isMinus) {
            switch (state.currentChar().toRawValue()) {
            case U'i':
                flags.clear(GroupFlag::IgnoreCase);
                break;
            case U'm':
                flags.clear(GroupFlag::Multiline);
                break;
            case U's':
                flags.clear(GroupFlag::DotAll);
                break;
            case U'x':
                flags.clear(GroupFlag::Verbose);
                break;
            case U'a':
                state.throwParsingError("Invalid group flag '-a', use 'u' to switch back to Unicode mode"_el);
            case U'u':
                state.throwParsingError("Invalid group flag '-u', use 'a' to switch back to ASCII mode"_el);
            case U'-':
                state.throwParsingError("Repeated '-' in group flags"_el);
            default:
                throwInternalError("Unexpected group flag"_el);
            }
        } else {
            switch (state.currentChar().toRawValue()) {
            case U'a':
                flags.set(GroupFlag::Ascii);
                break;
            case U'u':
                flags.clear(GroupFlag::Ascii);
                break;
            case U'i':
                flags.set(GroupFlag::IgnoreCase);
                break;
            case U'm':
                flags.set(GroupFlag::Multiline);
                break;
            case U's':
                flags.set(GroupFlag::DotAll);
                break;
            case U'x':
                flags.set(GroupFlag::Verbose);
                break;
            case U'-':
                isMinus = true;
                break;
            default:
                throwInternalError("Unexpected group flag"_el);
            }
        }
        state.readNext();
        flagsCount += 1;
    }
    if (state.currentChar() == U')') {
        // The `(?i)` syntax is only allowed at the start of a pattern.
        if (!isAtPatternStart(state)) {
            state.throwParsingError("Flags '(?...)' are only allowed at the start of a pattern"_el);
        }
        if (isMinus) {
            state.throwParsingError("A minus '(?-i:...)' is only allowed in group flags"_el);
        }
        // Overwrite the flags of the root group.
        std::get<node_data::Group>(state.currentGroup()->data()).flags = flags;
        state.readNext(); // consume the closing parenthesis
    } else if (state.currentChar() == U':') {
        state.readNext(); // consume the colon
        state.pushGroup(node_data::Group::createNonCapturing(flags));
    } else if (isAtPatternStart(state)) {
        state.throwParsingError("Expected ')' after the flags, or ':' to create a new group"_el);
    } else {
        state.throwParsingError("Expected ':' after the group flags"_el);
    }
}

inline void handleAtomicGroup(ParserState &state) {
    state.readNext(); // consume '>'
    state.pushGroup(node_data::Group::createAtomic(state.inheritGroupFlags(), state.nextAtomicGroupId()));
}

inline void handleGroupComment(ParserState &state) {
    while (state.currentChar() != U')') {
        if (state.isAtEnd()) {
            state.throwParsingError("Unexpected pattern end in comment"_el);
        }
        if (state.currentChar() == U'\\') { // Just handle the case of an escaped closing ')'.
            state.readNext();
            if (state.currentChar() == U')') {
                state.readNext();
            }
        } else {
            state.readNext();
        }
    }
    state.readNext(); // consume the closing ')'
}

inline void handleNonCapturingGroup(ParserState &state) {
    state.readNext(); // consume ':'
    state.pushGroup(node_data::Group::createNonCapturing(state.inheritGroupFlags()));
}

inline void handleCapturingGroup(ParserState &state) {
    state.pushGroup(node_data::Group::createCapture(state.inheritGroupFlags(), state.nextCaptureGroupIndex()));
}

inline void handleNamedGroup(ParserState &state) {
    const auto openChar = state.readNextAndExchange();
    if (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
        state.throwParsingError("A group name must not start with a digit"_el);
    }
    text::String name;
    while (state.currentChar().isAsciiWord()) {
        if (name.length().toSizeT() > limits::maximumGroupNameLength) {
            state.throwParsingError(
                text::StringFormat{"The group name exceeds {} characters"}.build(limits::maximumGroupNameLength));
        }
        name.append(state.currentChar().caseFolded());
        state.readNext();
    }
    if (state.isAtEnd()) {
        state.throwParsingError("Unexpected end of pattern in group name"_el);
    }
    if (openChar == U'\'' && state.currentChar() != U'\'') {
        state.throwParsingError("Expected closing quote after group name"_el);
    }
    if (openChar == U'<' && state.currentChar() != U'>') {
        state.throwParsingError("Expected closing angle bracket after group name"_el);
    }
    state.readNext();         // consume the closing character ('>' or '\'')
    state.addGroupName(name); // Check for duplicates
    state.pushGroup(
        node_data::Group::createCapture(state.inheritGroupFlags(), state.nextCaptureGroupIndex(), std::move(name)));
}

inline void handleGroupOpen(ParserState &state) {
    state.readNext(); // consume the open parenthesis
    // check if this is an advanced group.
    if (state.currentChar() == U'?') {
        state.readNext();
        switch (state.currentChar().toRawValue()) {
        case U'i':
        case U'm':
        case U's':
        case U'x':
        case U'a':
        case U'u':
        case U'-':
            handleGroupFlags(state);
            break;
        case U'>':
            handleAtomicGroup(state);
            break;
        case U'#':
            handleGroupComment(state);
            break;
        case U':':
            handleNonCapturingGroup(state);
            break;
        case U'P':
            state.readNext();
            if (state.currentChar() == U'=') {
                state.throwParsingError("Backreferences are not supported"_el);
            }
            if (state.currentChar() != U'<') {
                state.throwParsingError("Expected '<' after 'P' to define a named group"_el);
            }
            handleNamedGroup(state);
            break;
        case U'<':
        case U'\'':
            handleNamedGroup(state);
            break;
        case U'R':
            state.throwParsingError("Recursive groups are not supported"_el);
        case U'0':
        case U'1':
        case U'2':
        case U'3':
        case U'4':
        case U'5':
        case U'6':
        case U'7':
        case U'8':
        case U'9':
        case U'+':
        case U'&':
            state.throwParsingError("Subpattern calls are not supported"_el);
        case U'(':
            state.throwParsingError("Conditional patterns are not supported"_el);
        default:
            state.throwParsingError("Unexpected character after advanced group open sequence"_el);
        }
        return;
    }
    handleCapturingGroup(state);
}

inline void handleAlternative(ParserState &state) {
    state.checkEmptyAlternative();

    state.readNext(); // consume the '|' character introducing a new alternative
    if (state.currentGroup()->size() >= state.settings().maximumAlternativeCount()) {
        state.throwParsingError(
            text::StringFormat{"Too many alternatives. Maximum is {} alternatives"}.build(
                state.settings().maximumAlternativeCount()));
    }
    state.addSequence();
}

inline void handleGroupClose(ParserState &state) {
    state.checkEmptyGroup();
    state.checkEmptyAlternative(true);

    state.readNext(); // consume `)`
    if (state.currentGroup() == state.rootNode()) {
        state.throwParsingError("Unexpected closing parenthesis. There is no open group at this point"_el);
    }
    const auto closedGroup = state.popGroup();
    state.replaceLastNode(handleQuantifier(state, closedGroup));
}

}
