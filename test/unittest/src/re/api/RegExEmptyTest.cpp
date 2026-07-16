// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api)
class RegExEmptyTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compilePattern() {
        Settings settings;
        settings.enableFeature(Feature::EmptyGroups);
        WITH_CONTEXT(requireCompile(""_el, {}, settings));
    }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compilePattern());

        // Everything should match, with a zero-width match result.
        const auto testCases = NoneCapGroupTestCases{
            {""_el},
            {"abc"_el, 0, ""_el},
            {"😀"_el, 0, ""_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(testCases));
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compilePattern());

        // Only an empty string for `fullMatch`.
        WITH_CONTEXT(requireFullMatchWithNoCaptures(""_el));
        WITH_CONTEXT(requireNoFullMatch("abc"_el));
        WITH_CONTEXT(requireNoFullMatch("😀"_el));
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePattern());

        // Everything should match at the first position.
        const auto testCases = NoneCapGroupTestCases{
            {""_el},
            {"abc"_el, 0, ""_el},
            {"😀"_el, 0, ""_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePattern());

        const auto expectedLines1 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
        };
        WITH_CONTEXT(requireFindAll(""_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));
        WITH_CONTEXT(requireFindAll(""_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));

        const auto expectedLines2 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0001-0001 ''",
            "Match 03:",
            "00: 0002-0002 ''",
            "Match 04:",
            "00: 0003-0003 ''",
        };
        WITH_CONTEXT(requireFindAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));
        WITH_CONTEXT(requireFindAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));

        const auto expectedLines3 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0004-0004 ''",
        };
        WITH_CONTEXT(requireFindAll("😀"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines3));
        WITH_CONTEXT(requireFindAll("😀"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines3));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePattern());

        const auto expectedLines1 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
        };
        WITH_CONTEXT(requireCollectAll(""_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));
        WITH_CONTEXT(requireCollectAll(""_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines1));

        const auto expectedLines2 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0001-0001 ''",
            "Match 03:",
            "00: 0002-0002 ''",
            "Match 04:",
            "00: 0003-0003 ''",
        };
        WITH_CONTEXT(requireCollectAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));
        WITH_CONTEXT(requireCollectAll("abc"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines2));

        const auto expectedLines3 = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0004-0004 ''",
        };
        WITH_CONTEXT(requireCollectAll("😀"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines3));
        WITH_CONTEXT(requireCollectAll("😀"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines3));
    }

    void testReplace() {
        WITH_CONTEXT(compilePattern());

        const auto testCases = ReplaceTestCases{
            {""_el, "<x>"_el, "<x>"_el},
            {"abc"_el, "<x>"_el, "<x>a<x>b<x>c<x>"_el},
            {"😀"_el, "<x>"_el, "<x>😀<x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
