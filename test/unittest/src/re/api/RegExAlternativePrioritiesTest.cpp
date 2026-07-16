// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Alternation)
class RegExAlternativePrioritiesTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void testPriorities() {
        // In the Thompson NFA, it's important that the order of the threads are correct,
        // as this order defines the priority.
        // Regex engines evaluate alternations left to right and stop at the first successful alternative.
        // The following expression is testing this behavior.
        // When matching the text "a", group 1 must match, as this thread must have the higher priority as
        // the thread containing group 2.

        WITH_CONTEXT(requireCompile("(?:(?:x|(a))|(a))"_el));
        WITH_CONTEXT(requireFullMatch("a"_el));
        matchLines = createGroupLines();
        const auto expectedLines = std::vector<std::string>{
            "00: 0000-0001 'a'",
            "01: 0000-0001 'a'",
            "02: 0000-0000 ''",
        };
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }
};
