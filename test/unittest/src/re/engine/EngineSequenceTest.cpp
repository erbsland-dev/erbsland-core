// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching)
class EngineSequenceTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testOneSequence() {
        WITH_CONTEXT(assembleProgram({
            ".section &sequence",
            "str1:     .data \"Ｅrbsland RegEx\"",
            ".section &program",
            "          SEQUENCE %str1, 14", // 14 characters (not bytes!)
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("Ｅrbsland RegEx"_el, 16));
        WITH_CONTEXT(requireMatch("Ｅrbsland RegEx Engine"_el, 16));
        WITH_CONTEXT(requireNoMatch("Erbsland RegEx"_el));
        WITH_CONTEXT(requireNoMatch("Ｅrbsland RegE"_el));
        WITH_CONTEXT(requireNoMatch("Ｅrbsland RegE"_el));
    }

    void testOneISequence() {
        WITH_CONTEXT(assembleProgram({
            ".section &sequence",
            "str1:     .data \"ｅrbsland regex\"",
            ".section &program",
            "          CI SEQUENCE %str1, 14", // 14 characters (not bytes!)
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("Ｅrbsland RegEx"_el, 16));
        WITH_CONTEXT(requireMatch("Ｅrbsland RegEx Engine"_el, 16));
        WITH_CONTEXT(requireMatch("ｅrbsland regex"_el, 16));
        WITH_CONTEXT(requireMatch("ｅrbsland regex engine"_el, 16));
        WITH_CONTEXT(requireMatch("ＥRBSLAND REGEX"_el, 16));
        WITH_CONTEXT(requireMatch("ＥRBSLAND REGEX ENGINE"_el, 16));
        WITH_CONTEXT(requireNoMatch("Erbsland RegEx"_el));
        WITH_CONTEXT(requireNoMatch("erbsland regex"_el));
        WITH_CONTEXT(requireNoMatch("ｅrbsland rege"_el));
        WITH_CONTEXT(requireNoMatch("ｅrbsland rege"_el));
    }

    void testMultipleSequence() {
        WITH_CONTEXT(assembleProgram({
            ".section &sequence",
            "str1:     .data \"Ｅrbsland\"",
            "str2:     .data \"ＲegEx\"",
            ".section &program",
            "          SEQUENCE %str1, 8",
            "          SEQUENCE %str2, 5",
            "          SEQUENCE %str1, 6",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("ＥrbslandＲegExＥrbsla"_el, 25));
        WITH_CONTEXT(requireMatch("ＥrbslandＲegExＥrbsland"_el, 25));
    }
};
