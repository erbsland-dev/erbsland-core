// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineAnchorStartEndTextTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testStart_AtBeginning_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &Start",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireNoMatch("b"_el));
    }

    void testStart_NotAtBeginning_DoesNotMatch() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'x'",
            "          ANCHOR &Start",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch("x"_el));
        WITH_CONTEXT(requireNoMatch("xy"_el));
    }

    void testStart_EmptyInput_DoesNotMatch() {
        // `Start` matches the beginning of the input, even for empty input.
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &Start",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(""_el, 0));
    }

    void testEnd_EmptyInput_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &End",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(""_el, 0));
    }

    void testEnd_AfterConsumingAllInput_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &End",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireNoMatch("ab"_el));
    }

    void testEnd_WithUnicodeCharacter_MatchesAtTrueEnd() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR '😄'",
            "          ANCHOR &End",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("😄"_el, 4));
        WITH_CONTEXT(requireNoMatch("😄x"_el));
    }
};
