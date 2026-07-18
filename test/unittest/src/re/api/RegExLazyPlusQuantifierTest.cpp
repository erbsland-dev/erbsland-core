// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Lazy)
class RegExLazyPlusQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharLazyOneOrMore() { WITH_CONTEXT(requireCompile("a+?"_el)); }

    void compilePrefixSingleCharLazyOneOrMore() { WITH_CONTEXT(requireCompile("xa+?"_el)); }

    void compilePrefixSingleCharLazyOneOrMorePostfix() { WITH_CONTEXT(requireCompile("xa+?a"_el)); }

    void compilePrefixGroupLazyOneOrMorePostfix() { WITH_CONTEXT(requireCompile("x(?:ab)+?y"_el)); }

    void compileNestedGroupsLazyOneOrMore() { WITH_CONTEXT(requireCompile("x(?:a+?b+?(?:xyz(?:123)+?)+?)+?y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharLazyOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "a"_el},
                {"aaa"_el, 0, "a"_el},
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

        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xa"_el},
                {"xaaa"_el, 0, "xa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "ax"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaa"_el, 0, "xaa"_el},
                {"xaaa"_el, 0, "xaa"_el},
                {"xaaaa"_el, 0, "xaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "xa"_el,
                "y"_el,
                "😀xaa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupLazyOneOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaby"_el, 0, "xaby"_el},
                {"xabababy"_el, 0, "xabababy"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "xa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsLazyOneOrMore());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xabxyz123y"_el, 0, "xabxyz123y"_el},
                {"xaabxyz123xyz123123y"_el, 0, "xaabxyz123xyz123123y"_el},
                {"xaabxyz123abbxyz123123y"_el, 0, "xaabxyz123abbxyz123123y"_el},
                {"xabxyz123abxyz123abxyz123y"_el, 0, "xabxyz123abxyz123abxyz123y"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "x"_el,
                "y"_el,
                "xa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharLazyOneOrMore());
        {
            const auto matchCases = std::vector<String>{
                "a"_el,
                "aa"_el,
                "aaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "b"_el,
                "ab"_el,
                "ba"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMorePostfix());
        {
            const auto matchCases = std::vector<String>{
                "xaa"_el,
                "xaaa"_el,
                "xaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "xa"_el,
                "x"_el,
                ""_el,
                "a"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMorePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaaa"_el, 2, "xaa"_el},
            {"zzxaaa"_el, 2, "xaa"_el},
            {"zzxaa"_el, 2, "xaa"_el},
            {"xaa"_el, 0, "xaa"_el},
            {"xaaaa"_el, 0, "xaa"_el},
            {"😀xaaa"_el, 4, "xaa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
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
        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'xaa'",
            "Match 02:",
            "00: 0005-0008 'xaa'",
            "Match 03:",
            "00: 0009-0012 'xaa'",
        };
        WITH_CONTEXT(requireFindAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "x"_el,
            "y"_el,
            "xa"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'xaa'",
            "Match 02:",
            "00: 0005-0008 'xaa'",
            "Match 03:",
            "00: 0009-0012 'xaa'",
        };
        WITH_CONTEXT(requireCollectAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaaa xaa xaaaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOneOrMorePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {"xaaa"_el, ""_el, "a"_el},
            {"xaaa"_el, "<x>"_el, "<x>a"_el},
            {"xaaa xa xaa"_el, "<x>"_el, "<x>a xa <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
