// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserFlagsAndVerboseTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testStartFlagsAtPatternStart() {
        // (?i) at the start sets the root flags and does not create a new group
        parseAndTest(
            "(?i)abc"_el,
            {
                "Group(size=1,flags=i)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"abc\")",
            });
    }

    void testStartFlagsNotAtPatternStartError() {
        // Using (?i) not at the very start must fail
        parser = Parser{"a(?i)"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testGroupWithFlags() {
        // (?-i:...) should create a non-capturing group with flags applied to the group only
        parseAndTest(
            "(?-i:abc)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"abc\")",
            });
    }

    void testStartFlags_AllLetters() {
        // All supported letters at pattern start (order does not matter for behavior, but toString is imsax)
        parseAndTest(
            "(?imsax)ab"_el,
            {
                "Group(size=1,flags=imsax)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
            });

        // Unicode switch at start: (?u) should be accepted and clear ASCII mode (no visible flag)
        parseAndTest(
            "(?u)ab"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
            });

        // ASCII then switch back to Unicode: result is no ASCII flag
        parseAndTest(
            "(?a)(?u)ab"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
            });
    }

    void testLocalGroupFlags_AllLetters() {
        // Local flags on a non-capturing group
        parseAndTest(
            "(?imsax:ab)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,flags=imsax)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"ab\")",
            });
    }

    void testNestedFlagClearingAndSwitching() {
        // Clear -i in nested group
        parseAndTest(
            "(?ia:a(?-i:b)c)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,flags=ia)",
                "      Sequence(size=3)",
                "        CharacterSequence(\"a\")",
                "        Group(size=1,flags=a)",
                "          Sequence(size=1)",
                "            CharacterSequence(\"b\")",
                "        CharacterSequence(\"c\")",
            });

        // ASCII in outer, switch to Unicode in inner: inner has no visible flags
        parseAndTest(
            "(?a:(?u:xy))"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,flags=a)",
                "      Sequence(size=1)",
                "        Group(size=1)",
                "          Sequence(size=1)",
                "            CharacterSequence(\"xy\")",
            });
    }

    void testVerbose_InitialAndLocal() {
        // Initial verbose via start flags at pattern start: ignore whitespace and inline # comments
        parseAndTest(
            "(?x)  a  # comment\n  b  "_el,
            {
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
            });

        // Initial verbose via constructor flags
        parser = Parser{"  a  # c\n  b  "_el, GroupFlags{GroupFlag::Verbose}};
        REQUIRE_NOTHROW(node = parser.parse());
        {
            const auto actual = node->toTestTree();
            const std::vector<std::string_view> expected{
                "Group(size=1,flags=x)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
            };
            WITH_CONTEXT(requireLines(actual, expected));
        }

        // Local verbose on a non-capturing group should only apply inside that group
        parseAndTest(
            "(?x:  a  # c\n  b  )(c)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    Group(size=1,flags=x)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"ab\")",
                "    Group(size=1,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"c\")",
            });
    }

    void testErrors_GroupFlagsVariants() {
        // 1) Minus not allowed for start‑only flags (?i)
        parser = Parser{"(?-i)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 2) Repeated '-' inside group flags
        parser = Parser{"(?--i:abc)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 2b) Invalid '-a' and '-u' in group flags
        parser = Parser{"(?-a:abc)"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"(?-u:abc)"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 3) Too many group flags (exceed limits::maximumFlagCount = 10)
        // Use 12 'i' flags to ensure the check triggers inside the loop
        parser = Parser{"(?iiiiiiiiiiii)abc"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 4) Unexpected character after advanced group open sequence
        parser = Parser{"(?Q)abc"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 5) Expected ':' after group flags when used inside a group (not at start)
        parser = Parser{"a(?iq)abc"_el}; // 'q' is not a valid flag nor ':'
        REQUIRE_THROWS(node = parser.parse());

        // 6) At pattern start, flags must end with ')' or be followed by ':'
        parser = Parser{"(?iq)abc"_el}; // invalid tail at start
        REQUIRE_THROWS(node = parser.parse());
    }
};
