// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/StdFormat.hpp>

using impl::limits::maximumQuantifierCount;

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserQuantifierTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testSimpleQuantifiers_OnLiteral() {
        // ? 0 or 1
        WITH_CONTEXT(parseAndTest(
            "a?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));

        // ?+ possessive, ?? lazy
        WITH_CONTEXT(parseAndTest(
            "a?+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=possessive,atomicGroupId=0)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a??"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=lazy)",
                "      CharacterSequence(\"a\")",
            }));

        // * 0 or more
        WITH_CONTEXT(parseAndTest(
            "a*"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a*+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=possessive,atomicGroupId=0)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a*?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=lazy)",
                "      CharacterSequence(\"a\")",
            }));

        // + 1 or more
        WITH_CONTEXT(parseAndTest(
            "a+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a++"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=possessive,atomicGroupId=0)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a+?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=lazy)",
                "      CharacterSequence(\"a\")",
            }));
    }

    void testCountedQuantifiers_OnLiteral() {
        // {n}
        WITH_CONTEXT(parseAndTest(
            "a{3}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                // exact count => min=max=3, greedy
                "    Quantifier(min=3,max=3,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));

        // {n,m}
        WITH_CONTEXT(parseAndTest(
            "a{2,5}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=5,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));
        // possessive and lazy suffix
        WITH_CONTEXT(parseAndTest(
            "a{2,5}+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=5,mode=possessive,atomicGroupId=0)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a{2,5}?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=5,mode=lazy)",
                "      CharacterSequence(\"a\")",
            }));

        // {n,}
        WITH_CONTEXT(parseAndTest(
            "a{4,}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=4,max=*,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a{4,}+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=4,max=*,mode=possessive,atomicGroupId=0)",
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a{4,}?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=4,max=*,mode=lazy)",
                "      CharacterSequence(\"a\")",
            }));
    }

    void testCountedQuantifier_ZeroToM() {
        // {,m} => 0..m
        WITH_CONTEXT(parseAndTest(
            "a{,3}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=3,mode=greedy)",
                "      CharacterSequence(\"a\")",
            }));
    }

    void testExactOneOptimization() {
        // {1} and {1,1} must not create a quantifier node (optimization)
        WITH_CONTEXT(parseAndTest(
            "a{1}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a{1,1}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a\")",
            }));
    }

    void testSimpleQuantifiers_OnDot() {
        // Default dot is any char except LineBreak => CharacterCategory(^LineBreak)
        WITH_CONTEXT(parseAndTest(
            ".?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=greedy)",
                "      CharacterCategory(Any)",
            }));
        WITH_CONTEXT(parseAndTest(
            ".*?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=lazy)",
                "      CharacterCategory(Any)",
            }));
        WITH_CONTEXT(parseAndTest(
            ".++"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=possessive,atomicGroupId=0)",
                "      CharacterCategory(Any)",
            }));
        // Counted forms on dot
        WITH_CONTEXT(parseAndTest(
            ".{2,4}+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=4,mode=possessive,atomicGroupId=0)",
                "      CharacterCategory(Any)",
            }));
        WITH_CONTEXT(parseAndTest(
            ".{3,}?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=3,max=*,mode=lazy)",
                "      CharacterCategory(Any)",
            }));
    }

    void testDotAllFlag_Variant() {
        // With DotAll flag, '.' becomes Category(DotAll)
        WITH_CONTEXT(parseAndTest(
            "."_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(Any)",
            }));

        WITH_CONTEXT(parseAndTest(
            "."_el,
            {
                "Group(size=1,flags=s)",
                "  Sequence(size=1)",
                "    CharacterCategory(AnyDotAll)",
            },
            GroupFlags{GroupFlag::DotAll}));

        WITH_CONTEXT(parseAndTest(
            ".+?"_el,
            {
                "Group(size=1,flags=s)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=lazy)",
                "      CharacterCategory(AnyDotAll)",
            },
            GroupFlags{GroupFlag::DotAll}));
    }

    void testLimits_DefaultBoundary() {
        // boundary success at maximumQuantifierCount
        const auto max = static_cast<unsigned>(maximumQuantifierCount);
        WITH_CONTEXT(parseAndTest(
            "a{10000}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                std::format("    Quantifier(min={},max={},mode=greedy)", max, max),
                "      CharacterSequence(\"a\")",
            }));
        WITH_CONTEXT(parseAndTest(
            "a{1,10000}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                std::format("    Quantifier(min=1,max={},mode=greedy)", max),
                "      CharacterSequence(\"a\")",
            }));

        // exceeding should fail
        parser = Parser{"a{10001}"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"a{1,10001}"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testLimits_CustomLowerMaximum() {
        Settings settings;
        settings.setMaximumQuantifierCount(3);

        // Allowed: at boundary
        WITH_CONTEXT(parseAndTest(
            "a{3}"_el,
            {"Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=3,max=3,mode=greedy)",
                "      CharacterSequence(\"a\")"},
            {},
            settings));

        // Exceed minimum
        parser = Parser{"a{4}"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
        // Exceed maximum (finite)
        parser = Parser{"a{1,4}"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
        // Exceed minimum with open upper bound
        parser = Parser{"a{4,}"_el, {}, settings};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testErrors_InQuantifierParsing() {
        // Too many digits in {n}
        parser = Parser{"a{123456}"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Too many digits in {n,m} upper part
        parser = Parser{"a{1,123456}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // max < min
        parser = Parser{"a{5,4}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Missing '}' after parsing max
        parser = Parser{"a{1,2x"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Non-digit and not '}' after comma in {n,?}
        parser = Parser{"a{1,x}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Unexpected token after {n ...}
        parser = Parser{"a{123x}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // {,} => missing digit after comma
        parser = Parser{"a{,}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // {,mX} => missing closing brace after parsing max
        parser = Parser{"a{,2x"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Start with '{' then end: missing content after '{'
        parser = Parser{"a{"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Invalid char after '{'
        parser = Parser{"a{a}"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testErrors_UnexpectedQuantifierTokens() {
        // Unexpected at pattern start or without a preceding token
        parser = Parser{"*"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"+"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"?"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"{"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"}"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"a}"_el};
        REQUIRE_THROWS(node = parser.parse());
    }
};
