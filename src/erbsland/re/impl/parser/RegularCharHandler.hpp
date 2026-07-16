// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "QuantifierHandler.hpp"

namespace erbsland::re::impl::parser {

/// Add a regular character.
/// This helper method is also called from `handleEscapeSequence()`.
inline void addRegularChar(ParserState &state, text::Char character) {
    state.validatePatternCharacter(character);
    if (state.currentFlags().isSet(GroupFlag::IgnoreCase)) {
        // To speed up comparison, store case-folded characters.
        character = character.caseFolded();
    }
    if (isRepetitionStart(state.currentChar())) {
        state.addNode(handleQuantifier(state, state.createNode(node_data::CharacterSequence{character})));
    } else {
        if (const auto charSeq = state.lastNode(); charSeq != nullptr && charSeq->isCharacterSequence()) {
            if (charSeq->size() >= limits::maximumCharacterSequenceLength) {
                state.throwParsingError(
                    text::StringFormat{
                        "This static character sequence is too long. The maximum allowed length is {} characters"}
                        .build(limits::maximumCharacterSequenceLength));
            }
            // Join the character with the existing character sequence.
            charSeq->addCharacter(character);
        } else {
            // Start a new character sequence.
            state.addNodeData(node_data::CharacterSequence{character});
        }
    }
}

/// Handle regular characters with no special meaning.
inline void handleRegularChar(ParserState &state) {
    const auto character = state.currentChar();
    state.readNext();
    addRegularChar(state, character);
}

}
