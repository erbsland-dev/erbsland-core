// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineSplitTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testTwoEqualPaths() {
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %path1, %path2",
            "path1:    CHAR 'a'",
            "          JUMP %end",
            "path2:    CHAR 'b'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("ayz"_el, 1));
        WITH_CONTEXT(requireMatch("byz"_el, 1));
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }

    void testMatchTwoUnequalPaths() {
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %path2, %path1",
            "path1:    CHAR 'a'",
            "          JUMP %end",
            "path2:    CHAR 'a'",
            "          CHAR 'a'",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("aaa"_el, 3)); // Path 2 has priority.
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }

    void testSuccessTwoUnequalPaths() {
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %path2, %path1",
            "path1:    CHAR 'a'",
            "          JUMP %end",
            "path2:    CHAR 'a'",
            "          CHAR 'a'",
            "          CHAR 'a'",
            "end:      SUCCESS",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1)); // path 2 has priority, but success stops early.
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }

    void testSplitGreedyRepetition() {
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %loop1, %end",
            "loop1:    CHAR 'a'",
            "          SPLIT %loop1, %end",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaxxx"_el, 60));
        WITH_CONTEXT(requireMatch("xyz"_el, 0));
    }

    void testSplitLazyRepetition() {
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %end, %loop1",
            "loop1:    CHAR 'a'",
            "          SPLIT %end, %loop1",
            "end:      SUCCESS",
        }));
        WITH_CONTEXT(requireMatch("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaxxx"_el, 0));
        WITH_CONTEXT(requireMatch("xyz"_el, 0));
    }

    void testSplitMultiple() {
        WITH_CONTEXT(assembleProgram({
            "          SPLIT %path1, %path2",
            "path1:    CHAR 'a'",
            "          MATCH",
            "path2:    SPLIT %path3, %path4",
            "path3:    CHAR 'b'",
            "          MATCH",
            "path4:    CHAR 'c'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireMatch("b"_el, 1));
        WITH_CONTEXT(requireMatch("c"_el, 1));
        WITH_CONTEXT(requireNoMatch("x"_el));
    }
};
