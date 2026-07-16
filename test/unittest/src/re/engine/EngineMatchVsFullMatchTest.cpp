// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineMatchVsFullMatchTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testPrefixMatch_TrailingInput_MatchYesFullMatchNo() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "end:      MATCH",
        }));

        // Same program, different APIs.
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireFullMatch("a"_el, 1));

        WITH_CONTEXT(requireMatch("ab"_el, 1));
        WITH_CONTEXT(requireNoFullMatch("ab"_el));

        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoFullMatch(""_el));
    }

    void testEndAnchor_RequiresTrueEnd_ForBothCalls() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &End",
            "end:      MATCH",
        }));

        // With an explicit end-anchor, both APIs must reject trailing input.
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireFullMatch("a"_el, 1));

        WITH_CONTEXT(requireNoMatch("ab"_el));
        WITH_CONTEXT(requireNoFullMatch("ab"_el));
    }

    void testZeroLengthMatch_DoesNotImplyFullMatch() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &Start",
            "end:      MATCH",
        }));

        // The program matches the empty string at the current position.
        // `match` must accept this even when there is remaining input.
        WITH_CONTEXT(requireMatch(""_el, 0));
        WITH_CONTEXT(requireMatch("abc"_el, 0));

        // `fullMatch` must require that the whole input was consumed.
        WITH_CONTEXT(requireFullMatch(""_el, 0));
        WITH_CONTEXT(requireNoFullMatch("abc"_el));
    }

    void testZeroLengthFullMatch_RequiresBothStartAndEnd() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &Start",
            "          ANCHOR &End",
            "end:      MATCH",
        }));

        WITH_CONTEXT(requireMatch(""_el, 0));
        WITH_CONTEXT(requireFullMatch(""_el, 0));

        WITH_CONTEXT(requireNoMatch("a"_el));
        WITH_CONTEXT(requireNoFullMatch("a"_el));
    }

    void testUnicodeCharacter_TrailingAscii_MatchYesFullMatchNo() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR '😄'",
            "end:      MATCH",
        }));

        WITH_CONTEXT(requireMatch("😄"_el, 4));
        WITH_CONTEXT(requireFullMatch("😄"_el, 4));

        WITH_CONTEXT(requireMatch("😄x"_el, 4));
        WITH_CONTEXT(requireNoFullMatch("😄x"_el));
    }

    void testCaptures_AreConsistent_WhenBothSucceed() {
        WITH_CONTEXT(assembleProgram({
            "          .groups 1",
            "          START CAPTURE 0",
            "          CHAR 'a'",
            "          STOP CAPTURE 0",
            "end:      MATCH",
        }));

        // `match` succeeds on a prefix.
        WITH_CONTEXT(requireMatch("ab"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0001 'a'",
            "01: 0000-0001 'a'",
        }));

        // Same program with `fullMatch` must reject trailing input, and must not produce captures.
        WITH_CONTEXT(requireNoFullMatch("ab"_el));

        // When it does succeed, captures must match.
        WITH_CONTEXT(requireFullMatch("a"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0001 'a'",
            "01: 0000-0001 'a'",
        }));
    }

    void testDotStar_Greedy_MatchesAll_ForBothCalls() {
        // Equivalent to pattern `.*`.
        // Greedy repetition should consume as much as possible before reporting the match.
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %loop, %end",
            "loop:     ANY",
            "          SPLIT %loop, %end",
            "end:      MATCH",
        }));

        WITH_CONTEXT(requireMatch(""_el, 0));
        WITH_CONTEXT(requireFullMatch(""_el, 0));

        WITH_CONTEXT(requireMatch("abc"_el, 3));
        WITH_CONTEXT(requireFullMatch("abc"_el, 3));

        WITH_CONTEXT(requireMatch("😄"_el, 4));
        WITH_CONTEXT(requireFullMatch("😄"_el, 4));
    }

    void testGreedyMaximum_PrefersLongestMatch_EvenForSingleCharacterInput() {
        // Regression test for a class of bugs where the engine incorrectly keeps an earlier
        // empty match and fails to prefer a later longer match that starts at the same position.

        // Equivalent to pattern `.*` (greedy).
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %loop, %end",
            "loop:     ANY",
            "          SPLIT %loop, %end",
            "end:      MATCH",
        }));

        // Single ASCII character.
        WITH_CONTEXT(requireMatch("x"_el, 1));
        WITH_CONTEXT(requireFullMatch("x"_el, 1));

        // Single multi-byte UTF-8 character.
        WITH_CONTEXT(requireMatch("😄"_el, 4));
        WITH_CONTEXT(requireFullMatch("😄"_el, 4));

        // Equivalent to pattern `a*` (greedy).
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %loop, %end",
            "loop:     CHAR 'a'",
            "          SPLIT %loop, %end",
            "end:      MATCH",
        }));

        // Single character that can be consumed by the repeat.
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireFullMatch("a"_el, 1));

        // Must still allow empty input.
        WITH_CONTEXT(requireMatch(""_el, 0));
        WITH_CONTEXT(requireFullMatch(""_el, 0));
    }

    void testDotStarLazy_MatchReturnsEmptyPrefix_ButFullMatchConsumesAll() {
        // Equivalent to pattern `.*?`.
        // The lazy repetition prefers the empty match for `match()`, but `fullMatch()` must still
        // backtrack/expand until the whole input is consumed.
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %end, %loop",
            "loop:     ANY",
            "          SPLIT %end, %loop",
            "end:      MATCH",
        }));

        WITH_CONTEXT(requireMatch(""_el, 0));
        WITH_CONTEXT(requireFullMatch(""_el, 0));

        WITH_CONTEXT(requireMatch("abc"_el, 0));
        WITH_CONTEXT(requireFullMatch("abc"_el, 3));

        WITH_CONTEXT(requireMatch("😄x"_el, 0));
        WITH_CONTEXT(requireFullMatch("😄x"_el, 5));
    }
};
