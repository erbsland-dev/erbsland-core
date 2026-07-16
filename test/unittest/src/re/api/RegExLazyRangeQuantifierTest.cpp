// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Lazy)
class RegExLazyRangeQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharLazyMinMaxZeroToThree() { WITH_CONTEXT(requireCompile("a{0,3}?"_el)); }

    void compileSingleCharLazyMinMaxOneToThree() { WITH_CONTEXT(requireCompile("a{1,3}?"_el)); }

    void compilePrefixSingleCharLazyMinMaxZeroToThree() { WITH_CONTEXT(requireCompile("xa{0,3}?"_el)); }

    void compilePrefixSingleCharLazyMinMaxOneToThreePostfix() { WITH_CONTEXT(requireCompile("xa{1,3}?a"_el)); }

    void compilePrefixGroupLazyMinMaxPostfix() { WITH_CONTEXT(requireCompile("x(?:ab){2,4}?y"_el)); }

    void compileNestedGroupsLazyMinMax() {
        WITH_CONTEXT(requireCompile("x(?:a{1,2}?b{0,2}?(?:xyz(?:123){1,3}?){1,2}?){2,3}?y"_el));
    }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharLazyMinMaxZeroToThree());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {""_el, 0, ""_el},
                {"a"_el, 0, ""_el},
                {"aa"_el, 0, ""_el},
                {"aaa"_el, 0, ""_el},
                {"aaaa"_el, 0, ""_el},
                {"ba"_el, 0, ""_el},
                {"😀"_el, 0, ""_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }

        WITH_CONTEXT(compileSingleCharLazyMinMaxOneToThree());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "a"_el},
                {"aaa"_el, 0, "a"_el},
                {"aaaa"_el, 0, "a"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "ba"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxZeroToThree());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"x"_el, 0, "x"_el},
                {"xa"_el, 0, "x"_el},
                {"xaa"_el, 0, "x"_el},
                {"xaaa"_el, 0, "x"_el},
                {"xaaaa"_el, 0, "x"_el},
                {"xba"_el, 0, "x"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "y"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxOneToThreePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaa"_el},
                {"xaaaa"_el, 0, "xaa"_el},
                {"xaaaaa"_el, 0, "xaa"_el},
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

        WITH_CONTEXT(compilePrefixGroupLazyMinMaxPostfix());
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

        WITH_CONTEXT(compileNestedGroupsLazyMinMax());
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
        WITH_CONTEXT(compileSingleCharLazyMinMaxZeroToThree());
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

        WITH_CONTEXT(compileSingleCharLazyMinMaxOneToThree());
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

        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxOneToThreePostfix());
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
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxOneToThreePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaaaa"_el, 2, "xaa"_el},
            {"zzxaaaa"_el, 2, "xaa"_el},
            {"zzxaaa"_el, 2, "xaa"_el},
            {"zzxaa"_el, 2, "xaa"_el},
            {"xaa"_el, 0, "xaa"_el},
            {"xaaaa"_el, 0, "xaa"_el},
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
        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxOneToThreePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'xaa'",
            "Match 02:",
            "00: 0006-0009 'xaa'",
            "Match 03:",
            "00: 0010-0013 'xaa'",
        };
        WITH_CONTEXT(requireFindAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxOneToThreePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'xaa'",
            "Match 02:",
            "00: 0006-0009 'xaa'",
            "Match 03:",
            "00: 0010-0013 'xaa'",
        };
        WITH_CONTEXT(requireCollectAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaaa xaa xaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharLazyMinMaxOneToThreePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {"xaaaa"_el, ""_el, "aa"_el},
            {"xaaaa"_el, "<x>"_el, "<x>aa"_el},
            {"xaaaa xa xaa"_el, "<x>"_el, "<x>aa xa <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
