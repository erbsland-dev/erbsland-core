// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EngineBase.hpp"

TESTED_TARGETS(Engine)
TAGS(Matching) class EngineAnchorWordBoundaryUnicodeAsciiTest final : public UNITTEST_SUBCLASS(EngineBase) {
public:
    void testUnicodeWordBoundary_AtStartBeforeWord_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &UWordBoundary",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
        WITH_CONTEXT(requireNoMatch(" "_el));
    }

    void testUnicodeWordBoundary_WordToNonWord_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &UWordBoundary",
            "          CHAR ' '",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a "_el, 2));
        WITH_CONTEXT(requireNoMatch("ab"_el));
    }

    void testUnicodeWordBoundary_NonWordToWord_Matches_AndNonUnicodeDoesNot() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR ' '",
            "          ANCHOR &UWordBoundary",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(" a"_el, 2));

        WITH_CONTEXT(assembleProgram({
            "          CHAR ' '",
            "          ANCHOR &NUWordBoundary",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch(" a"_el));
    }

    void testUnicodeWordBoundary_NonWordToNonWord_DoesNotMatch_AndNonUnicodeDoes() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR ' '",
            "          ANCHOR &UWordBoundary",
            "          CHAR '!'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch(" !"_el));

        WITH_CONTEXT(assembleProgram({
            "          CHAR ' '",
            "          ANCHOR &NUWordBoundary",
            "          CHAR '!'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(" !"_el, 2));
    }

    void testUnicodeWordBoundary_WordToWord_DoesNotMatch_AndNonUnicodeDoes() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &UWordBoundary",
            "          CHAR 'b'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch("ab"_el));

        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &NUWordBoundary",
            "          CHAR 'b'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("ab"_el, 2));
    }

    void testUnicodeWordBoundary_WithNonAsciiWordCharacter_Matches() {
        // 'ä' is a Unicode letter; it must behave as a word character in Unicode mode.
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &UWordBoundary",
            "          CHAR 'ä'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("ä"_el, 2));
    }

    void testAsciiWordBoundary_WithNonAsciiWordCharacter_DoesNotMatch_AndNonAsciiDoes() {
        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &AWordBoundary",
            "          CHAR 'ä'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch("ä"_el));

        WITH_CONTEXT(assembleProgram({
            "          ANCHOR &NAWordBoundary",
            "          CHAR 'ä'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("ä"_el, 2));
    }

    void testAsciiWordBoundary_WordToEnd_Matches() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR 'a'",
            "          ANCHOR &AWordBoundary",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch("a"_el, 1));
    }

    void testAsciiWordBoundary_NonWordToWord_Matches_AndNonAsciiDoesNot() {
        WITH_CONTEXT(assembleProgram({
            "          CHAR ' '",
            "          ANCHOR &AWordBoundary",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireMatch(" a"_el, 2));

        WITH_CONTEXT(assembleProgram({
            "          CHAR ' '",
            "          ANCHOR &NAWordBoundary",
            "          CHAR 'a'",
            "end:      MATCH",
        }));
        WITH_CONTEXT(requireNoMatch(" a"_el));
    }
};
