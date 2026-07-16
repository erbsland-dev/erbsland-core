// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Greedy)
class RegExGreedyMinimumQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharMinZeroOrMore() { WITH_CONTEXT(requireCompile("a{0,}"_el)); }

    void compileSingleCharMinOneOrMore() { WITH_CONTEXT(requireCompile("a{1,}"_el)); }

    void compileSingleCharMinTwoOrMore() { WITH_CONTEXT(requireCompile("a{2,}"_el)); }

    void compileSingleCharMinTwentyFiveOrMore() { WITH_CONTEXT(requireCompile("a{25,}"_el)); }

    void compilePrefixSingleCharMinTwoOrMore() { WITH_CONTEXT(requireCompile("xa{2,}"_el)); }

    void compilePrefixSingleCharMinTwoOrMorePostfix() { WITH_CONTEXT(requireCompile("xa{2,}a"_el)); }

    void compilePrefixGroupMinTwoOrMorePostfix() { WITH_CONTEXT(requireCompile("x(?:ab){2,}y"_el)); }

    void compileNestedGroupsMinTwoOrMore() { WITH_CONTEXT(requireCompile("x(?:a{2,}b{2,}(?:xy){2,}){2,}y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharMinZeroOrMore());
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

        WITH_CONTEXT(compileSingleCharMinOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "aa"_el},
                {"aaa"_el, 0, "aaa"_el},
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

        WITH_CONTEXT(compileSingleCharMinTwoOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aa"_el, 0, "aa"_el},
                {"aaa"_el, 0, "aaa"_el},
                {"aaaa"_el, 0, "aaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "a"_el,
                "b"_el,
                "ba"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileSingleCharMinTwentyFiveOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aaaaaaaaaaaaaaaaaaaaaaaaa"_el, 0, "aaaaaaaaaaaaaaaaaaaaaaaaa"_el},
                {"aaaaaaaaaaaaaaaaaaaaaaaaaa"_el, 0, "aaaaaaaaaaaaaaaaaaaaaaaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "aaaaaaaaaaaaaaaaaaaaaaaa"_el,
                "b"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMore());
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
                "ax"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaaa"_el, 0, "xaaa"_el},
                {"xaaaa"_el, 0, "xaaaa"_el},
                {"xaaaaa"_el, 0, "xaaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "xa"_el,
                "xaa"_el,
                "y"_el,
                "😀xaaa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupMinTwoOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xababy"_el, 0, "xababy"_el},
                {"xabababy"_el, 0, "xabababy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "xy"_el,
                "xaby"_el,
                "xabab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsMinTwoOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaabbxyxyaabbxyxyy"_el, 0, "xaabbxyxyaabbxyxyy"_el},
                {"xaaabbbxyxyxyaabbxyxyy"_el, 0, "xaaabbbxyxyxyaabbxyxyy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "xy"_el,
                "xaabbxyxyy"_el,
                "xaabbxyaabbxyxyy"_el,
                "xaabbxyxyaabbxyxy"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharMinZeroOrMore());
        {
            const auto matchCases = std::vector<StringView>{
                ""_el,
                "a"_el,
                "aa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "b"_el,
                "ab"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileSingleCharMinOneOrMore());
        {
            const auto matchCases = std::vector<StringView>{
                "a"_el,
                "aa"_el,
                "aaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "b"_el,
                "ab"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileSingleCharMinTwentyFiveOrMore());
        {
            const auto matchCases = std::vector<StringView>{
                "aaaaaaaaaaaaaaaaaaaaaaaaa"_el,
                "aaaaaaaaaaaaaaaaaaaaaaaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "aaaaaaaaaaaaaaaaaaaaaaaa"_el,
                "b"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xaaa"_el,
                "xaaaa"_el,
                "xaaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "xaa"_el,
                "x"_el,
                "xaaaaab"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaaa"_el, 2, "xaaaa"_el},
            {"zzxaaa"_el, 2, "xaaa"_el},
            {"zzxaaaaa"_el, 2, "xaaaaa"_el},
            {"xaaa"_el, 0, "xaaa"_el},
            {"xaaaa"_el, 0, "xaaaa"_el},
            {"😀xaaaa"_el, 4, "xaaaa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "xa"_el,
            "y"_el,
            "😀"_el,
            "xaa"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xaaaa'",
            "Match 02:",
            "00: 0006-0010 'xaaa'",
            "Match 03:",
            "00: 0011-0017 'xaaaaa'",
        };
        WITH_CONTEXT(requireFindAll("xaaaa xaaa xaaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaaa xaaa xaaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xaaaa'",
            "Match 02:",
            "00: 0006-0010 'xaaa'",
            "Match 03:",
            "00: 0011-0017 'xaaaaa'",
        };
        WITH_CONTEXT(requireCollectAll("xaaaa xaaa xaaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaaa xaaa xaaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {""_el, "<x>"_el, ""_el},
            {"xaaaa"_el, ""_el, ""_el},
            {"xaaaa"_el, "<x>"_el, "<x>"_el},
            {"xaaaa xa xaaa"_el, "<x>"_el, "<x> xa <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
