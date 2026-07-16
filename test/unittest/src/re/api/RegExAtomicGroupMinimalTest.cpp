// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

/// Test if atomic groups operations work at a minimal level.
/// No pruning should be performed.
TESTED_TARGETS(RegEx)
TAGS(Api AtomicGroup)
class RegExAtomicGroupMinimalTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compilePattern() { WITH_CONTEXT(requireCompile("(?>a)"_el)); }

    void testIfAtomicOperationIsInCode() {
        compilePattern();
        const auto lines = diagnostics::Disassembler{regex}.disassemble();
        bool hasStartAtomic = false;
        bool hasStopAtomic = false;
        for (const auto &line : lines) {
            if (line.contains("START ATOMIC"_el)) {
                hasStartAtomic = true;
            }
            if (line.contains("STOP ATOMIC"_el)) {
                hasStopAtomic = true;
            }
        }
        REQUIRE(hasStartAtomic);
        REQUIRE(hasStopAtomic);
    }

    TESTED_TARGETS(match)
    void testMatch() {
        compilePattern();
        const auto matchCases = NoneCapGroupTestCases{
            {"a"_el},
            {"abc"_el, 0, "a"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "b"_el,
            "xyz"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }
    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        compilePattern();
        const auto matchCases = std::vector<StringView>{"a"_el};
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "abc"_el,
            "b"_el,
            "xyz"_el,
        };
        WITH_CONTEXT(requireNoFullMatch(noMatchCases));
    }
    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        compilePattern();
        const auto matchCases = NoneCapGroupTestCases{
            {"abc"_el, 0, "a"_el},
            {"xyza"_el, 3, "a"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "xyz"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }
    TESTED_TARGETS(findAll)
    void testFindAll() {
        compilePattern();
        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0003-0004 'a'",
            "Match 02:",
            "00: 0007-0008 'a'",
            "Match 03:",
            "00: 0008-0009 'a'",
        };
        WITH_CONTEXT(requireFindAll("012axyzaa"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireNoFindAll("0123456789"_el));
    }
};
