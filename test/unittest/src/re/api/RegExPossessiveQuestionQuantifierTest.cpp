// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Quantifiers Possessive)
class RegExPossessiveQuestionQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compileSingleCharOptional() { WITH_CONTEXT(requireCompile("a?+"_el)); }

    void compileSingleCharOptionalWithRequiredPostfix() { WITH_CONTEXT(requireCompile("a?+a"_el)); }

    void compileOptionalAmbiguousGroupWithPrefixAndPostfix() { WITH_CONTEXT(requireCompile("x(?:aa|a)?+ay"_el)); }

    void testMatchBehaviorDifferenceToGreedy() {
        WITH_CONTEXT(compileSingleCharOptionalWithRequiredPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"aa"_el, 0, "aa"_el},
                {"aaa"_el, 0, "aa"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "a"_el,
                "ba"_el,
                "ab"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        WITH_CONTEXT(compileOptionalAmbiguousGroupWithPrefixAndPostfix());
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"xaaay"_el, 0, "xaaay"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "xay"_el,
                "xaay"_el,
                "xaby"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(compileSingleCharOptional());
        const auto matchCases = NoneCapGroupTestCases{
            {""_el, 0, ""_el},
            {"a"_el, 0, "a"_el},
            {"aa"_el, 0, "a"_el},
            {"b"_el, 0, ""_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(compileSingleCharOptionalWithRequiredPostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "aa"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "a"_el,
                "aaa"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }

        WITH_CONTEXT(compileOptionalAmbiguousGroupWithPrefixAndPostfix());
        {
            const auto matchCases = std::vector<StringView>{
                "xaaay"_el,
            };
            WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                "xay"_el,
                "xaay"_el,
                "xaaaay"_el,
            };
            WITH_CONTEXT(requireNoFullMatch(noMatchCases));
        }
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(compileOptionalAmbiguousGroupWithPrefixAndPostfix());
        const auto testCases = std::vector<NoCaptureTestCase>{
            {"zzxaaay"_el, 2, "xaaay"_el},
            {"xaaay"_el, 0, "xaaay"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(testCases));

        const auto noMatchCases = std::vector<StringView>{
            "zzxay"_el,
            "xaay"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }

    void testReplace() {
        WITH_CONTEXT(compileSingleCharOptionalWithRequiredPostfix());
        const auto testCases = ReplaceTestCases{
            {""_el, "<x>"_el, ""_el},
            {"a"_el, "<x>"_el, "a"_el},
            {"aa"_el, "<x>"_el, "<x>"_el},
            {"aaa aa"_el, "<x>"_el, "<x>a <x>"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }
};
