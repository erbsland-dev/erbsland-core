// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserFeaturesTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testQuotedLiterals_Disabled() {
        // When QuotedLiterals is disabled, \Q...\E must be rejected
        Settings s;
        s.disableFeature(Feature::QuotedLiterals);
        parser = Parser{"\\Qx\\E"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\Qx\\E]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());

        // Sanity: enabled by default should parse
        parseAndTest(
            "\\Qx\\E"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"x\")",
            });
    }

    void testEscapeBell_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeBell);
        parser = Parser{"\\a"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\a]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\a"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\a]"_el}.parse());
    }

    void testEscapeControl_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeControl);
        parser = Parser{"\\cA"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\cA]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\cZ"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\cZ]"_el}.parse());
    }

    void testEscapeEscape_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeEscape);
        parser = Parser{"\\e"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\e]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\e"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\e]"_el}.parse());
    }

    void testEscapeFormFeed_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeFormFeed);
        parser = Parser{"\\f"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\f]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\f"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\f]"_el}.parse());
    }

    void testEscapeOctal_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeOctal);
        parser = Parser{"\\o{141}"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\o{141}]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\o{141}"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\o{141}]"_el}.parse());
    }

    void testEscapeHex_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeHex);
        // short form
        parser = Parser{"\\x41"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\x41]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // long form
        parser = Parser{"\\x{41}"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\x{41}]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\x41"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\x41]"_el}.parse());
        REQUIRE_NOTHROW(Parser{"\\x{41}"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\x{41}]"_el}.parse());
    }

    void testEscapeHorizontalSpace_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeHorizontalSpace);
        parser = Parser{"\\h"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\h]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\H"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\h"_el}.parse());
        REQUIRE_NOTHROW(Parser{"\\H"_el}.parse());
    }

    void testEscapeVerticalSpace_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeVerticalSpace);
        parser = Parser{"\\v"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"[\\v]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\V"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\v"_el}.parse());
        REQUIRE_NOTHROW(Parser{"\\V"_el}.parse());
    }

    void testPosixClasses_Disabled() {
        // POSIX character class only valid inside [...] per implementation
        Settings s;
        s.disableFeature(Feature::PosixClasses);
        parser = Parser{"[[:digit:]]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"[[:digit:]]"_el}.parse());
    }

    void testAnchorLowercaseZ_Disabled() {
        Settings s;
        s.disableFeature(Feature::AnchorLowercaseZ);
        parser = Parser{"\\z"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity
        REQUIRE_NOTHROW(Parser{"\\z"_el}.parse());
    }

    void testEscapeLongUnicode_Disabled() {
        Settings s;
        s.disableFeature(Feature::EscapeLongUnicode);
        // plain long unicode escape \UHHHHHHHH
        parser = Parser{"\\U00000041"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // inside character class
        parser = Parser{"[\\U00000041]"_el, {}, s};
        REQUIRE_THROWS(node = parser.parse());
        // Sanity: enabled by default should parse
        REQUIRE_NOTHROW(Parser{"\\U00000041"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\U00000041]"_el}.parse());
        REQUIRE_NOTHROW(Parser{"\\U{41}"_el}.parse());
        REQUIRE_NOTHROW(Parser{"[\\U{41}]"_el}.parse());
    }
};
