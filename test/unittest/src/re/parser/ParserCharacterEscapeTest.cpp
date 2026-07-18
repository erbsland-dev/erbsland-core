// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserCharacterEscapeTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    struct EscapeTestCase {
        String pattern;
        std::string_view expected;
    };
    using EscapeTestCases = std::vector<EscapeTestCase>;

    void testRawNullCharacter() {
        const auto rawNullPattern = String::fromCharacter(el::text::Char{U'\0'});
        Settings settings;
        settings.enableFeature(Feature::AcceptNullInPattern);
        WITH_CONTEXT(parseAndTest(
            String{rawNullPattern},
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"\\u0000\")",
            },
            GroupFlags{},
            settings));
    }

    void testEmbeddedSequence() {
        parseAndTest(
            "abc\\.xyz"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"abc.xyz\")",
            });
    }

    void requireUnchanged(
        const String &pattern,
        const std::string_view expected,
        GroupFlags groupFlags = GroupFlags{},
        const Settings &settings = {}) {
        auto enclosedPattern = StringEditor{"abc"_el};
        enclosedPattern.append(pattern);
        enclosedPattern.append("xyz"_el);
        parser = Parser{enclosedPattern, groupFlags, settings};
        REQUIRE_NOTHROW(node = parser.parse());
        const auto actual = node->toTestTree();
        const auto expectedEntry = std::format("    CharacterSequence(\"abc{}xyz\")", expected);
        const auto expectedTree = std::vector<std::string_view>{{
            "Group(size=1)",
            "  Sequence(size=1)",
            std::string_view{expectedEntry},
        }};
        WITH_CONTEXT(requireLines(actual, expectedTree))
    }

    void testEscapeSequences() {
        const auto testCases = EscapeTestCases{
            {"\\."_el, "."},
            {"\\\\"_el, "\\\\"}, // expected: will be escaped
            {"\\^"_el, "^"},
            {"\\$"_el, "$"},
            {"\\|"_el, "|"},
            {"\\{"_el, "{"},
            {"\\}"_el, "}"},
            {"\\("_el, "("},
            {"\\)"_el, ")"},
            {"\\["_el, "["},
            {"\\]"_el, "]"},
            {"\\+"_el, "+"},
            {"\\*"_el, "*"},
            {"\\?"_el, "?"},
            {"\\ "_el, " "},
            {"\\\""_el, "\\\""}, // expected: will be escaped
            {"\\'"_el, "'"},
            {"\\#"_el, "#"},
        };
        EscapeTestCase lastTestCase;
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                for (const auto &testCase : testCases) {
                    lastTestCase = testCase;
                    requireUnchanged(testCase.pattern, testCase.expected);
                }
            },
            [&]() -> std::string {
                return std::format(
                    "Failed at pattern \"{}\"", lastTestCase.pattern.toSafeString(el::unit::CpLength{200U}));
            });
    }

    void testSpecialCharEscapes() {
        // Each special escape should translate to its control character, which is rendered as \u00XX
        WITH_CONTEXT(requireUnchanged("\\a"_el, "\\u0007"));
        WITH_CONTEXT(requireUnchanged("\\f"_el, "\\u000C"));
        WITH_CONTEXT(requireUnchanged("\\n"_el, "\\u000A"));
        WITH_CONTEXT(requireUnchanged("\\r"_el, "\\u000D"));
        WITH_CONTEXT(requireUnchanged("\\t"_el, "\\u0009"));
    }

    void testNumericHexEscapes() {
        // \xHH two-digit hex
        Settings allowZeroSettings;
        allowZeroSettings.enableFeature(Feature::AcceptNullInPattern);
        WITH_CONTEXT(requireUnchanged("\\x00"_el, "\\u0000", {}, allowZeroSettings));
        WITH_CONTEXT(requireUnchanged("\\x41"_el, "A"));
        WITH_CONTEXT(requireUnchanged("\\xFF"_el, "ÿ"));
        // \x{...} variable length
        WITH_CONTEXT(requireUnchanged("\\x{100}"_el, "Ā"));
        // Errors
        parser = Parser{"\\x00"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\xG1"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\x{}"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\x{123456789}"_el};
        REQUIRE_THROWS(node = parser.parse());
        // note: fixed-digit overflow is covered in UnicodeEscapes using \U with too many digits
    }

    void testNumericOctalEscapes() {
        // \o{...} octal
        WITH_CONTEXT(requireUnchanged("\\o{141}"_el, "a")); // 0141 = 'a'
        // Errors
        parser = Parser{"\\o"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\o{"_el};
        REQUIRE_THROWS(node = parser.parse());
        Settings allowZeroSettings;
        allowZeroSettings.enableFeature(Feature::AcceptNullInPattern);
        WITH_CONTEXT(requireUnchanged("\\o{00000000000}"_el, "\\u0000", {}, allowZeroSettings));
        parser = Parser{"\\o{999}"_el}; // invalid octal digits
        REQUIRE_THROWS(node = parser.parse());
        // Too many digits for variable-length octal (exceeds internal safety limit)
        parser = Parser{"\\o{0000000000000}"_el, {}, allowZeroSettings}; // 13 digits -> triggers "Too many digits"
        REQUIRE_THROWS(node = parser.parse());
    }

    void testUnicodeEscapes() {
        // \uHHHH (4 hex)
        Settings allowZeroSettings;
        allowZeroSettings.enableFeature(Feature::AcceptNullInPattern);
        WITH_CONTEXT(requireUnchanged("\\u0000"_el, "\\u0000", {}, allowZeroSettings));
        WITH_CONTEXT(requireUnchanged("\\u0041"_el, "A"));
        // \u{...} variable length
        WITH_CONTEXT(requireUnchanged("\\u{1F600}"_el, "😀")); // grinning face
        // \UHHHHHHHH (8 hex)
        WITH_CONTEXT(requireUnchanged("\\U0001F642"_el, "🙂")); // slightly smiling face
        // Note: we do not assert the exact rendering for U+10FFFF here to avoid source encoding issues.
        // Out of range and invalid sequences
        parser = Parser{"\\u0000"_el};   // zero is not allowed by default
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\u{0}"_el};    // zero is not allowed by default
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\U110000"_el}; // > 0x10FFFF
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\uD800"_el};   // surrogate range
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\u{D800}"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Too many fixed digits for \U (expects exactly 8 hex digits) -> provide 10 to trigger guard before 10th
        parser = Parser{"\\U123456789A"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Too many digits in \u{...}
        parser = Parser{"\\u{123456789}"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Too few digits in fixed-length \uHHHH
        parser = Parser{"\\u123"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Missing closing '}'
        parser = Parser{"\\u{00abc"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testBackreferencesAndUnknownAndDangling() {
        // Backreferences are not supported
        parser = Parser{"\\0"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\1"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Unknown escape
        parser = Parser{"\\q"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Dangling backslash
        parser = Parser{"\\"_el};
        REQUIRE_THROWS(node = parser.parse());
        // PCRE control: unexpected end and non-letter
        parser = Parser{"\\c"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"\\c*"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testCharacterClassesAndBoundariesAndAnchors() {
        // Default (Unicode) mode
        WITH_CONTEXT(parseAndTest(
            "\\d\\D\\s\\S\\w\\W\\b\\B\\A\\Z\\z"_el,
            {
                "Group(size=1)",
                "  Sequence(size=11)",
                "    CharacterCategory(DigitUnicode)",
                "    CharacterCategory(^DigitUnicode)",
                "    CharacterCategory(SpaceUnicode)",
                "    CharacterCategory(^SpaceUnicode)",
                "    CharacterCategory(WordUnicode)",
                "    CharacterCategory(^WordUnicode)",
                "    Anchor(UnicodeWordBoundary)",
                "    Anchor(NonUnicodeWordBoundary)",
                "    Anchor(Start)",
                "    Anchor(End)",
                "    Anchor(End)",
            }));

        // ASCII mode for \d\s\w and \b/\B
        parser = Parser{"\\d\\D\\s\\S\\w\\W\\b\\B"_el, GroupFlags{GroupFlag::Ascii}};
        REQUIRE_NOTHROW(node = parser.parse());
        const auto actual = node->toTestTree();
        const std::vector<std::string_view> expected{
            "Group(size=1,flags=a)",
            "  Sequence(size=8)",
            "    CharacterCategory(DigitAscii)",
            "    CharacterCategory(^DigitAscii)",
            "    CharacterCategory(SpaceAscii)",
            "    CharacterCategory(^SpaceAscii)",
            "    CharacterCategory(WordAscii)",
            "    CharacterCategory(^WordAscii)",
            "    Anchor(AsciiWordBoundary)",
            "    Anchor(NonAsciiWordBoundary)",
        };
        WITH_CONTEXT(requireLines(actual, expected));

        // Horizontal and vertical whitespace (Unicode mode)
        WITH_CONTEXT(parseAndTest(
            "\\h"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(HorizontalSpaceUnicode)",
            }));
        WITH_CONTEXT(parseAndTest(
            "\\H"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(^HorizontalSpaceUnicode)",
            }));
        WITH_CONTEXT(parseAndTest(
            "\\v"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(VerticalSpaceUnicode)",
            }));
        WITH_CONTEXT(parseAndTest(
            "\\V"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(^VerticalSpaceUnicode)",
            }));

        // Horizontal and vertical whitespace (ASCII mode)
        WITH_CONTEXT(parseAndTest(
            "\\h"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(HorizontalSpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
        WITH_CONTEXT(parseAndTest(
            "\\H"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(^HorizontalSpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
        WITH_CONTEXT(parseAndTest(
            "\\v"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(VerticalSpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
        WITH_CONTEXT(parseAndTest(
            "\\V"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(^VerticalSpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));

        // Regression: adjacent escapes must not consume the next escape's identifier.
        WITH_CONTEXT(parseAndTest(
            "\\h\\v"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    CharacterCategory(HorizontalSpaceUnicode)",
                "    CharacterCategory(VerticalSpaceUnicode)",
            }));
        WITH_CONTEXT(parseAndTest(
            "\\h\\v"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=2)",
                "    CharacterCategory(HorizontalSpaceAscii)",
                "    CharacterCategory(VerticalSpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
    }

    void testEscapeSequencesWithQuantifiers() {
        // Escape-based atoms must accept quantifiers.
        // This covers all escape sequences that create nodes for character categories/classes.
        WITH_CONTEXT(parseAndTest(
            "\\s+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(SpaceUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\S+?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=lazy)",
                "      CharacterCategory(^SpaceUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\d{2,3}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=3,mode=greedy)",
                "      CharacterCategory(DigitUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\D?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=1,mode=greedy)",
                "      CharacterCategory(^DigitUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\w*"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=greedy)",
                "      CharacterCategory(WordUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\W++"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=possessive,atomicGroupId=0)",
                "      CharacterCategory(^WordUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\h{2}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=2,mode=greedy)",
                "      CharacterCategory(HorizontalSpaceUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\H*?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=0,max=*,mode=lazy)",
                "      CharacterCategory(^HorizontalSpaceUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\v+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(VerticalSpaceUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\V{1,}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(^VerticalSpaceUnicode)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\p{Ll}+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(LowercaseLetter)",
            }));

        WITH_CONTEXT(parseAndTest(
            "\\P{UppercaseLetter}{2,3}?"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=2,max=3,mode=lazy)",
                "      CharacterCategory(^UppercaseLetter)",
            }));
    }

    void testEscapeSequencesWithQuantifiers_FlagDependent() {
        // ASCII mode changes the resulting categories for \d, \s, \w.
        WITH_CONTEXT(parseAndTest(
            "\\d+"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(DigitAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
        WITH_CONTEXT(parseAndTest(
            "\\s+"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(SpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
        WITH_CONTEXT(parseAndTest(
            "\\w+"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(WordAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));

        // DotAll changes \s / \S mappings; still must accept quantifiers.
        WITH_CONTEXT(parseAndTest(
            "\\s+"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::DotAll}));
        WITH_CONTEXT(parseAndTest(
            "\\S+"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::DotAll}));

        WITH_CONTEXT(parseAndTest(
            "\\s+"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::Ascii, GroupFlag::DotAll}));
        WITH_CONTEXT(parseAndTest(
            "\\S+"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::Ascii, GroupFlag::DotAll}));
    }

    void testUnicodeProperties() {
        // Simple property short and long names
        parseAndTest(
            "\\p{Ll}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(LowercaseLetter)",
            });
        parseAndTest(
            "\\P{UppercaseLetter}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(^UppercaseLetter)",
            });
        // Unknown or invalid property
        parser = Parser{"\\p{UnknownProperty}"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Missing brace
        parser = Parser{"\\pX"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Unterminated
        parser = Parser{"\\p{"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Too long name
        parser = Parser{"\\p{ThisPropertyNameIsWayTooLong}"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Underscore in category name is ignored
        parseAndTest(
            "\\p{Lower_case_Letter}"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(LowercaseLetter)",
            });

        // Also test negated variant for too long names
        parser = Parser{"\\P{ThisPropertyNameIsWayTooLong}"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testOctalLimits() {
        // Too many octal digits (> 11)
        parser = Parser{"\\o{123456789012}"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testSpaceAndDotAllModes() {
        // Unicode + DotAll: \s and \S should become explicit CharacterClass including/excluding line breaks
        parseAndTest(
            "\\s"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::DotAll});
        parseAndTest(
            "\\S"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::DotAll});

        // ASCII + DotAll: same as above but with ASCII flag present
        parseAndTest(
            "\\s"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::Ascii, GroupFlag::DotAll});
        parseAndTest(
            "\\S"_el,
            {
                "Group(*)",
                "  Sequence(size=1)",
                "    CharacterCategory(*)",
            },
            GroupFlags{GroupFlag::Ascii, GroupFlag::DotAll});
    }

    void testNotNewlineEscape() {
        // \N = any character except newline
        parseAndTest(
            "\\N"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([^*)",
            });

        // \N must also be quantifiable.
        parseAndTest(
            "\\N+"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    Quantifier(min=1,max=*,mode=greedy)",
                "      CharacterClass([^*)",
            });
    }

    void testQuotedLiteralBlock() {
        // Simple quoted literal block: specials must be taken as literals
        parseAndTest(
            "\\Q.^$[]\\E"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\".^$[]\")",
            });

        // Inside \Q...\E, a backslash not followed by 'E' is literal (covers EscapeSequenceHandler.hpp:179)
        parseAndTest(
            "\\Q\\x\\E"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterSequence(\"\\\\x\")",
            });

        // Unterminated literal block with trailing backslash before end (covers break at line 173 + error)
        parser = Parser{"\\Qabc\\"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testStrayQuotedEndE() {
        // RegExError: \E without preceding \Q
        parser = Parser{"\\E"_el};
        REQUIRE_THROWS(node = parser.parse());
    }
};
