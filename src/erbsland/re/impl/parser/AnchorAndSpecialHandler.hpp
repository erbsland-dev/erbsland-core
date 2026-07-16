// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserState.hpp"
#include "QuantifierHandler.hpp"

namespace erbsland::re::impl::parser {

/// Handle anchors.
inline void handleAnchor(ParserState &state, const TextAnchor textAnchor) {
    state.readNext();
    state.addNodeData(node_data::Anchor{textAnchor});
}

/// Handle dot
inline void handleDot(ParserState &state) {
    state.readNext();
    PatternNodePtr newNode;
    if (state.currentFlags().isSet(GroupFlag::DotAll)) {
        newNode = state.createNode(node_data::CharacterCategory{Category::AnyDotAll, false});
    } else {
        newNode = state.createNode(node_data::CharacterCategory{Category::Any, false});
    }
    // Test if repetition follows the dot.
    state.currentSequence()->addChild(handleQuantifier(state, newNode));
}

/// Handle begin of line.
inline void handleBeginOfLine(ParserState &state) {
    if (state.currentFlags().isSet(GroupFlag::Multiline)) {
        handleAnchor(state, TextAnchor::LineStart);
    } else {
        handleAnchor(state, TextAnchor::Start);
    }
}

/// Handle end of line.
inline void handleEndOfLine(ParserState &state) {
    if (state.currentFlags().isSet(GroupFlag::Multiline)) {
        handleAnchor(state, TextAnchor::LineEnd);
    } else {
        handleAnchor(state, TextAnchor::End);
    }
}

/// Handle a comment in verbose mode
inline void handleVerboseComment(ParserState &state) {
    state.readNext(); // consume '#'
    while (state.currentChar() != U'\n' && !state.isAtEnd()) {
        state.readNext();
    }
}

}
