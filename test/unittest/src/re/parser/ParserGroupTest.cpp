// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserGroupTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testAtomicAndNonCapturingGroups() {
        // Atomic group (?>...)
        parseAndTest(
            "(?>abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,atomicGroupId=0,atomic)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });

        // Non‑capturing group (?:...)
        parseAndTest(
            "(?:abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });
    }

    void testCapturingGroupsSingleMultipleNested() {
        // Single capturing group
        parseAndTest(
            "(abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });

        // Multiple sequential capturing groups
        parseAndTest(
            "(a)(b)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    Group(size=1,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
                "    Group(size=1,index=2)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"b\")",
            });

        // Nested capturing groups
        parseAndTest(
            "((xy)z)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1)",
                "      Sequence(size=2)",
                "        Group(size=1,index=2)",
                "          Sequence(size=1)",
                "            CharacterSequence(\"xy\")",
                "        CharacterSequence(\"z\")",
            });
    }

    void testNamedGroupsAllForms() {
        // Perl/Python compatible variants
        parseAndTest(
            "(?<name>abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1,name=\"name\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });

        parseAndTest(
            "(?'name'abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1,name=\"name\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });

        parseAndTest(
            "(?P<name>abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1,name=\"name\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });
    }

    void testErrors_NamedGroups() {
        // Missing closing '>' for (?<name>
        parser = Parser{"(?<name)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Missing closing quote for (?'name
        parser = Parser{"(?'name)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Unexpected end in group name
        parser = Parser{"(?<name"_el};
        REQUIRE_THROWS(node = parser.parse());

        // (?P=...) backreference not supported
        parser = Parser{"(?P=name)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // (?P...) without '<' for a named group
        parser = Parser{"(?P'name')"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Group name starting with a digit
        parser = Parser{"(?<1name>abc)"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testErrors_TooManyCaptureGroups() {
        StringEditor pattern;
        for (std::size_t i = 0; i < el::re::impl::limits::maximumCaptureGroupCount; ++i) {
            pattern.append("(abc)"_el);
        }
        parser = Parser{pattern};
        REQUIRE_NOTHROW(node = parser.parse());
        pattern.append("(def)"_el); // this one exceeds the maximum.
        parser = Parser{pattern};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testNamedGroups_NestedAndMultiple() {
        // Nested named groups
        parseAndTest(
            "(?<outer>a(?<inner>y)c)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1,name=\"outer\")",
                "      Sequence(size=3)",
                "        CharacterSequence(\"a\")",
                "        Group(size=1,index=2,name=\"inner\")",
                "          Sequence(size=1)",
                "            CharacterSequence(\"y\")",
                "        CharacterSequence(\"c\")",
            });

        // Multiple named groups sequentially
        parseAndTest(
            "(?<a>x)(?<b>y)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    Group(size=1,index=1,name=\"a\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"x\")",
                "    Group(size=1,index=2,name=\"b\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"y\")",
            });
    }

    void testNamedGroup_DuplicateNameErrors() {
        // Duplicate names should error
        parser = Parser{"(?<a>x)(?<a>y)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Case-insensitive duplicate names should error
        parser = Parser{"(?<a>x)(?<A>y)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Duplicate across nested and following
        parser = Parser{"((?<a>x))(?P<a>y)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Triplet with duplicate at the end
        parser = Parser{"(?<a>x)(?<b>y)(?'a'z)"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testComment_Unterminated() {
        // Unterminated inline comment should error with unexpected end
        parser = Parser{"(?#this is a comment"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testGroupCloseAndUnclosedErrors() {
        // Unexpected close
        parser = Parser{"abc)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Unclosed single group
        parser = Parser{"("_el};
        REQUIRE_THROWS(node = parser.parse());

        // Too many closing parentheses
        parser = Parser{"(a))"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Nested unclosed group at end
        parser = Parser{"((a)"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void prepareParserWithNestedGroups(const std::size_t opens, Settings settings = {}) {
        StringEditor s;
        s.reserve(el::unit::ByteLength{(opens * 4U) + 1U});
        for (std::size_t i = 0; i < opens; ++i) {
            s.append("(?:"_el);
        }
        s.append("x"_el);
        for (std::size_t i = 0; i < opens; ++i) {
            s.append(")"_el);
        }
        lastPattern = s;
        parser = Parser{lastPattern, {}, settings};
    }

    void testGroupNestingLimit_DefaultAndCustom() {
        // With default settings: nesting exactly at the limit should parse,
        // nesting exceeding the limit should throw.
        {
            constexpr auto limit = el::re::impl::limits::maximumGroupNestingDepth;

            // Exactly at the limit: should NOT throw
            prepareParserWithNestedGroups(limit);
            REQUIRE_NOTHROW(node = parser.parse());

            // One more than the limit: should throw
            prepareParserWithNestedGroups(limit + 1);
            REQUIRE_THROWS(node = parser.parse());
        }

        // With a custom (lower) limit via Settings: ensure enforcement
        {
            Settings settings;
            settings.setMaximumGroupNestingDepth(5);

            // Up to 5 opens is fine
            prepareParserWithNestedGroups(5, settings);
            REQUIRE_NOTHROW(node = parser.parse());

            // 6 opens exceeds the custom limit and must throw
            prepareParserWithNestedGroups(6, settings);
            REQUIRE_THROWS(node = parser.parse());
        }
    }
};
