// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

/// Test if a nested atomic group works as expected.
TESTED_TARGETS(RegEx)
TAGS(Api AtomicGroup)
class RegExAtomicGroupNestedTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void testPattern1() {
        // Inner atomic must block backtracking inside the outer group
        WITH_CONTEXT(requireCompile("^(?>(?>a|ab)c)$"_el));
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"ac"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

            const auto noMatchCases = std::vector<StringView>{
                ""_el,
                "abc"_el, // critical case.
                "xyz"_el,
            };
            WITH_CONTEXT(requireNoMatch(noMatchCases));
        }

        // compare with non-atomic variant.
        WITH_CONTEXT(requireCompile("^(?>(?:a|ab)c)$"_el));
        {
            const auto matchCases = NoneCapGroupTestCases{
                {"ac"_el},
                {"abc"_el},
            };
            WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));
        }
    }

    void testPattern2() {
        // Inner atomic must not wipe outer backtracking states ("don't cut too far")
        WITH_CONTEXT(requireCompile("^(?>(?:a|ab)(?>b|bc)d)$"_el));
        const auto matchCases = NoneCapGroupTestCases{
            {"abd"_el},
            {"abbd"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "abcd"_el, // critical case.
            "xyz"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }

    void testPattern3() {
        // Inner atomic commits, then the outer alternation must still be tryable if the first branch fails
        WITH_CONTEXT(requireCompile("^(?>(?:a(?>bb|b)c)|ab)$"_el));
        const auto matchCases = NoneCapGroupTestCases{
            {"ab"_el},
            {"abc"_el},
            {"abbc"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            "ac"_el,
            "abb"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }
};
