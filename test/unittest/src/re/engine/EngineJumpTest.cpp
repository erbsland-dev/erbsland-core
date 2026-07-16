// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineJumpTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testJumpToEnd() {
        WITH_CONTEXT(assembleProgram({
            "          JUMP %end",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("x"_el, 0));
    }

    void testLongSeriesOfJumps() {
        WITH_CONTEXT(assembleProgram({
            "          JUMP %pos4",
            "          FAILURE",
            "pos1:     JUMP %pos5",
            "          FAILURE",
            "pos2:     JUMP %pos3",
            "          FAILURE",
            "pos3:     CHAR 'x'",
            "          JUMP %pos1",
            "          FAILURE",
            "pos4:     JUMP %pos2",
            "          FAILURE",
            "pos5:     CHAR 'y'",
            "          MATCH",
        }));
        WITH_CONTEXT(requireMatch("xyzabc"_el, 2));
        WITH_CONTEXT(requireNoMatch("x0123"_el));
        WITH_CONTEXT(requireNoMatch("yxabc"_el));
    }

    void testInfiniteLoopDetection() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          CHAR 'b'",
            "          CHAR 'c'",
            "loop:     NONE",
            "          JUMP %loop",
        }));
        WITH_CONTEXT(requireErrorException("abc"_el, ErrorCategory::Timeout));
    }
};
