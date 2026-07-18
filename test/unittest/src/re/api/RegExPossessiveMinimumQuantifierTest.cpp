// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Possessive)
class RegExPossessiveMinimumQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharMinZeroOrMore() { WITH_CONTEXT(requireCompile("a{0,}+"_el)); }

    void compileSingleCharMinOneOrMore() { WITH_CONTEXT(requireCompile("a{1,}+"_el)); }

    void compileSingleCharMinTwoOrMore() { WITH_CONTEXT(requireCompile("a{2,}+"_el)); }

    void compileSingleCharMinTwentyFiveOrMore() { WITH_CONTEXT(requireCompile("a{25,}+"_el)); }

    void compilePrefixSingleCharMinTwoOrMore() { WITH_CONTEXT(requireCompile("xa{2,}+"_el)); }

    void compilePrefixSingleCharMinTwoOrMorePostfix() { WITH_CONTEXT(requireCompile("xa{2,}+b"_el)); }

    void compileBacktrackingSensitiveMinTwoOrMore() { WITH_CONTEXT(requireCompile("a{2,}+a"_el)); }

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

            const auto noMatchCases = std::vector<String>{
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

            const auto noMatchCases = std::vector<String>{
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

            const auto noMatchCases = std::vector<String>{
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

            const auto noMatchCases = std::vector<String>{
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
                {"xaab"_el, 0, "xaab"_el},
                {"xaaab"_el, 0, "xaaab"_el},
                {"xaaaab"_el, 0, "xaaaab"_el},
                {"xaabx"_el, 0, "xaab"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "xa"_el,
                "y"_el,
                "😀xaab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    void testNoBacktrackingBehavior() {
        // Greedy minimum quantifier could backtrack to satisfy the trailing "a".
        WITH_CONTEXT(compileBacktrackingSensitiveMinTwoOrMore());
        const auto noMatchCases = std::vector<String>{
            "aaa"_el,
            "aaaa"_el,
            "aaaaa"_el,
            "aaab"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharMinZeroOrMore());
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
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileSingleCharMinTwentyFiveOrMore());
        {
            const auto matchCases = std::vector<String>{
                "aaaaaaaaaaaaaaaaaaaaaaaaa"_el,
                "aaaaaaaaaaaaaaaaaaaaaaaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "aaaaaaaaaaaaaaaaaaaaaaaa"_el,
                "b"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaab"_el, 2, "xaaab"_el},
            {"zzxaab"_el, 2, "xaab"_el},
            {"zzxaaaab"_el, 2, "xaaaab"_el},
            {"xaab"_el, 0, "xaab"_el},
            {"xaaaab"_el, 0, "xaaaab"_el},
            {"😀xaab"_el, 4, "xaab"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "x"_el,
            "xab"_el,
            "y"_el,
            "😀"_el,
            "xa"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xaaab'",
            "Match 02:",
            "00: 0006-0010 'xaab'",
            "Match 03:",
            "00: 0011-0017 'xaaaab'",
        };
        WITH_CONTEXT(requireFindAll("xaaab xaab xaaaab"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaab xaab xaaaab"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xaaab'",
            "Match 02:",
            "00: 0006-0010 'xaab'",
            "Match 03:",
            "00: 0011-0017 'xaaaab'",
        };
        WITH_CONTEXT(requireCollectAll("xaaab xaab xaaaab"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaab xaab xaaaab"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharMinTwoOrMorePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {"xaab"_el, ""_el, ""_el},
            {"xaab"_el, "<x>"_el, "<x>"_el},
            {"xaaab xa xaaaab"_el, "<x>"_el, "<x> xa <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
