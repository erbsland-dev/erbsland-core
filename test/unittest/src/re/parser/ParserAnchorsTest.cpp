// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserAnchorsTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testCaretAndDollar_SingleLine() {
        // In single-line mode, ^ => Start and $ => End
        parseAndTest(
            "^a$"_el,
            {
                "Group(size=1)",
                "  Sequence(size=3)",
                "    Anchor(Start)",
                "    CharacterSequence(\"a\")",
                "    Anchor(End)",
            });
    }

    void testCaretAndDollar_Multiline() {
        // In multi-line mode, ^ => LineStart and $ => LineEnd
        parser = Parser{"^a$"_el, GroupFlags{GroupFlag::Multiline}};
        REQUIRE_NOTHROW(node = parser.parse());
        const auto actual = node->toTestTree();
        const std::vector<std::string_view> expected{
            "Group(size=1,flags=m)",
            "  Sequence(size=3)",
            "    Anchor(LineStart)",
            "    CharacterSequence(\"a\")",
            "    Anchor(LineEnd)",
        };
        WITH_CONTEXT(requireLines(actual, expected));
    }

    void testEscapeAnchors_A_Z_z() {
        // \A => Start, \Z and legacy \z => End
        parseAndTest(
            "\\A\\Z\\z"_el,
            {
                "Group(size=1)",
                "  Sequence(size=3)",
                "    Anchor(Start)",
                "    Anchor(End)",
                "    Anchor(End)",
            });
    }

    void testWordBoundaries_UnicodeDefault() {
        // Default is Unicode mode
        parseAndTest(
            "\\bX\\B"_el,
            {
                "Group(size=1)",
                "  Sequence(size=3)",
                "    Anchor(UnicodeWordBoundary)",
                "    CharacterSequence(\"X\")",
                "    Anchor(NonUnicodeWordBoundary)",
            });
    }

    void testWordBoundaries_AsciiFlag() {
        parser = Parser{"\\bX\\B"_el, GroupFlags{GroupFlag::Ascii}};
        REQUIRE_NOTHROW(node = parser.parse());
        const auto actual = node->toTestTree();
        const std::vector<std::string_view> expected{
            "Group(size=1,flags=a)",
            "  Sequence(size=3)",
            "    Anchor(AsciiWordBoundary)",
            "    CharacterSequence(\"X\")",
            "    Anchor(NonAsciiWordBoundary)",
        };
        WITH_CONTEXT(requireLines(actual, expected));
    }
};
