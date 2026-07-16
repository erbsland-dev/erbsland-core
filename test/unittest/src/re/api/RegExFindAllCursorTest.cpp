// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

TESTED_TARGETS(RegEx findAll)
TAGS(Engine)
class RegExFindAllCursorTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    /// As an empty pattern always matches, `findAll` must match at every position.
    /// For the text `abc` these are four position between the three characters,
    void testEmptyPattern() {
        Settings settings;
        settings.enableFeature(Feature::EmptyGroups);
        WITH_CONTEXT(requireCompile(""_el, {}, settings));
        WITH_CONTEXT(requireFindAll("abc"_el));
        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0001-0001 ''",
            "Match 03:",
            "00: 0002-0002 ''",
            "Match 04:",
            "00: 0003-0003 ''",
        };
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    /// Word boundary matches are also zero-width matches.
    /// In the text `ab cd` there are four positions of word-boundaries that should produce a match.
    void testWordBoundary() {
        WITH_CONTEXT(requireCompile("\\b"_el));
        WITH_CONTEXT(requireFindAll("ab cd"_el));
        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0000 ''",
            "Match 02:",
            "00: 0002-0002 ''",
            "Match 03:",
            "00: 0003-0003 ''",
            "Match 04:",
            "00: 0005-0005 ''",
        };
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    /// Single character matching are one-character matches.
    /// In the text `abc`, a dot pattern must produce three matches, one for each character.
    void testSingleCharMatches() {
        WITH_CONTEXT(requireCompile("(?s)."_el));
        WITH_CONTEXT(requireFindAll("abc"_el));
        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0001 'a'",
            "Match 02:",
            "00: 0001-0002 'b'",
            "Match 03:",
            "00: 0002-0003 'c'",
        };
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }
};
