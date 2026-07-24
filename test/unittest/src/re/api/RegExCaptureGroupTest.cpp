// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

#include <erbsland/re/StdFormat.hpp>

#include <memory>

using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Captures)
class RegExCaptureGroupTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    TESTED_TARGETS(match)
    void testMatch() {
        WITH_CONTEXT(requireCompile("(a)(b(c))"_el));

        WITH_CONTEXT(requireMatch("abc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0003 'abc'",
            "01: 0000-0001 'a'",
            "02: 0001-0003 'bc'",
            "03: 0002-0003 'c'",
        }));

        WITH_CONTEXT(requireMatch("abcd"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0003 'abc'",
            "01: 0000-0001 'a'",
            "02: 0001-0003 'bc'",
            "03: 0002-0003 'c'",
        }));
    }

    TESTED_TARGETS(fullMatch)
    void testFullMatch() {
        WITH_CONTEXT(requireCompile("(a)(b(c))"_el));

        WITH_CONTEXT(requireFullMatch("abc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0003 'abc'",
            "01: 0000-0001 'a'",
            "02: 0001-0003 'bc'",
            "03: 0002-0003 'c'",
        }));

        WITH_CONTEXT(requireNoFullMatch("abcd"_el));
    }

    TESTED_TARGETS(findFirst)
    void testFindFirst() {
        WITH_CONTEXT(requireCompile("(a)(b(c))"_el));

        WITH_CONTEXT(requireFindFirst("...abc..."_el));
        WITH_CONTEXT(requireGroups({
            "00: 0003-0006 'abc'",
            "01: 0003-0004 'a'",
            "02: 0004-0006 'bc'",
            "03: 0005-0006 'c'",
        }));
    }

    TESTED_TARGETS(findAll)
    void testFindAll() {
        WITH_CONTEXT(requireCompile("(a)b"_el));

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0002 'ab'",
            "01: 0000-0001 'a'",
            "Match 02:",
            "00: 0003-0005 'ab'",
            "01: 0003-0004 'a'",
        };

        WITH_CONTEXT(requireFindAll("ab ab"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testCollectAll() {
        WITH_CONTEXT(requireCompile("(a)b"_el));

        const auto expectedLines = std::vector<std::string>{
            "Match 01:",
            "00: 0000-0002 'ab'",
            "01: 0000-0001 'a'",
            "Match 02:",
            "00: 0003-0005 'ab'",
            "01: 0003-0004 'a'",
        };

        WITH_CONTEXT(requireCollectAll("ab ab"_el));
        WITH_CONTEXT(requireLines(matchLines, expectedLines));
    }

    void testReplace() {
        WITH_CONTEXT(requireCompile("(a)(b(c))"_el));

        const auto testCases = ReplaceTestCases{
            {"abc"_el, "{1}-{2}-{3}"_el, "a-bc-c"_el},
            {"...abc..."_el, "[{0}]"_el, "...[abc]..."_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }

    void testOptionalGroup() {
        WITH_CONTEXT(requireCompile("a(b)?c"_el));

        WITH_CONTEXT(requireMatch("abc"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0003 'abc'",
            "01: 0001-0002 'b'",
        }));

        WITH_CONTEXT(requireMatch("ac"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 'ac'",
            "01: 0000-0000 ''", // non-participating group
        }));
    }

    void testRepeatedGroup() {
        WITH_CONTEXT(requireCompile("(a)*b"_el));

        WITH_CONTEXT(requireMatch("aaab"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0004 'aaab'",
            "01: 0002-0003 'a'", // last capture
        }));

        WITH_CONTEXT(requireMatch("b"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0001 'b'",
            "01: 0000-0000 ''",
        }));
    }

    void testNestedRepeatedGroup() {
        WITH_CONTEXT(requireCompile("(a(b))*"_el));

        WITH_CONTEXT(requireMatch("abab"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0004 'abab'",
            "01: 0002-0004 'ab'",
            "02: 0003-0004 'b'",
        }));
    }

    void testNamedGroup() {
        WITH_CONTEXT(requireCompile("(?<first>a)(?<second>b)"_el));

        WITH_CONTEXT(requireMatch("ab"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 'ab'",
            "01: 0000-0001 'a'",
            "02: 0001-0002 'b'",
        }));

        const auto testCases = ReplaceTestCases{
            {"ab"_el, "{first}-{second}"_el, "a-b"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }

    void testNamedGroupNamesOutliveRegEx() {
        auto expression = RegEx::compile("(?<letter>a)"_el);
        const auto match = expression->fullMatch("a"_el);
        const auto weakExpression = std::weak_ptr<RegEx>{expression};

        REQUIRE(match != nullptr);
        expression.reset();

        REQUIRE(weakExpression.expired());
        REQUIRE(match->hasGroupName("LETTER"_el));
        REQUIRE_EQUAL(match->group("letter"_el).name(), "letter"_el);
        REQUIRE_EQUAL(match->content("letter"_el), "a"_el);
    }

    void testCopiedRegExCanCreateNamedMatch() {
        const auto expression = RegEx::compile("(?<letter>a)"_el);
        auto copiedExpression = *expression;

        const auto match = copiedExpression.fullMatch("a"_el);

        REQUIRE(match != nullptr);
        REQUIRE_EQUAL(match->content("letter"_el), "a"_el);
    }

    void testNamedGroupAltSyntax() {
        // Test alternative syntax for named groups
        WITH_CONTEXT(requireCompile("(?'first'a)(?P<second>b)"_el));

        WITH_CONTEXT(requireMatch("ab"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0002 'ab'",
            "01: 0000-0001 'a'",
            "02: 0001-0002 'b'",
        }));

        const auto testCases = ReplaceTestCases{
            {"ab"_el, "{first}-{second}"_el, "a-b"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }

    void testComplexNesting() {
        WITH_CONTEXT(requireCompile("((a)(b))((c)(d))"_el));

        WITH_CONTEXT(requireMatch("abcd"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0004 'abcd'",
            "01: 0000-0002 'ab'",
            "02: 0000-0001 'a'",
            "03: 0001-0002 'b'",
            "04: 0002-0004 'cd'",
            "05: 0002-0003 'c'",
            "06: 0003-0004 'd'",
        }));
    }

    void testDeepNesting() {
        WITH_CONTEXT(requireCompile("(a(b(c(d))))"_el));

        WITH_CONTEXT(requireMatch("abcd"_el));
        WITH_CONTEXT(requireGroups({
            "00: 0000-0004 'abcd'",
            "01: 0000-0004 'abcd'",
            "02: 0001-0004 'bcd'",
            "03: 0002-0004 'cd'",
            "04: 0003-0004 'd'",
        }));
    }
};
