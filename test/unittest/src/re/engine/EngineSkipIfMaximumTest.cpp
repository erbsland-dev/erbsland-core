// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineSkipIfMaximumTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testSkipIfMaximumEnforcesFixedRepetition() {
        WITH_CONTEXT(assembleProgram({
            "          COUNTER 0, 0",
            "loop:     CHAR 'a'",
            "          SKIP MAXIMUM 0, 1",
            "          JUMP %loop",
            "          SUCCESS",
        }));

        WITH_CONTEXT(requireFullMatch("aa"_el, 2));
        WITH_CONTEXT(requireNoFullMatch("a"_el));
        WITH_CONTEXT(requireNoFullMatch("aaa"_el));
    }
};
