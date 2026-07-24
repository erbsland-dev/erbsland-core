// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharacterClassState.hpp"
#include "PatternNode.hpp"
#include "QuantifierHandler.hpp"

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

namespace erbsland::re::impl::parser {

inline void handleCharacterClass(ParserState &state) {
    state.readNext(); // consume the initial `[`

    // Parse the contents of the character class.
    CharClassHandlerState handlerState{state};
    handlerState.parse();
    auto [ranges, categories, isNegated] = std::move(handlerState).takeResult();
    Category::normalizeList(categories);

    // After parsing, decide how to model it in the AST
    if (ranges.empty()) {
        if (categories.empty()) {
            state.throwParsingError("An empty character class '[]' is not allowed"_el);
        }
        state.addNodeData(node_data::CharacterCategory{categories, isNegated});
        return;
    }
    // As soon we have custom ranges, the fastest approach is using a `CharClass` object.
    PatternNodePtr newNode;
    if (categories.empty()) {
        auto charClass = CharClass{ranges};
        charClass.prepareForUse();
        newNode = state.createNode(node_data::CharacterClass{std::move(charClass), isNegated});
    } else {
        // Add all ranges from the Unicode categories to the existing ranges.
        for (const auto &category : categories) {
            const auto characterSet = category.characterSet();
            characterSet.forEach(
                [&ranges](const text::CharRange range) -> void { ranges.emplace_back(range.from(), range.to()); });
        }
        auto charClass = CharClass{ranges};
        charClass.prepareForUse();
        newNode = state.createNode(node_data::CharacterClass{std::move(charClass), isNegated});
    }
    state.addNode(handleQuantifier(state, newNode));
}

}
