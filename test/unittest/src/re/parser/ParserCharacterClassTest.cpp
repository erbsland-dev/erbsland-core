// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/StdFormat.hpp>

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserCharacterClassTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testBasicLiteralsAndRanges() {
        // Single literal
        WITH_CONTEXT(parseAndTest(
            "[a]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([a])",
            }));

        // Simple range
        WITH_CONTEXT(parseAndTest(
            "[a-z]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([a-z])",
            }));

        // Invalid range order (start > end) -> error
        parser = Parser{"[z-a]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Leading '-' treated as literal
        WITH_CONTEXT(parseAndTest(
            "[-]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([\\u002D])",
            }));

        // Negated class
        WITH_CONTEXT(parseAndTest(
            "[^a]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([^a])",
            }));

        // Double '-' inside a range -> error
        parser = Parser{"[a--b]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Trailing '-' -> error
        parser = Parser{"[a-]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // '-' without preceding literal when content exists (after \d) -> error
        parser = Parser{"[\\d-]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // '^' in the middle -> error
        parser = Parser{"[a^]"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testFinalizePendingLiteral() {
        // Pending start literal at class end should be added
        WITH_CONTEXT(parseAndTest(
            "[a]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([a])",
            }));
    }

    void testCategoriesAndMerging() {
        // Category-only => CharacterCategory node
        WITH_CONTEXT(parseAndTest(
            "[\\d]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(DigitUnicode)",
            }));

        // Literal + category => merged into CharacterClass node
        WITH_CONTEXT(parseAndTest(
            "[a\\d]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([0-9a*])", // * = Unicode digits
            }));

        // Start literal followed by category should process start literal before range/category (non-throw)
        WITH_CONTEXT(parseAndTest(
            "[a\\w]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([0-9A-Z_a-z*])", // * = Unicode letters and digits
            }));
    }

    void testSpaceClasses() {
        // Space class (Unicode)
        WITH_CONTEXT(parseAndTest(
            "[\\s]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(SpaceUnicode)",
            }));
        // Space class (ASCII)
        WITH_CONTEXT(parseAndTest(
            "[\\s]"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(SpaceAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
    }

    void testEscapedLiterals() {
        // All literal-escapes are allowed as characters inside a class
        WITH_CONTEXT(parseAndTest(
            "[\\.\\\\\\^\\$\\|\\{\\}\\(\\)\\[\\]\\+\\*\\?\\ \\\"\\'\\#]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([*])",
            }));
    }

    void testSpecialEscapesAndNumbers() {
        // Special escapes
        WITH_CONTEXT(parseAndTest(
            "[\\a\\cA\\e\\f\\n\\r\\t]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([*])",
            }));

        // Octal, hex, unicode forms
        WITH_CONTEXT(parseAndTest(
            "[\\o{141}\\x41\\x{100}\\u0041\\U00000041]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([*])",
            }));
    }

    void testQuotedBlocks() {
        // Quoted literal inside class
        WITH_CONTEXT(parseAndTest(
            "[\\Qab\\E]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([*])",
            }));

        // Unterminated quoted block
        parser = Parser{"[\\Qabc]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Unterminated quoted block with unexpected end right after backslash
        parser = Parser{"[\\Qabc\\"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Unexpected end quote without start
        parser = Parser{"[\\E]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Backslash inside quoted content should be treated as a literal (not ending the block)
        WITH_CONTEXT(parseAndTest(
            "[\\Qx\\y\\E]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([\\\\x-y])",
            }));
    }

    void testAsciiDigitAndWord() {
        // \d in ASCII mode -> Category Digit
        WITH_CONTEXT(parseAndTest(
            "[\\d]"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(DigitAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
        // \w in ASCII mode -> Category Word
        WITH_CONTEXT(parseAndTest(
            "[\\w]"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
                "    CharacterCategory(WordAscii)",
            },
            GroupFlags{GroupFlag::Ascii}));
    }

    void testUnicodeProperty() {
        // Valid property
        WITH_CONTEXT(parseAndTest(
            "[\\p{Ll}]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory(*)",
            }));
        // Missing '{'
        parser = Parser{"[\\p]"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Unknown property
        parser = Parser{"[\\p{unknown}]"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Invalid character in name
        parser = Parser{"[\\p{L-}]"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testNegatedClassesAndAnchorsNotAllowed() {
        // Negated ranges inside class are not allowed
        const std::vector<String> negated = {
            "[\\D]"_el, "[\\S]"_el, "[\\W]"_el, "[\\H]"_el, "[\\N]"_el, "[\\V]"_el, "[\\P{Ll}]"_el};
        for (const auto &pat : negated) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    parser = Parser{pat};
                    REQUIRE_THROWS(node = parser.parse());
                },
                [&]() { return std::format("pattern: {}", pat.toSafeString(el::unit::CpLength{200U})); });
        }
        // Anchors are not allowed in classes
        const std::vector<String> anchors = {"[\\A]"_el, "[\\z]"_el, "[\\Z]"_el, "[\\b]"_el, "[\\B]"_el};
        for (const auto &pat : anchors) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    parser = Parser{pat};
                    REQUIRE_THROWS(node = parser.parse());
                },
                [&]() { return std::format("pattern: {}", pat.toSafeString(el::unit::CpLength{200U})); });
        }
    }

    void testUnexpectedEscapeAndEndCases() {
        // Unknown escape
        parser = Parser{"[\\y]"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Unexpected end of pattern in class (open range)
        parser = Parser{"[a-"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Empty class
        parser = Parser{"[]"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testRangeMustNotEndWithCategoryOrList() {
        // Open range then \d -> should error: "A range must not end in character class"
        parser = Parser{"[a-\\d]"_el};
        REQUIRE_THROWS(node = parser.parse());
        // Open range then \h (range list) -> should error as well
        parser = Parser{"[a-\\h]"_el};
        REQUIRE_THROWS(node = parser.parse());
    }

    void testCaseInsensitiveFoldingAscii() {
        // With case-insensitive flag, ASCII ranges should fold to lower-case
        WITH_CONTEXT(parseAndTest(
            "(?i)[A-Z]"_el,
            {
                "Group(size=1,flags=i)",
                "  Sequence(size=1)",
                "    CharacterClass([a-z])",
            }));

        // Also works for a list of uppercase ASCII literals
        WITH_CONTEXT(parseAndTest(
            "(?i)[ABC]"_el,
            {
                "Group(size=1,flags=i)",
                "  Sequence(size=1)",
                // Adjacent literals are normalized to a range by CharClass normalization
                "    CharacterClass([a-c])",
            }));
    }

    void testCaseInsensitiveFoldingUnicode() {
        // Non-ASCII uppercase umlauts should fold to lowercase with (?i)
        WITH_CONTEXT(parseAndTest(
            "(?i)[ÄÖÜ]"_el,
            {
                "Group(size=1,flags=i)",
                "  Sequence(size=1)",
                "    CharacterClass([\\u00E4\\u00F6\\u00FC])",
            }));

        // And ranges should be normalized to lower-case range
        WITH_CONTEXT(parseAndTest(
            "(?i)[Ä-Ü]"_el,
            {
                "Group(size=1,flags=i)",
                "  Sequence(size=1)",
                "    CharacterClass([\\u00E4-\\u00FC])",
            }));
    }

    void testMismatchedRangeErrors() {
        // Range with mixed case endpoints is accepted in case-sensitive patterns.
        WITH_CONTEXT(parseAndTest(
            "[A-z]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterClass([A-z])",
            }));

        // The same range is rejected with the case-insensitive flag,
        // as case folding would change the meaning of the expression.
        parser = Parser{"(?i)[A-z]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // Unicode mismatched ranges should also error (mismatched case, begin < end)
        parser = Parser{"(?i)[Ä-ö]"_el};
        REQUIRE_THROWS(node = parser.parse());
        parser = Parser{"(?i)[a-Ü]"_el};
        REQUIRE_THROWS(node = parser.parse());
    }
};
