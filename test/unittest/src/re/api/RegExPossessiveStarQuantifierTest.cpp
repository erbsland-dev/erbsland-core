// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Possessive)
class RegExPossessiveStarQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharZeroOrMore() { WITH_CONTEXT(requireCompile("a*+"_el)); }

    void compileAmbiguousGroupZeroOrMorePostfix() {
        // Possessive star quantifier: do not backtrack inside the quantified section.
        WITH_CONTEXT(requireCompile("x(?:ab|a)*+b"_el));
    }

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

        WITH_CONTEXT(compileAmbiguousGroupZeroOrMorePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xb"_el, 0, "xb"_el},
                {"xabb"_el, 0, "xabb"_el},
                {"xababb"_el, 0, "xababb"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            // The possessive quantifier consumes the longest possible match and does not backtrack.
            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "xab"_el,
                "xabab"_el,
                "xaba"_el,
                "xa"_el,
                "y"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileAmbiguousGroupZeroOrMorePostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xb"_el,
                "xabb"_el,
                "xababb"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "xab"_el,
                "xabab"_el,
                "xaba"_el,
                "xa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileAmbiguousGroupZeroOrMorePostfix());

        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxabb"_el, 2, "xabb"_el},
            {"zzxababxabb"_el, 7, "xabb"_el},
            {"xabb"_el, 0, "xabb"_el},
            {"😀xabb"_el, 4, "xabb"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "xabab"_el,
            "zzxabab"_el,
            "xaba"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compileAmbiguousGroupZeroOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xabb'",
            "Match 02:",
            "00: 0011-0015 'xabb'",
        };
        WITH_CONTEXT(requireFindAll("xabb xabab xabb"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xabb xabab xabb"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "xabab"_el,
            "xaba"_el,
            "y"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compileAmbiguousGroupZeroOrMorePostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0004 'xabb'",
            "Match 02:",
            "00: 0011-0015 'xabb'",
        };
        WITH_CONTEXT(requireCollectAll("xabb xabab xabb"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xabb xabab xabb"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compileAmbiguousGroupZeroOrMorePostfix());

        const auto testCases = ReplaceTestCases{
            {""_el, "<x>"_el, ""_el},
            {"xabb"_el, "<x>"_el, "<x>"_el},
            {"xabab"_el, "<x>"_el, "xabab"_el},
            {"xabb xabab xabb"_el, "<x>"_el, "<x> xabab <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
