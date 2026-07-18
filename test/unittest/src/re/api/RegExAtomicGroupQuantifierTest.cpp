// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

/// Test if atomic groups actually "lock-in" the greedy match and do not "back-track".
/// Pruning should be performed to only keep the longest greedy match.
TESTED_TARGETS(RegEx)
TAGS(Api AtomicGroup)
class RegExAtomicGroupQuantifierTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compilePattern() { WITH_CONTEXT(requireCompile("(?>a+)[ab]"_el)); }

    TESTED_TARGETS(match)
    void testMatch() {
        compilePattern();
        const auto matchCases = NoneCapGroupTestCases{
            {"ab"_el},
            {"aab"_el},
            {"aaab"_el},
            {"aaabxyz"_el, 0, "aaab"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "aa"_el,
            "aaa"_el,
            "aaaa"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }
    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        compilePattern();
        auto veryLongMatch = StringEditor::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{10'000U});
        veryLongMatch.append("b"_el);
        const auto matchCases = std::vector<String>{
            "ab"_el,
            "aab"_el,
            "aaab"_el,
            veryLongMatch,
        };
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

        const auto veryLongNoMatch = String::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{10'000U});
        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "aa"_el,
            "aaa"_el,
            "aaaa"_el,
            "aaaabxyz"_el,
            veryLongNoMatch,
        };
        WITH_CONTEXT(requireNoFullMatch(noMatchCases));
    }
    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        compilePattern();
        const auto matchCases = NoneCapGroupTestCases{
            {"ab"_el, 0, "ab"_el},
            {"aaaab"_el, 0, "aaaab"_el},
            {"xyzaab"_el, 3, "aab"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(matchCases));

        const auto noMatchCases = std::vector<String>{
            ""_el,
            "a"_el,
            "aa"_el,
            "aaaaaaaa"_el,
            "xyzaaaaaaaa"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }
    TESTED_TARGETS(findAll)
    void testFindAll() {
        compilePattern();
        {
            const auto expectedLines = std::vector<std::string>{
                "Match 01:",
                "00: 0000-0005 'aaaab'",
                "Match 02:",
                "00: 0005-0010 'aaaab'",
                "Match 03:",
                "00: 0010-0015 'aaaab'",
            };
            WITH_CONTEXT(requireFindAll("aaaabaaaabaaaab"_el));
            WITH_CONTEXT(requireLines(matchLines, expectedLines));
        }
        {
            const auto expectedLines = std::vector<std::string>{
                "Match 01:",
                "00: 0003-0006 'aab'",
                "Match 02:",
                "00: 0009-0011 'ab'",
                "Match 03:",
                "00: 0014-0018 'aaab'",
            };
            WITH_CONTEXT(requireFindAll("xyzaabayxab012aaab"_el));
            WITH_CONTEXT(requireLines(matchLines, expectedLines));
        }
        WITH_CONTEXT(requireNoFindAll("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_el));
    }
};
