// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Possessive)
class RegExPossessiveExactQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharExact() { WITH_CONTEXT(requireCompile("a{3}+"_el)); }

    void compileAmbiguousGroupExact() {
        // Possessive exact quantifier: repetitions are fixed (2), but the repeated expression is ambiguous.
        // The possessive quantifier must not backtrack inside the quantified section.
        WITH_CONTEXT(requireCompile("(?:aa|a){2}+a"_el));
    }

    void compileAmbiguousGroupExactWithPrefixAndPostfix() { WITH_CONTEXT(requireCompile("x(?:aa|a){2}+ay"_el)); }

    void testMatchBehaviorDifferenceToGreedy() {
        // Greedy variant can backtrack inside the quantified section and therefore matches "aaa".
        {
            WITH_CONTEXT(requireCompile("(?:aa|a){2}a"_el));
            const auto greedyMatchCases = NoneCapGroupTestCases{
                {"aaa"_el, 0, "aaa"_el},
                {"aaaa"_el, 0, "aaaa"_el},
                {"aaaaa"_el, 0, "aaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(greedyMatchCases));
        }

        // Possessive variant must not backtrack inside the exact quantifier and therefore fails on "aaa".
        WITH_CONTEXT(compileAmbiguousGroupExact());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aaaaa"_el, 0, "aaaaa"_el},
                {"aaaaaa"_el, 0, "aaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "a"_el,
                "aa"_el,
                "aaa"_el,
                "aaaa"_el,
                "aaab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharExact());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aaa"_el, 0, "aaa"_el},
                {"aaaa"_el, 0, "aaa"_el},
                {"aaab"_el, 0, "aaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "aa"_el,
                "baa"_el,
                "😀"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileAmbiguousGroupExact());
        {
            const auto matchCases = std::vector<String>{
                "aaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "aaa"_el,
                "aaaa"_el,
                "aaaaaa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileAmbiguousGroupExact());
        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzzaaaaa"_el, 3, "aaaaa"_el},
            {"aaaaa"_el, 0, "aaaaa"_el},
            {"aaaaaa"_el, 0, "aaaaa"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "aaa"_el,
            "zzzaaa"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    void testReplace() {
        WITH_CONTEXT(compileAmbiguousGroupExactWithPrefixAndPostfix());
        const auto testCases = ReplaceTestCases{
            {""_el, "<x>"_el, ""_el},
            {"xaaay"_el, "<x>"_el, "xaaay"_el},
            {"xaaaaay"_el, "<x>"_el, "<x>"_el},
            {"xaaaaay xaaaaay"_el, "<x>"_el, "<x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
