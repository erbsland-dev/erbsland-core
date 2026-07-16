// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Greedy)
class RegExGreedyRangeQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharMinMaxZeroToThree() { WITH_CONTEXT(requireCompile("a{0,3}"_el)); }

    void compileSingleCharMinMaxOneToThree() { WITH_CONTEXT(requireCompile("a{1,3}"_el)); }

    void compilePrefixSingleCharMinMaxZeroToThree() { WITH_CONTEXT(requireCompile("xa{0,3}"_el)); }

    void compilePrefixSingleCharMinMaxOneToThreePostfix() { WITH_CONTEXT(requireCompile("xa{1,3}a"_el)); }

    void compilePrefixGroupMinMaxPostfix() { WITH_CONTEXT(requireCompile("x(?:ab){2,4}y"_el)); }

    void compileNestedGroupsMinMax() {
        WITH_CONTEXT(requireCompile("x(?:a{1,2}b{0,2}(?:xyz(?:123){1,3}){1,2}){2,3}y"_el));
    }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharMinMaxZeroToThree());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {""_el, 0, ""_el},
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "aa"_el},
                {"aaa"_el, 0, "aaa"_el},
                {"aaaa"_el, 0, "aaa"_el},
                {"ba"_el, 0, ""_el},
                {"😀"_el, 0, ""_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }

        WITH_CONTEXT(compileSingleCharMinMaxOneToThree());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "aa"_el},
                {"aaa"_el, 0, "aaa"_el},
                {"aaaa"_el, 0, "aaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "ba"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharMinMaxZeroToThree());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"x"_el, 0, "x"_el},
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaaa"_el},
                {"xaaaa"_el, 0, "xaaa"_el},
                {"xba"_el, 0, "x"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "y"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharMinMaxOneToThreePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaaa"_el},
                {"xaaaa"_el, 0, "xaaaa"_el},
                {"xaaaaa"_el, 0, "xaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "xa"_el,
                "y"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupMinMaxPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xababy"_el, 0, "xababy"_el},
                {"xabababy"_el, 0, "xabababy"_el},
                {"xababababy"_el, 0, "xababababy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "xy"_el,
                "xaby"_el,
                "xababab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsMinMax());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaxyz123axyz123y"_el, 0, "xaxyz123axyz123y"_el},
                {"xaxyz123abxyz123xyz123123aabbxyz123123y"_el, 0, "xaxyz123abxyz123xyz123123aabbxyz123123y"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharMinMaxZeroToThree());
        {
            const auto matchCases = std::vector<StringView>{
                ""_el,
                "a"_el,
                "aa"_el,
                "aaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "aaaa"_el,
                "b"_el,
                "ab"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileSingleCharMinMaxOneToThree());
        {
            const auto matchCases = std::vector<StringView>{
                "a"_el,
                "aa"_el,
                "aaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "aaaa"_el,
                "b"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharMinMaxOneToThreePostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xaa"_el,
                "xaaa"_el,
                "xaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "x"_el,
                "xa"_el,
                "xaaaaa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupMinMaxPostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xababy"_el,
                "xabababy"_el,
                "xababababy"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharMinMaxOneToThreePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaaaa"_el, 2, "xaaaa"_el},
            {"zzxaaaa"_el, 2, "xaaaa"_el},
            {"zzxaaa"_el, 2, "xaaa"_el},
            {"zzxaa"_el, 2, "xaa"_el},
            {"xaaaa"_el, 0, "xaaaa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "xa"_el,
            "y"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharMinMaxOneToThreePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xaaaa'",
            "Match 02:",
            "00: 0006-0009 'xaa'",
            "Match 03:",
            "00: 0010-0014 'xaaa'",
        };
        WITH_CONTEXT(requireFindAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharMinMaxOneToThreePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xaaaa'",
            "Match 02:",
            "00: 0006-0009 'xaa'",
            "Match 03:",
            "00: 0010-0014 'xaaa'",
        };
        WITH_CONTEXT(requireCollectAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharMinMaxOneToThreePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"xaaaa"_el, ""_el, ""_el},
            {"xaaaa"_el, "<x>"_el, "<x>"_el},
            {"xaaaa xa xaa xaaa"_el, "<x>"_el, "<x> xa <x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
