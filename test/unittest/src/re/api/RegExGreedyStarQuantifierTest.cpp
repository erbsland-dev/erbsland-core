// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Greedy)
class RegExGreedyStarQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharZeroOrMore() { WITH_CONTEXT(requireCompile("a*"_el)); }

    void compilePrefixSingleCharZeroOrMore() { WITH_CONTEXT(requireCompile("xa*"_el)); }

    void compilePrefixSingleCharZeroOrMorePostfix() { WITH_CONTEXT(requireCompile("xa*a"_el)); }

    void compilePrefixGroupZeroOrMorePostfix() { WITH_CONTEXT(requireCompile("x(?:ab)*y"_el)); }

    void compileNestedGroupsZeroOrMore() { WITH_CONTEXT(requireCompile("x(?:a*b*c(?:xyz(?:123)*)*)*y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharZeroOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {""_el, 0, ""_el},
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "aa"_el},
                {"ba"_el, 0, ""_el},
                {"😀"_el, 0, ""_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharZeroOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"x"_el, 0, "x"_el},
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xaa"_el},
                {"xba"_el, 0, "x"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "y"_el,
                "a"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharZeroOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaaa"_el},
                {"xaaaa"_el, 0, "xaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "😀xaa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupZeroOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xy"_el, 0, "xy"_el},
                {"xaby"_el, 0, "xaby"_el},
                {"xabababy"_el, 0, "xabababy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "xay"_el,
                "xab"_el,
                "a"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsZeroOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xy"_el, 0, "xy"_el},
                {"xcy"_el, 0, "xcy"_el},
                {"xccy"_el, 0, "xccy"_el},
                {"xcccy"_el, 0, "xcccy"_el},
                {"xaaabbbcxyz123123xyzy"_el, 0, "xaaabbbcxyz123123xyzy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "a"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharZeroOrMore());
        {
            const auto matchCases = std::vector<String>{
                ""_el,
                "a"_el,
                "aa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "b"_el,
                "ab"_el,
                "😀"_el,
                "aab"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharZeroOrMorePostfix());
        {
            const auto matchCases = std::vector<String>{
                "xa"_el,
                "xaa"_el,
                "xaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "x"_el,
                "xaaaab"_el,
                ""_el,
                "a"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupZeroOrMorePostfix());
        {
            const auto matchCases = std::vector<String>{
                "xy"_el,
                "xaby"_el,
                "xabababy"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "xa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharZeroOrMorePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaa"_el, 2, "xaaa"_el},
            {"zzxa"_el, 2, "xa"_el},
            {"xa"_el, 0, "xa"_el},
            {"xaaaa"_el, 0, "xaaaa"_el},
            {"😀xaaa"_el, 4, "xaaa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "x"_el,
            "y"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharZeroOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xaaa'",
            "Match 02:",
            "00: 0005-0007 'xa'",
            "Match 03:",
            "00: 0008-0011 'xaa'",
        };
        WITH_CONTEXT(requireFindAll("xaaa xa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaa xa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "x"_el,
            "y"_el,
            "a"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharZeroOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xaaa'",
            "Match 02:",
            "00: 0005-0007 'xa'",
            "Match 03:",
            "00: 0008-0011 'xaa'",
        };
        WITH_CONTEXT(requireCollectAll("xaaa xa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaa xa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharZeroOrMorePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"xaaa"_el, ""_el, ""_el},
            {"xaaa"_el, "<x>"_el, "<x>"_el},
            {"xaaa xa xaa"_el, "<x>"_el, "<x> <x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
