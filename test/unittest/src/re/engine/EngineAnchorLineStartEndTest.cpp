// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineAnchorLineStartEndTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testLineStart_AtBeginning() {
        // `LineStart` only checks if it after a newline or at the start of the text.
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &LineStart",
            "          ANY",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireMatch("b"_el, 1));
    }

    void testLineStart_AfterNewline_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          ANY",
            "          ANCHOR &LineStart",
            "          ANY",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("\na"_el, 2));
        WITH_CONTEXT(requireNoMatch("ab"_el));
        WITH_CONTEXT(requireNoMatch("a\n"_el));
    }

    void testLineStart_AfterNewline_AtEndOfInput_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          ANY",
            "          ANCHOR &LineStart",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("\n"_el, 1));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testLineStart_ConsecutiveNewlines_MatchesAtEachLineStart() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR '\\n'",
            "          ANCHOR &LineStart",
            "          CHAR '\\n'",
            "          ANCHOR &LineStart",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("\n\na"_el, 3));
    }

    void testLineEnd_BeforeNewline_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &LineEnd",
            "          CHAR '\\n'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a\n"_el, 2));
        WITH_CONTEXT(requireNoMatch("ab\n"_el));
    }

    void testLineEnd_AtEndOfInput() {
        // `LineEnd` only checks for a following newline character or the end of the input.
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &LineEnd",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireMatch("a\n"_el, 1));
    }

    void testLineEnd_EmptyInput() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &LineEnd",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(""_el, 0));
    }

    void testLineEnd_ConsecutiveNewlines_MatchesBeforeNewline() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR '\\n'",
            "          ANCHOR &LineEnd",
            "          CHAR '\\n'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("\n\n"_el, 2));
    }

    void testLineEnd_WithUnicodeAroundNewline_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR '😄'",
            "          ANCHOR &LineEnd",
            "          CHAR '\\n'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("😄\n"_el, 5));
    }
};
