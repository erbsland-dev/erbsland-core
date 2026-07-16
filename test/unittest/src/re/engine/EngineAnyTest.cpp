// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineAnyTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testOneAny() {
        WITH_CONTEXT(assembleProgram({
            "          ANY",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireMatch("😄"_el, 4));
        WITH_CONTEXT(requireMatch("abc"_el, 1));
        WITH_CONTEXT(requireMatch("😄az"_el, 4));
        WITH_CONTEXT(requireMatch("a😄z"_el, 1));
        WITH_CONTEXT(requireNoMatch(""_el));
    }

    void testManyAny() {
        WITH_CONTEXT(assembleProgram({
            "          ANY",
            "          ANY",
            "          ANY",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("abc"_el, 3));
        WITH_CONTEXT(requireMatch("a😄z"_el, 6));
        WITH_CONTEXT(requireNoMatch(""_el));
        WITH_CONTEXT(requireNoMatch("a"_el));
        WITH_CONTEXT(requireNoMatch("ab"_el));
        WITH_CONTEXT(requireNoMatch("a😄"_el));
    }
};
