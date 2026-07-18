// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineFindFirstTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testFindChars() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          CHAR 'b'",
            "          CHAR 'c'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("abc123456"_el, CaptureRange{0, 3}));
        WITH_CONTEXT(requireFindFirst("1abc23456"_el, CaptureRange{1, 4}));
        WITH_CONTEXT(requireFindFirst("12abc3456"_el, CaptureRange{2, 5}));
        WITH_CONTEXT(requireFindFirst("123abc456"_el, CaptureRange{3, 6}));
        WITH_CONTEXT(requireFindFirst("a23abc456"_el, CaptureRange{3, 6}));
        WITH_CONTEXT(requireFindFirst("ab3abc456"_el, CaptureRange{3, 6}));
        WITH_CONTEXT(requireFindFirst("1ababc456"_el, CaptureRange{3, 6}));
        WITH_CONTEXT(requireFindFirst("aaaabc456"_el, CaptureRange{3, 6}));
        WITH_CONTEXT(requireNoFindFirst("123456789"_el));
        WITH_CONTEXT(requireNoFindFirst("a23456789"_el));
        WITH_CONTEXT(requireNoFindFirst("abaabbacb"_el));
    }

    void testFindFirst_PrefersLeftmostStart_WhenMultipleOccurrencesExist() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          CHAR 'b'",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("ababa"_el, CaptureRange{0, 3}));
        WITH_CONTEXT(requireFindFirst("xxababa"_el, CaptureRange{2, 5}));
        WITH_CONTEXT(requireNoFindFirst("abbb"_el));
    }

    void testFindFirst_FindsOverlappingCandidates() {
        // Equivalent to pattern `aa`.
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("aaa"_el, CaptureRange{0, 2}));
        WITH_CONTEXT(requireFindFirst("baaa"_el, CaptureRange{1, 3}));
        WITH_CONTEXT(requireNoFindFirst("aba"_el));
    }

    void testFindFirst_EmptyProgram_MatchesAtStart() {
        // A program that immediately matches must return an empty match at position 0.
        WITH_CONTEXT(assembleProgram({
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst(""_el, CaptureRange{0, 0}));
        WITH_CONTEXT(requireFindFirst("abc"_el, CaptureRange{0, 0}));
        WITH_CONTEXT(requireFindFirst("😄x"_el, CaptureRange{0, 0}));
    }

    void testFindFirst_StartAnchor_OnlyMatchesAtZero() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &Start",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("a"_el, CaptureRange{0, 1}));
        WITH_CONTEXT(requireNoFindFirst("ba"_el));
        WITH_CONTEXT(requireNoFindFirst("xxa"_el));
    }

    void testFindFirst_EndAnchor_FindsOnlyWhenMatchEndsAtEnd() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &End",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("a"_el, CaptureRange{0, 1}));
        WITH_CONTEXT(requireFindFirst("ba"_el, CaptureRange{1, 2}));
        WITH_CONTEXT(requireNoFindFirst("ab"_el));
        WITH_CONTEXT(requireFindFirst("baa"_el, CaptureRange{2, 3}));
    }

    void testFindFirst_Captures_AreRelativeToFoundMatch() {
        WITH_CONTEXT(assembleProgram({
            "          .groups 1",
            "          START CAPTURE 0",
            "          CHAR 'a'",
            "          STOP CAPTURE 0",
            "end:      MATCH",
        }));

        WITH_CONTEXT(requireFindFirst("xxa"_el, CaptureRange{2, 3}));
        WITH_CONTEXT(requireGroups({
            "00: 0002-0003 'a'",
            "01: 0002-0003 'a'",
        }));

        WITH_CONTEXT(requireFindFirst("a..a"_el, CaptureRange{0, 1}));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0001 'a'",
            "01: 0000-0001 'a'",
        }));
    }

    void testFindFirst_AlternationPriority_ChoosesHigherPriorityBranch_AtSameStart() {
        // Path 2 has priority.
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %path2, %path1",
            "path1:    CHAR 'a'",
            "          JUMP %end",
            "path2:    CHAR 'a'",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("aax"_el, CaptureRange{0, 2}));

        // Same start position, but a different priority order.
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %path1, %path2",
            "path1:    CHAR 'a'",
            "          JUMP %end",
            "path2:    CHAR 'a'",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("aax"_el, CaptureRange{0, 1}));
    }

    void testFindFirst_AlternationWithEmptyBranch() {
        // If an empty branch has priority, it must win and produce a zero-length match.
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %empty, %non_empty",
            "empty:    MATCH",
            "non_empty: CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("a"_el, CaptureRange{0, 0}));
        WITH_CONTEXT(requireFindFirst("xxx"_el, CaptureRange{0, 0}));

        // If the non-empty branch has priority, it must win (where possible).
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'b'",
            "          SPLIT %non_empty, %empty",
            "empty:    MATCH",
            "non_empty: CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoFindFirst("a"_el));
        WITH_CONTEXT(requireFindFirst("b"_el, CaptureRange{0, 1}));
        WITH_CONTEXT(requireFindFirst("ba"_el, CaptureRange{0, 2}));
        WITH_CONTEXT(requireFindFirst("xxba"_el, CaptureRange{2, 4}));
    }

    void testFindFirst_UnicodeCharacter_UsesByteOffsets() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR '😄'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireFindFirst("x😄y"_el, CaptureRange{1, 5}));
        WITH_CONTEXT(requireFindFirst("😄😄"_el, CaptureRange{0, 4}));
        WITH_CONTEXT(requireNoFindFirst("xyz"_el));
    }

    void testFindFirst_BacktrackingStress_AmbiguousAlternationWithRepetition() {
        // Roughly equivalent to `(a|aa)+b`.
        // This can create a lot of backtracking; it must still find the first match or conclude no match.
        WITH_CONTEXT(assembleProgram({
            "loop:     SPLIT %one, %two",
            "one:      CHAR 'a'",
            "          JUMP %cont",
            "two:      CHAR 'a'",
            "          CHAR 'a'",
            "cont:     SPLIT %loop, %after",
            "after:    CHAR 'b'",
            "end:      MATCH",
        }));

        WITH_CONTEXT(requireFindFirst("aaaaab"_el, CaptureRange{0, 6}));
        WITH_CONTEXT(requireFindFirst("xxaaaaabyy"_el, CaptureRange{2, 8}));
        WITH_CONTEXT(requireNoFindFirst("aaaaaaaaaaaaaac"_el));

        auto longTextMatch = StringEditor::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{99'999U}); // ~100kb
        longTextMatch.append("b"_el);
        WITH_CONTEXT(requireFindFirst(longTextMatch, CaptureRange{0, longTextMatch.length().toSizeT()}));
        auto longTextNotMatch =
            StringEditor::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{99'999U}); // ~100kb
        longTextNotMatch.append("c"_el);
        WITH_CONTEXT(requireNoFindFirst(longTextNotMatch));
    }
};
