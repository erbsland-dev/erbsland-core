// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

using namespace el::re;

/// Test if basic nesting of atomic groups is working.
/// No pruning should be performed.
TESTED_TARGETS(RegEx)
TAGS(Api AtomicGroup)
class RegExAtomicGroupMinimalNestedTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void compilePattern() {
        WITH_CONTEXT(requireCompile("_(?>a(?>b(?>c(?>d(?>e(?>f(?>g(?>h(?>i(?>j)1)2)3)4)5)6)7)8)9)_"_el));
    }

    TESTED_TARGETS(match)
    void testMatch() {
        compilePattern();
        const auto matchCases = NoneCapGroupTestCases{
            {"_abcdefghij123456789_"_el},
            {"_abcdefghij123456789_xyz"_el, 0, "_abcdefghij123456789_"_el},
        };
        WITH_CONTEXT(requireMatchWithNoneCapGroups(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "_"_el,
            "_a"_el,
            "_ab"_el,
            "_abc"_el,
            "_abcd"_el,
            "_abcde"_el,
            "_abcdef"_el,
            "_abcdefg"_el,
            "_abcdefgh"_el,
            "_abcdefghi"_el,
            "_abcdefghij"_el,
            "_abcdefghij1"_el,
            "_abcdefghij12"_el,
            "_abcdefghij123"_el,
            "_abcdefghij1234"_el,
            "_abcdefghij12345"_el,
            "_abcdefghij123456"_el,
            "_abcdefghij1234567"_el,
            "_abcdefghij12345689"_el,
            "_!bcdefghij12345689_"_el,
            "_a!cdefghij12345689_"_el,
            "_ab!defghij12345689_"_el,
            "_abc!efghij12345689_"_el,
            "_abcd!fghij12345689_"_el,
            "_abcde!ghij12345689_"_el,
            "_abcdef!hij12345689_"_el,
            "_abcdefg!ij12345689_"_el,
            "_abcdefgh!j12345689_"_el,
            "_abcdefghi!12345689_"_el,
            "_abcdefghij!2345689_"_el,
            "_abcdefghij1!345689_"_el,
        };
        WITH_CONTEXT(requireNoMatch(noMatchCases));
    }
    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        compilePattern();
        const auto matchCases = std::vector<StringView>{
            {"_abcdefghij123456789_"_el},
        };
        WITH_CONTEXT(requireFullMatchWithNoCaptures(matchCases));
    }
    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        compilePattern();
        const auto matchCases = NoneCapGroupTestCases{
            {"_abcdefghij123456789_"_el, 0, "_abcdefghij123456789_"_el},
            {"______________________abcdefghij123456789______abcdefghij123456789_"_el, 21, "_abcdefghij123456789_"_el},
        };
        WITH_CONTEXT(requireFindFirstNoCaptures(matchCases));

        const auto noMatchCases = std::vector<StringView>{
            ""_el,
            "__ab_abc_abcd_abcdef_abcdefghij123456789"_el,
        };
        WITH_CONTEXT(requireNoFindFirst(noMatchCases));
    }
    TESTED_TARGETS(findAll)
    void testFindAll() {
        compilePattern();
        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0021-0042 '_abcdefghij123456789_'",
            "Match 02:",
            "00: 0046-0067 '_abcdefghij123456789_'",
        };
        WITH_CONTEXT(requireFindAll("______________________abcdefghij123456789______abcdefghij123456789_"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
        WITH_CONTEXT(requireNoFindAll("______________________abcdefghi!123456789______abcdefghi!123456789_"_el));
    }
};
