// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ParserState.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl::parser {

/// Handle optional repetition after any expression.
/// Tests for `+`, `+?`, `*`, `*?`, `?`, `??`, `{n,m}`
/// If a repetition is found, encloses the last node in the current sequence
/// @param state The current parser state.
/// @param repeatedNode The node to be repeated.
/// @return The repeated node wrapped in a repetition node, if no repetition is found, returns nullptr.
[[nodiscard]] inline auto handleQuantifier(ParserState &state, const PatternNodePtr &repeatedNode) -> PatternNodePtr {
    using node_data::Quantifier;
    if (isRepetitionStart(state.currentChar())) {
        // if the character is followed by a repetition or is optional, put it into a repetition container.
        Quantifier::Count minimum = 0;
        Quantifier::Count maximum = 1U;
        auto mode = Quantifier::Mode::Greedy;
        if (state.currentChar() == U'?') {
            state.readNext(); // 0-1 is already set.
        } else if (state.currentChar() == U'*') {
            state.readNext();
            maximum = Quantifier::infinitelyMany();
        } else if (state.currentChar() == U'+') {
            state.readNext();
            minimum = 1;
            maximum = Quantifier::infinitelyMany();
        } else if (state.currentChar() == U'{') {
            state.readNext();
            text::StringEditor minString;
            text::StringEditor maxString;
            if (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                while (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                    if (minString.length() >= unit::ByteLength{5U}) {
                        state.throwParsingError("Quantifier is too large. A maximum of five digits is allowed"_el);
                    }
                    minString.append(state.currentChar());
                    state.readNext();
                }
                if (state.currentChar() == U',') {
                    state.readNext();
                    minimum = minString.toInteger<std::uint16_t>();
                    if (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                        while (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                            if (maxString.length() >= unit::ByteLength{5U}) {
                                state.throwParsingError(
                                    "Quantifier is too large. A maximum of five digits is allowed"_el);
                            }
                            maxString.append(state.currentChar());
                            state.readNext();
                        }
                        if (state.currentChar() == U'}') {
                            state.readNext();
                            maximum = maxString.toInteger<std::uint16_t>();
                            if (maximum < minimum) {
                                state.throwParsingError(
                                    "The maximum repetition count must be greater than or equal to the minimum "_el
                                    "repetition count");
                            }
                            // success
                        } else {
                            state.throwParsingError("Expected '}' after quantifier"_el);
                        }
                    } else if (state.currentChar() == U'}') {
                        state.readNext();
                        maximum = Quantifier::infinitelyMany();
                        // success
                    } else {
                        state.throwParsingError("Expected digit or '}' after ',' in quantifier"_el);
                    }
                } else if (state.currentChar() == U'}') {
                    state.readNext();
                    minimum = minString.toInteger<std::uint16_t>();
                    maximum = minimum;
                    // success
                } else {
                    state.throwParsingError("Expected ',' or '}' after quantifier"_el);
                }
            } else if (state.currentChar() == U',') {
                state.readNext();
                minimum = 0;
                if (!state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                    state.throwParsingError("Expected digit after ',' in quantifier"_el);
                }
                if (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                    while (state.currentChar().isDigitValue(text::IntegerBase::Decimal)) {
                        if (maxString.length() >= unit::ByteLength{5U}) {
                            state.throwParsingError("Quantifier is too large. A maximum of five digits is allowed"_el);
                        }
                        maxString.append(state.currentChar());
                        state.readNext();
                    }
                    if (state.currentChar() == U'}') {
                        state.readNext();
                        maximum = maxString.toInteger<std::uint16_t>();
                        // success
                    } else {
                        state.throwParsingError("Expected '}' after quantifier"_el);
                    }
                } else {
                    state.throwParsingError("Expected digit or '}' after ',' in quantifier"_el);
                }
            } else {
                state.throwParsingError("Expected ',' or digit after '{' quantifier"_el);
            }
        }
        if (state.currentChar() == U'?') {
            state.readNext();
            mode = Quantifier::Mode::Lazy;
        } else if (state.currentChar() == U'+') {
            state.readNext();
            mode = Quantifier::Mode::Possessive;
        }
        if (minimum == 1 && maximum == 1) {
            return repeatedNode; // a{1,1} equals a
        }
        if (minimum > state.settings().maximumQuantifierCount()) {
            state.throwParsingError(
                text::StringFormat{"Minimum exceeds the maximum allowed quantifier count of {} repetitions"}.build(
                    state.settings().maximumQuantifierCount()));
        }
        if (maximum != Quantifier::infinitelyMany() && maximum > state.settings().maximumQuantifierCount()) {
            state.throwParsingError(
                text::StringFormat{"Maximum exceeds the maximum allowed quantifier count of {} repetitions"}.build(
                    state.settings().maximumQuantifierCount()));
        }

        // Calculate the counter-index based on the current quantifier nesting level (ancestor quantifiers).
        Quantifier::CounterIndex counterIndex = 0;
        auto parent = repeatedNode->parent();
        while (parent != nullptr) {
            if (parent->isQuantifier()) {
                counterIndex = std::get<Quantifier>(parent->data()).counterIndex + 1;
                break;
            }
            parent = parent->parent();
        }
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(counterIndex < limits::maximumCounterCount, "Counter index exceeds maximum"_el);

        // nest the repeated node in a new quantifier node.
        auto quantifier = Quantifier{repeatedNode, counterIndex, minimum, maximum, mode};
        if (mode == Quantifier::Mode::Possessive) {
            quantifier.atomicGroupId = state.nextAtomicGroupId();
        }
        auto newNode = state.createNode(std::move(quantifier));
        repeatedNode->setParent(newNode);

        // Recalculate all counter indices in the newly nested subtree so each quantifier gets
        // an index based on its final nesting level.
        {
            Quantifier::CounterIndex nestingLevel = counterIndex;
            newNode->traverse(
                [&nestingLevel](PatternNode &node, int) -> void {
                    if (node.isQuantifier()) {
                        auto &q = std::get<Quantifier>(node.data());
                        q.counterIndex = nestingLevel;
                        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
                            q.counterIndex < limits::maximumCounterCount, "Counter index exceeds maximum"_el);
                        nestingLevel += 1;
                    }
                },
                [&nestingLevel](PatternNode &node, int) -> void {
                    if (node.isQuantifier()) {
                        nestingLevel -= 1;
                    }
                });
        }
        return newNode;
    }
    return repeatedNode;
}

}
