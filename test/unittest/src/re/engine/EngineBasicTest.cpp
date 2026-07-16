// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineBasicTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testConstruction() {
        // manually construct and run the engine to see if it works
        assembleProgram({
            // always matching, zero-width program
            "NONE", // Add a None instruction to make sure the program must advance the PC.
            "SUCCESS",
        });
        engine = Engine::create(engineData);
        input = std::make_shared<MockStringInput>("abc"_el);
        state = engine->createState(input);
        hasMatch = engine->match(*state);
        REQUIRE_EQUAL(hasMatch, EngineHasMatch::Yes);
        captureGroups = state->createCaptureGroups({});
        REQUIRE_EQUAL(captureGroups.size(), 1);
        REQUIRE_EQUAL(captureGroups[0].begin(), 0);
        REQUIRE_EQUAL(captureGroups[0].end(), 0);
    }

    void testOneCharacterMatch() {
        assembleProgram({
            "CHAR 'a'",
            "MATCH",
        });
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }

    void testMultipleCharacterMatch() {
        assembleProgram({
            "CHAR 'a'",
            "CHAR 'b'",
            "CHAR 'c'",
            "MATCH",
        });
        WITH_CONTEXT(requireMatch("abc"_el, 3));
        WITH_CONTEXT(requireMatch("abcdef"_el, 3));
        WITH_CONTEXT(requireMatch("abcabc"_el, 3));
        WITH_CONTEXT(requireNoMatch("axx"_el));
        WITH_CONTEXT(requireNoMatch("abx"_el));
        WITH_CONTEXT(requireNoMatch("xyz"_el));
    }
};
