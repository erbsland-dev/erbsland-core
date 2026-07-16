// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Lazy)
class RegExLazyQuestionQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharLazyOptional() { WITH_CONTEXT(requireCompile("a??"_el)); }

    void compilePrefixSingleCharLazyOptional() { WITH_CONTEXT(requireCompile("xa??"_el)); }

    void compilePrefixSingleCharLazyOptionalPostfix() { WITH_CONTEXT(requireCompile("xa??a"_el)); }

    void compilePrefixGroupLazyOptionalPostfix() { WITH_CONTEXT(requireCompile("x(?:ab)??y"_el)); }

    void compileNestedGroupsLazyOptional() { WITH_CONTEXT(requireCompile("x(?:a??b??(?:xyz(?:123)??)??)??y"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharLazyOptional());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {""_el, 0, ""_el},
                {"a"_el, 0, ""_el},
                {"aa"_el, 0, ""_el},
                {"ba"_el, 0, ""_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }

        WITH_CONTEXT(compilePrefixSingleCharLazyOptional());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"x"_el, 0, "x"_el},
                {"xa"_el, 0, "x"_el},
                {"xaa"_el, 0, "x"_el},
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

        WITH_CONTEXT(compilePrefixSingleCharLazyOptionalPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xa"_el, 0, "xa"_el},
                {"xaa"_el, 0, "xa"_el},
                {"xaaa"_el, 0, "xa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "x"_el,
                "y"_el,
                "zaa"_el,
                "😀xaa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compilePrefixGroupLazyOptionalPostfix());
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
                "xa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileNestedGroupsLazyOptional());
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
        WITH_CONTEXT(compileSingleCharLazyOptional());
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

        WITH_CONTEXT(compilePrefixSingleCharLazyOptionalPostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xa"_el,
                "xaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "x"_el,
                "xaaa"_el,
                ""_el,
                "a"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOptionalPostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaa"_el, 2, "xa"_el},
            {"zzxa"_el, 2, "xa"_el},
            {"xa"_el, 0, "xa"_el},
            {"xaa"_el, 0, "xa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "y"_el,
            "zaa"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOptionalPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0002 'xa'",
            "Match 02:",
            "00: 0004-0006 'xa'",
        };
        WITH_CONTEXT(requireFindAll("xaa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xaa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "x"_el,
            "y"_el,
            "zaa"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOptionalPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0002 'xa'",
            "Match 02:",
            "00: 0004-0006 'xa'",
        };
        WITH_CONTEXT(requireCollectAll("xaa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xaa xaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compilePrefixSingleCharLazyOptionalPostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, ""_el, ""_el},
            {"xaa"_el, ""_el, "a"_el},
            {"xaa"_el, "<x>"_el, "<x>a"_el},
            {"xaa xaa"_el, "<x>"_el, "<x>a <x>a"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
