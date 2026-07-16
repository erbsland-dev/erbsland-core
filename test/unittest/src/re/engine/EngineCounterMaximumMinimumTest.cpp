// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching) class EngineCounterMaximumMinimumTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testCounter_SetsStartValue_ForMaximum() {
        // If COUNTER does not set the counter correctly, MAXIMUM would end the thread (>= max).
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 2",
            "          MAXIMUM 0, 3",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireNoMatch("b"_el));
    }

    void testMaximum_WhenMaximumReachedBeforeIncrease_ThreadEnds() {
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 3",
            "          MAXIMUM 0, 3",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch("a"_el));
    }

    void testMaximum_IncreasesCounter_UntilMaximumReached() {
        // Match exactly 3 times 'a'. The MAXIMUM operation counts iterations.
        // - After the 3rd MAXIMUM, the counter becomes 3.
        // - A thread that tries a 4th MAXIMUM ends (maximum reached before increase).
        // - A parallel thread can verify the minimum and then match.
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 0",
            "loop:     MAXIMUM 0, 3",
            "          CHAR 'a'",
            "          SPLIT %loop, %endcheck",
            "endcheck: MINIMUM 0, 3",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("aaa"_el, 3));
        WITH_CONTEXT(requireMatch("aaaa"_el, 3));
        WITH_CONTEXT(requireNoMatch("aa"_el));
        WITH_CONTEXT(requireNoMatch("b"_el));
    }

    void testMinimum_WhenBelowMinimum_ThreadEnds() {
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 2",
            "          MINIMUM 0, 3",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testMinimum_WhenAtOrAboveMinimum_ResetsCounterToZero() {
        // If MINIMUM doesn't reset the counter, the following MAXIMUM would end the thread.
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 3",
            "          MINIMUM 0, 3",
            "          MAXIMUM 0, 1",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
    }

    void testCounters_ArePerThread_NotSharedAcrossSplitBranches() {
        // SPLIT clones work; counters must not be shared between branches.
        // Branch A increments counter 0, then ends without match.
        // Branch B checks MINIMUM(0,1) and would match only if it incorrectly sees the increment from branch A.
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 0",
            "          SPLIT %left, %right",
            "left:     MAXIMUM 0, 1",
            "          NOT MATCH",
            "right:    MINIMUM 0, 1",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testCounters_Have15Slots_CanUseHighestIndex() {
        // The engine provides up to 15 counters per thread (indices 0..14).
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 14, 0",
            "          MAXIMUM 14, 1",
            "          MINIMUM 14, 1",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(""_el, 0));
    }

    void testAddCounter_SaturatesToUInt16Max() {
        // Ensure `ADD COUNTER` is a *saturating* add (no wrap-around).
        // If the value wraps to 0, the MINIMUM check would fail and the thread would end.
        // If it saturates at 65535, MINIMUM succeeds and resets the counter to 0, allowing MAXIMUM to run.
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 65535",
            "          ADD COUNTER 0, 1",
            "          MINIMUM 0, 65535",
            "          MAXIMUM 0, 1",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
    }
};
