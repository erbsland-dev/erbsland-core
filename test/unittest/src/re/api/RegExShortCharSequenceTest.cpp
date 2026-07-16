// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api CharSequences)
class RegExShortCharSequenceTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileShortSequence() {
        // short ASCII sequence -> CHAR operations
        WITH_CONTEXT(requireCompile("abc"_el));
    }

    void compileShortSequenceCaseInsensitive() { WITH_CONTEXT(requireCompile("(?i)abc"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileShortSequence());

        const auto matchCases = NoneCapGroupTestCases{
            {"abc"_el, 0, "abc"_el},
            {"abcdef"_el, 0, "abc"_el},
            {"abc😀"_el, 0, "abc"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "ABC"_el,
            "abC"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }

    void testMatchCaseInsensitive() {
        WITH_CONTEXT(compileShortSequenceCaseInsensitive());

        const auto matchCases = NoneCapGroupTestCases{
            {"abc"_el, 0, "abc"_el},
            {"abcdef"_el, 0, "abc"_el},
            {"abc😀"_el, 0, "abc"_el},
            {"ABC"_el, 0, "ABC"_el},
            {"ABCDEF"_el, 0, "ABC"_el},
            {"ABC😀"_el, 0, "ABC"_el},
            {"aBcD"_el, 0, "aBc"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "A"_el,
            "aB"_el,
            "xbc"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileShortSequence());

        const auto matchCases = std::vector<StringView>{
            "abc"_el,
        };
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "ABC"_el,
            "abC"_el,
            "xyz"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFullMatch(noMatchCases));
    }

    void testFullMatchCaseInsensitive() {
        WITH_CONTEXT(compileShortSequenceCaseInsensitive());

        const auto matchCases = std::vector<StringView>{
            "abc"_el,
            "ABC"_el,
            "abC"_el,
            "aBc"_el,
            "AbC"_el,
        };
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "xyz"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFullMatch(noMatchCases));
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileShortSequence());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"abc"_el, 0, "abc"_el},
            {"abcdef"_el, 0, "abc"_el},
            {"aabc"_el, 1, "abc"_el},
            {"ababc"_el, 2, "abc"_el},
            {"abababcbcabc"_el, 4, "abc"_el},
            {"😀abc"_el, 4, "abc"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "abababbc"_el,
            "ABC"_el,
            "abABC"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    void testFindFirstCaseInsensitive() {
        WITH_CONTEXT(compileShortSequenceCaseInsensitive());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"abc"_el, 0, "abc"_el},
            {"ABC"_el, 0, "ABC"_el},
            {"aBc"_el, 0, "aBc"_el},
            {"abcdef"_el, 0, "abc"_el},
            {"Abcdef"_el, 0, "Abc"_el},
            {"aabc"_el, 1, "abc"_el},
            {"AABC"_el, 1, "ABC"_el},
            {"ababc"_el, 2, "abc"_el},
            {"abababcbcabc"_el, 4, "abc"_el},
            {"😀abc"_el, 4, "abc"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "abababbc"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compileShortSequence());

        const auto expectedLines1 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'abc'",
        };
        WITH_CONTEXT(requireFindAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));
        WITH_CONTEXT(requireFindAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));

        const auto expectedLines2 = std::vector<std::string>{
            "Match 01:",
            "00: 0004-0007 'abc'",
            "Match 02:",
            "00: 0010-0013 'abc'",
            "Match 03:",
            "00: 0019-0022 'abc'",
            "Match 04:",
            "00: 0026-0029 'abc'",
        };
        WITH_CONTEXT(requireFindAll("😀abcdefabcababababc😀abc xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));
        WITH_CONTEXT(requireFindAll("😀abcdefabcababababc😀abc xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "abababbc"_el,
            "abCaBCABC"_el,
            "😀abbacbaabac"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compileShortSequence());

        const auto expectedLines1 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'abc'",
        };
        WITH_CONTEXT(requireCollectAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));
        WITH_CONTEXT(requireCollectAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));

        const auto expectedLines2 = std::vector<std::string>{
            "Match 01:",
            "00: 0004-0007 'abc'",
            "Match 02:",
            "00: 0010-0013 'abc'",
            "Match 03:",
            "00: 0019-0022 'abc'",
            "Match 04:",
            "00: 0026-0029 'abc'",
        };
        WITH_CONTEXT(requireCollectAll("😀abcdefabcababababc😀abc xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));
        WITH_CONTEXT(requireCollectAll("😀abcdefabcababababc😀abc xyz"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "a"_el,
            "ab"_el,
            "abababbc"_el,
            "abCaBCABC"_el,
            "😀abbacbaabac"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testReplace() {
        WITH_CONTEXT(compileShortSequence());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"abc"_el, ""_el, ""_el},
            {"abc"_el, "<x>"_el, "<x>"_el},
            {"😀abcdefabcababababc😀abc xyz"_el, ""_el, "😀defababab😀 xyz"_el},
            {"😀abcdefabcababababc😀abc xyz"_el, "<x>"_el, "😀<x>def<x>ababab<x>😀<x> xyz"_el},
            {"😀abcdefabcababababc😀abc xyz"_el, "<😆→>"_el, "😀<😆→>def<😆→>ababab<😆→>😀<😆→> xyz"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
