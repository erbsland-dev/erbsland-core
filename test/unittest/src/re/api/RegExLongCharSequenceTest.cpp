// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api CharSequences)
class RegExLongCharSequenceTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileLongSequence() {
        // long ASCII sequence -> SEQUENCE operation
        WITH_CONTEXT(requireCompile("abcdefgh"_el));
    }

    void compileLongSequenceCaseInsensitive() { WITH_CONTEXT(requireCompile("(?i)abcdefgh"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileLongSequence());

        const auto matchCases = NoneCapGroupTestCases{
            {"abcdefgh"_el, 0, "abcdefgh"_el},
            {"abcdefghijk"_el, 0, "abcdefgh"_el},
            {"abcdefgh😀"_el, 0, "abcdefgh"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "ABCDEFGH"_el,
            "abcDefgh"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }

    void testMatchCaseInsensitive() {
        WITH_CONTEXT(compileLongSequenceCaseInsensitive());

        const auto matchCases = NoneCapGroupTestCases{
            {"abcdefgh"_el, 0, "abcdefgh"_el},
            {"abcdefghijk"_el, 0, "abcdefgh"_el},
            {"abcdefgh😀"_el, 0, "abcdefgh"_el},
            {"ABCDEFGH"_el, 0, "ABCDEFGH"_el},
            {"ABCDEFGHIJK"_el, 0, "ABCDEFGH"_el},
            {"ABCDEFGH😀"_el, 0, "ABCDEFGH"_el},
            {"aBcDeFgHi"_el, 0, "aBcDeFgH"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "A"_el,
            "aB"_el,
            "xbcdefgh"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileLongSequence());

        const auto matchCases = std::vector<String>{
            "abcdefgh"_el,
        };
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "ABCDEFGH"_el,
            "abcDefgh"_el,
            "xyz"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFullMatch(noMatchCases));
    }

    void testFullMatchCaseInsensitive() {
        WITH_CONTEXT(compileLongSequenceCaseInsensitive());

        const auto matchCases = std::vector<String>{
            "abcdefgh"_el,
            "ABCDEFGH"_el,
            "abcDefgh"_el,
            "aBcDeFgH"_el,
            "AbCdEfGh"_el,
        };
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "xyz"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFullMatch(noMatchCases));
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileLongSequence());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"abcdefgh"_el, 0, "abcdefgh"_el},
            {"abcdefghijk"_el, 0, "abcdefgh"_el},
            {"aabcdefgh"_el, 1, "abcdefgh"_el},
            {"ababcdefgh"_el, 2, "abcdefgh"_el},
            {"abababcdefgh"_el, 4, "abcdefgh"_el},
            {"😀abcdefgh"_el, 4, "abcdefgh"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "abababbc"_el,
            "ABCDEFGH"_el,
            "abABCDEFGH"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    void testFindFirstCaseInsensitive() {
        WITH_CONTEXT(compileLongSequenceCaseInsensitive());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"abcdefgh"_el, 0, "abcdefgh"_el},
            {"ABCDEFGH"_el, 0, "ABCDEFGH"_el},
            {"aBcDeFgH"_el, 0, "aBcDeFgH"_el},
            {"abcdefghijk"_el, 0, "abcdefgh"_el},
            {"Abcdefghijk"_el, 0, "Abcdefgh"_el},
            {"aabcdefgh"_el, 1, "abcdefgh"_el},
            {"AABCDEFGH"_el, 1, "ABCDEFGH"_el},
            {"ababcdefgh"_el, 2, "abcdefgh"_el},
            {"abababcdefgh"_el, 4, "abcdefgh"_el},
            {"😀abcdefgh"_el, 4, "abcdefgh"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "abababbc"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compileLongSequence());

        const auto expectedLines1 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0008 'abcdefgh'",
        };
        WITH_CONTEXT(requireFindAll("abcdefgh"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));
        WITH_CONTEXT(requireFindAll("abcdefgh"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));

        const auto expectedLines2 = std::vector<std::string>{
            "Match 01:",
            "00: 0004-0012 'abcdefgh'",
            "Match 02:",
            "00: 0014-0022 'abcdefgh'",
            "Match 03:",
            "00: 0030-0038 'abcdefgh'",
            "Match 04:",
            "00: 0042-0050 'abcdefgh'",
        };
        WITH_CONTEXT(requireFindAll("😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));
        WITH_CONTEXT(requireFindAll("😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "abababbc"_el,
            "abcDefgHABCdEfGh"_el,
            "😀abbacbaabac"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compileLongSequence());

        const auto expectedLines1 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0008 'abcdefgh'",
        };
        WITH_CONTEXT(requireCollectAll("abcdefgh"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));
        WITH_CONTEXT(requireCollectAll("abcdefgh"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));

        const auto expectedLines2 = std::vector<std::string>{
            "Match 01:",
            "00: 0004-0012 'abcdefgh'",
            "Match 02:",
            "00: 0014-0022 'abcdefgh'",
            "Match 03:",
            "00: 0030-0038 'abcdefgh'",
            "Match 04:",
            "00: 0042-0050 'abcdefgh'",
        };
        WITH_CONTEXT(requireCollectAll("😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));
        WITH_CONTEXT(requireCollectAll("😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "abcdefg"_el,
            "abababbc"_el,
            "abcDefgHABCdEfGh"_el,
            "😀abbacbaabac"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testReplace() {
        WITH_CONTEXT(compileLongSequence());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"abcdefgh"_el, ""_el, ""_el},
            {"abcdefgh"_el, "<x>"_el, "<x>"_el},
            {"😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el, ""_el, "😀XYabababab😀 xyz"_el},
            {"😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el, "<x>"_el, "😀<x>XY<x>abababab<x>😀<x> xyz"_el},
            {"😀abcdefghXYabcdefghabababababcdefgh😀abcdefgh xyz"_el,
                "<😆→>"_el,
                "😀<😆→>XY<😆→>abababab<😆→>😀<😆→> xyz"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
