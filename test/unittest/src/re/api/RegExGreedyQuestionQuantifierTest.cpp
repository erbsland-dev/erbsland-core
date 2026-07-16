// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Greedy)
class RegExGreedyQuestionQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharOptional() { WITH_CONTEXT(requireCompile("a?"_el)); }

    void compilePrefixSingleCharOptional() { WITH_CONTEXT(requireCompile("xa?"_el)); }

    void compilePrefixSingleCharOptionalPostfix() { WITH_CONTEXT(requireCompile("xa?y"_el)); }

    void compilePrefixGroupOptionalPostfix() { WITH_CONTEXT(requireCompile("x(?:ab)?y"_el)); }

    void compileNestedGroupsOptional() { WITH_CONTEXT(requireCompile("x(?:a?b?(?:xyz(?:123)?)?)?y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharOptional());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {""_el, 0, ""_el},
                {"a"_el, 0, "a"_el},
                {"aa"_el, 0, "a"_el},
                {"ba"_el, 0, ""_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharOptional());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"x"_el, 0, "x"_el},
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "yx"_el,
                "a"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharOptionalPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xy"_el, 0, "xy"_el},
                {"xay"_el, 0, "xay"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "xaby"_el,
                "ay"_el,
                "y"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupOptionalPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xy"_el, 0, "xy"_el},
                {"xaby"_el, 0, "xaby"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "xab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsOptional());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xy"_el, 0, "xy"_el},
                {"xay"_el, 0, "xay"_el},
                {"xby"_el, 0, "xby"_el},
                {"xaby"_el, 0, "xaby"_el},
                {"xxyz123y"_el, 0, "xxyz123y"_el},
                {"xabxyz123y"_el, 0, "xabxyz123y"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
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
        WITH_CONTEXT(compileSingleCharOptional());
        {
            const auto matchCases = std::vector<StringView>{
                ""_el,
                "a"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "aa"_el,
                "b"_el,
                "ab"_el,
                "ba"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupOptionalPostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xy"_el,
                "xaby"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "xa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsOptional());
        {
            const auto matchCases = std::vector<StringView>{
                "xy"_el,
                "xay"_el,
                "xby"_el,
                "xaby"_el,
                "xxyz123y"_el,
                "xabxyz123y"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
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
        WITH_CONTEXT(compilePrefixSingleCharOptionalPostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxay"_el, 2, "xay"_el},
            {"zzxy"_el, 2, "xy"_el},
            {"xay"_el, 0, "xay"_el},
            {"xy"_el, 0, "xy"_el},
            {"😀xy"_el, 4, "xy"_el},
            {"😀xay"_el, 4, "xay"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "y"_el,
            "xaby"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharOptionalPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'xay'",
            "Match 02:",
            "00: 0004-0006 'xy'",
            "Match 03:",
            "00: 0007-0010 'xay'",
        };
        WITH_CONTEXT(requireFindAll("xay xy xay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xay xy xay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharOptionalPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0003 'xay'",
            "Match 02:",
            "00: 0004-0006 'xy'",
            "Match 03:",
            "00: 0007-0010 'xay'",
        };
        WITH_CONTEXT(requireCollectAll("xay xy xay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xay xy xay"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharOptionalPostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {"xay"_el, ""_el, ""_el},
            {"xay"_el, "<x>"_el, "<x>"_el},
            {"xay xy xay"_el, "<x>"_el, "<x> <x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
