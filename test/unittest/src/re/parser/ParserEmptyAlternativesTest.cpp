// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserEmptyAlternativesTest : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testEmptyAlternativesDisabled() {
        Settings settings;
        REQUIRE_FALSE(settings.hasFeature(Feature::EmptyAlternatives));

        const auto expectedError =
            "Empty alternatives are not allowed. Use '?' for optionality instead (e.g., '(?:a|b)?')";

        WITH_CONTEXT(parseAndExpectError("|a"_el, expectedError, 0U, settings));
        WITH_CONTEXT(parseAndExpectError("a|"_el, expectedError, 2U, settings));
        WITH_CONTEXT(parseAndExpectError("a||b"_el, expectedError, 2U, settings));
        WITH_CONTEXT(parseAndExpectError("(a|)"_el, expectedError, 3U, settings));
        WITH_CONTEXT(parseAndExpectError("(|a)"_el, expectedError, 1U, settings));
        WITH_CONTEXT(parseAndExpectError("(?:a||b)"_el, expectedError, 5U, settings));
    }

    void testEmptyAlternativesEnabled() {
        Settings settings;
        settings.enableFeature(Feature::EmptyAlternatives);

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

        parseAndTest(
            "(a|)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=2,index=1)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
                "      Sequence(size=0)",
            },
            {},
            settings);

        parseAndTest(
            "(|a)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=2,index=1)",
                "      Sequence(size=0)",
                "      Sequence(size=1)",
                "        CharacterSequence(\"a\")",
            },
            {},
            settings);
    }

    void testEmptyGroupsDisabled() {
        Settings settings;
        REQUIRE_FALSE(settings.hasFeature(Feature::EmptyGroups));

        WITH_CONTEXT(parseAndExpectError("()"_el, "Empty groups are not allowed", 1U, settings));
        WITH_CONTEXT(parseAndExpectError("(?:)"_el, "Empty groups are not allowed", 3U, settings));
        WITH_CONTEXT(parseAndExpectError(""_el, "Empty groups are not allowed", 0U, settings));
        WITH_CONTEXT(parseAndExpectError("a()b"_el, "Empty groups are not allowed", 2U, settings));
        WITH_CONTEXT(parseAndExpectError("(a())"_el, "Empty groups are not allowed", 3U, settings));
    }

    void testEmptyGroupsEnabled() {
        Settings settings;
        settings.enableFeature(Feature::EmptyGroups);

        parseAndTest(
            "()"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1,index=1)",
                "      Sequence(size=0)",
            },
            {},
            settings);

        parseAndTest(
            "(?:)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=1)",
                "      Sequence(size=0)",
            },
            {},
            settings);

        parseAndTest(
            ""_el,
            {
                "Group(size=1)",
                "  Sequence(size=0)",
            },
            {},
            settings);

        parseAndTest(
            "a()b"_el,
            {
                "Group(size=1)",
                "  Sequence(size=3)",
                "    CharacterSequence(\"a\")",
                "    Group(size=1,index=1)",
                "      Sequence(size=0)",
                "    CharacterSequence(\"b\")",
            },
            {},
            settings);
    }

    void testCombination() {
        Settings settings;
        settings.enableFeature(Feature::EmptyAlternatives);
        settings.enableFeature(Feature::EmptyGroups);

        parseAndTest(
            "(|)"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Group(size=2,index=1)",
                "      Sequence(size=0)",
                "      Sequence(size=0)",
            },
            {},
            settings);
    }
};
