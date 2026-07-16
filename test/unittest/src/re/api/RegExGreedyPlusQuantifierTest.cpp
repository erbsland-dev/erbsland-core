// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Greedy)
class RegExGreedyPlusQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharOneOrMore() { WITH_CONTEXT(requireCompile("a+"_el)); }

    void compilePrefixSingleCharOneOrMore() { WITH_CONTEXT(requireCompile("xa+"_el)); }

    void compilePrefixSingleCharOneOrMorePostfix() { WITH_CONTEXT(requireCompile("xa+a"_el)); }

    void compilePrefixGroupOneOrMorePostfix() { WITH_CONTEXT(requireCompile("x(?:ab)+y"_el)); }

    void compileNestedGroupsOneOrMore() { WITH_CONTEXT(requireCompile("x(?:a+b+(?:xyz(?:123)+)+)+y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "aa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "b"_el,
                "ba"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "ax"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharOneOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaaa"_el},
                {"xaaaa"_el, 0, "xaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "xa"_el,
                "y"_el,
                "😀xaa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupOneOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaby"_el, 0, "xaby"_el},
                {"xabababy"_el, 0, "xabababy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "xy"_el,
                "xay"_el,
                "xab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xabxyz123y"_el, 0, "xabxyz123y"_el},
                {"xaabxyz123xyz123123y"_el, 0, "xaabxyz123xyz123123y"_el},
                {"xaabxyz123abbxyz123123y"_el, 0, "xaabxyz123abbxyz123123y"_el},
                {"xabxyz123abxyz123abxyz123y"_el, 0, "xabxyz123abxyz123abxyz123y"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "xy"_el,
                "xaby"_el,
                "xxyz123y"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharOneOrMore());
        {
            const auto matchCases = std::vector<StringView>{
                "a"_el,
                "aa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "b"_el,
                "ab"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharOneOrMorePostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xaa"_el,
                "xaaa"_el,
                "xaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "xa"_el,
                "x"_el,
                "xaaaab"_el,
                ""_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupOneOrMorePostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xaby"_el,
                "xabababy"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "xy"_el,
                "xab"_el,
                "xabay"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharOneOrMorePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaaa"_el, 2, "xaaaa"_el},
            {"zzxaaa"_el, 2, "xaaa"_el},
            {"zzxaa"_el, 2, "xaa"_el},
            {"xaa"_el, 0, "xaa"_el},
            {"xaaaa"_el, 0, "xaaaa"_el},
            {"😀xaaa"_el, 4, "xaaa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "y"_el,
            "😀"_el,
            "xa"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharOneOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xaaa'",
            "Match 02:",
            "00: 0005-0008 'xaa'",
            "Match 03:",
            "00: 0009-0014 'xaaaa'",
        };
        WITH_CONTEXT(requireFindAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "y"_el,
            "xa"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharOneOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xaaa'",
            "Match 02:",
            "00: 0005-0008 'xaa'",
            "Match 03:",
            "00: 0009-0014 'xaaaa'",
        };
        WITH_CONTEXT(requireCollectAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharOneOrMorePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"xaaa"_el, ""_el, ""_el},
            {"xaaa"_el, "<x>"_el, "<x>"_el},
            {"xaaa xa xaa"_el, "<x>"_el, "<x> xa <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
