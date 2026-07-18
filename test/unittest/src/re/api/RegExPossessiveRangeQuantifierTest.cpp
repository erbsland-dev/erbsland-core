// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Possessive)
class RegExPossessiveRangeQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileAmbiguousGroupRangePostfix() {
        // Possessive range quantifier: the quantified expression is ambiguous and must not backtrack.
        WITH_CONTEXT(requireCompile("(?:aa|a){2,3}+aa"_el));
    }

    void compileAmbiguousGroupRangeWithPrefixAndPostfix() { WITH_CONTEXT(requireCompile("x(?:aa|a){2,3}+aay"_el)); }

    void testMatchBehaviorDifferenceToGreedy() {
        // The possessive quantifier must not backtrack inside the quantified section.
        WITH_CONTEXT(compileAmbiguousGroupRangePostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aaaaaaaa"_el, 0, "aaaaaaaa"_el},
                {"aaaaaaaaa"_el, 0, "aaaaaaaa"_el},
                {"aaaaaaaaaa"_el, 0, "aaaaaaaa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<String>{
                ""_el,
                "a"_el,
                "aa"_el,
                "aaa"_el,
                "aaaa"_el,
                "aaaaa"_el,
                "aaaaaa"_el,
                "aaaaaaa"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileAmbiguousGroupRangePostfix());
        {
            const auto matchCases = std::vector<String>{
                "aaaaaaaa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<String>{
                "aaaaaaa"_el,
                "aaaaaaaaa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileAmbiguousGroupRangeWithPrefixAndPostfix());
        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaaaaaaay"_el, 2, "xaaaaaaaay"_el},
            {"xaaaaaaaay"_el, 0, "xaaaaaaaay"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "xaaaaaaay"_el,
            "xaaaaaaaaay"_el,
            "😀"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    void testReplace() {
        WITH_CONTEXT(compileAmbiguousGroupRangeWithPrefixAndPostfix());
        const auto testCases = ReplaceTestCases{
            {""_el, "<x>"_el, ""_el},
            {"xaaaaaaaay"_el, "<x>"_el, "<x>"_el},
            {"xaaaaaaaay xaaaaaaaay"_el, "<x>"_el, "<x> <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
