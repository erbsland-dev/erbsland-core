// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineCharTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testCharSequence() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          CHAR '😄'",
            "          CHAR 'z'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a😄z"_el, 6));
        WITH_CONTEXT(requireNoMatch("a😄"_el));
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }

    void testCiCharSequence() {
        WITH_CONTEXT(assembleProgram({
            "          CI CHAR 'a'",
            "          CI CHAR '😄'",
            "          CI CHAR 'z'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("A😄Z"_el, 6));
        WITH_CONTEXT(requireMatch("a😄Z"_el, 6));
        WITH_CONTEXT(requireMatch("A😄z"_el, 6));
        WITH_CONTEXT(requireMatch("a😄z"_el, 6));
        WITH_CONTEXT(requireNoMatch("ä😄z"_el));
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }

    void testNotCharSequence() {
        WITH_CONTEXT(assembleProgram({
            "          NOT CHAR 'a'",
            "          NOT CHAR '😄'",
            "          NOT CHAR 'z'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("123"_el, 3));
        WITH_CONTEXT(requireMatch("😌😌😌"_el, 12));
        WITH_CONTEXT(requireNoMatch("a23"_el));
        WITH_CONTEXT(requireNoMatch("1😄3"_el));
        WITH_CONTEXT(requireNoMatch("12z"_el));
    }

    void testNotCiCharSequence() {
        WITH_CONTEXT(assembleProgram({
            "          NOT CI CHAR 'a'",
            "          NOT CI CHAR '😄'",
            "          NOT CI CHAR 'z'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("123"_el, 3));
        WITH_CONTEXT(requireMatch("😌😌😌"_el, 12));
        WITH_CONTEXT(requireNoMatch("a23"_el));
        WITH_CONTEXT(requireNoMatch("A23"_el));
        WITH_CONTEXT(requireNoMatch("1😄3"_el));
        WITH_CONTEXT(requireNoMatch("12z"_el));
        WITH_CONTEXT(requireNoMatch("12Z"_el));
    }
};
