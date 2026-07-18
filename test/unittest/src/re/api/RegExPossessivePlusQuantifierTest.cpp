// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Possessive)
class RegExPossessivePlusQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileAmbiguousGroupPlus() {
        // Possessive plus quantifier must not backtrack inside the repeated group.
        WITH_CONTEXT(requireCompile("(?:ab|a)++b"_el));
    }

    void compileAmbiguousGroupPlusWithPrefixAndPostfix() { WITH_CONTEXT(requireCompile("x(?:ab|a)++by"_el)); }

    void testMatchBehaviorDifferenceToGreedy() {
        // Greedy quantifiers could backtrack to match "ab" or "xaby".
        // The possessive variant must reject these inputs instead.
        WITH_CONTEXT(compileAmbiguousGroupPlus());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"abb"_el, 0, "abb"_el},
                {"aabb"_el, 0, "aabb"_el},
                {"abbc"_el, 0, "abb"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "a"_el,
                "ab"_el,
                "aab"_el,
                "aba"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileAmbiguousGroupPlusWithPrefixAndPostfix());
        {
            const auto matchCases = std::vector<String>{
                "xabby"_el,
                "xaabby"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "xaby"_el,
                "xaaby"_el,
                "xabbby"_el,
                "xabb"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileAmbiguousGroupPlusWithPrefixAndPostfix());
        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxabby"_el, 2, "xabby"_el},
            {"zzxaabby"_el, 2, "xaabby"_el},
            {"xabby"_el, 0, "xabby"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "xaby"_el,
            "zzxaby"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(compileAmbiguousGroupPlusWithPrefixAndPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xabby'",
            "Match 02:",
            "00: 0011-0017 'xaabby'",
        };
        WITH_CONTEXT(requireFindAll("xabby xaby xaabby"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireFindAll("xabby xaby xaabby"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "xaby"_el,
            "xabb"_el,
        };
        WITH_CONTEXT(requireNoFindAll(noMatchCases));
    }

    void testCollectAll() {
        WITH_CONTEXT(compileAmbiguousGroupPlusWithPrefixAndPostfix());

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0005 'xabby'",
            "Match 02:",
            "00: 0011-0017 'xaabby'",
        };
        WITH_CONTEXT(requireCollectAll("xabby xaby xaabby"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireCollectAll("xabby xaby xaabby"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(compileAmbiguousGroupPlusWithPrefixAndPostfix());
        const auto testCases = ReplaceTestCases{
            {""_el, "<x>"_el, ""_el},
            {"xaby"_el, "<x>"_el, "xaby"_el},
            {"xabby"_el, "<x>"_el, "<x>"_el},
            {"xabby xaabby"_el, "<x>"_el, "<x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
