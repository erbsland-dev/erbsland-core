// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserAlternativesTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testTopLevelAlternatives() {
        // Simple two-branch alternative at root
        parseAndTest(
            "a|b"_el,
            {
                "Group(size=2)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a\")",
                "  Sequence(size=1)",
                "    CharacterSequence(\"b\")",
            });

        // Three branches with varying lengths
        parseAndTest(
            "ab|cd|e"_el,
            {
                "Group(size=3)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"ab\")",
                "  Sequence(size=1)",
                "    CharacterSequence(\"cd\")",
                "  Sequence(size=1)",
                "    CharacterSequence(\"e\")",
            });
    }

    void testGroupAlternatives() {
        // Capturing group with alternatives, followed by trailing literal
        parseAndTest(
            "(a|b)c"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    Group(size=2,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"b\")",
                "    CharacterSequence(\"c\")",
            });

        // Non‑capturing group variant
        parseAndTest(
            "(?:x|y)z"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    Group(size=2)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"x\")",
                "      Sequence(size=1)",
                "        CharacterSequence(\"y\")",
                "    CharacterSequence(\"z\")",
            });
    }

    void testNestedAlternatives() {
        // Nested capturing groups with inner alternatives
        parseAndTest(
            "(a|(b|c))d"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    Group(size=2,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
                "      Sequence(size=1)",
                "        Group(size=2,index=2)",
                "          Sequence(size=1)",
                "            CharacterSequence(\"b\")",
                "          Sequence(size=1)",
                "            CharacterSequence(\"c\")",
                "    CharacterSequence(\"d\")",
            });
    }

    void testEmptyAlternatives() {
        Settings settings;
        settings.enableFeature(Feature::EmptyAlternatives);

        // Leading empty branch
        parseAndTest(
            "|a"_el,
            {
                "Group(size=2)",
                "  Sequence(size=0)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a\")",
            },
            {},
            settings);

        // Trailing empty branch
        parseAndTest(
            "a|"_el,
            {
                "Group(size=2)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a\")",
                "  Sequence(size=0)",
            },
            {},
            settings);

        // Empty middle branch
        parseAndTest(
            "a||b"_el,
            {
                "Group(size=3)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"a\")",
                "  Sequence(size=0)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"b\")",
            },
            {},
            settings);
    }

    // Note: limit handling for alternatives is covered elsewhere.
};
