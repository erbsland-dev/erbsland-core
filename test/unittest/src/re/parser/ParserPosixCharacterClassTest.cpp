// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserBase.hpp"

#include <erbsland/re/StdFormatForRegEx.hpp>

TESTED_TARGETS(Parser)
TAGS(Parsing)
class ParserPosixCharacterClassTest final : public UNITTEST_SUBCLASS(ParserBase) {
public:
    void testAllNamesUnicode() {
        // All supported names in Unicode mode (default). We do not validate exact set definitions.
        // We only assert that a character class/category node is created and parsing succeeds.
        const std::vector<String> patterns = {
            "[[:alnum:]]"_el,
            "[[:alpha:]]"_el,
            "[[:ascii:]]"_el,
            "[[:blank:]]"_el,
            "[[:cntrl:]]"_el,
            "[[:digit:]]"_el,
            "[[:graph:]]"_el,
            "[[:lower:]]"_el,
            "[[:print:]]"_el,
            "[[:punct:]]"_el,
            "[[:space:]]"_el,
            "[[:upper:]]"_el,
            "[[:word:]]"_el,
            "[[:xdigit:]]"_el,
        };

        for (const auto &pat : patterns) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    parseAndTest(
                        pat,
                        {
                            "Group(size=?)",
                            "  Sequence(*",
                        },
                        {
                            "    CharacterClass(*",
                            "    CharacterCategory(*",
                        });
                },
                [&]() -> std::string {
                    return std::format("Failed at pattern \"{}\"", pat.toSafeString(el::unit::CpLength{200U}));
                });
        }
    }

    void testAllNamesAscii() {
        // Same patterns, but in ASCII mode. Ensure they still parse to a range/category node.
        const GroupFlags ascii{GroupFlag::Ascii};
        const std::vector<String> patterns = {
            "[[:alnum:]]"_el,
            "[[:alpha:]]"_el,
            "[[:ascii:]]"_el,
            "[[:blank:]]"_el,
            "[[:cntrl:]]"_el,
            "[[:digit:]]"_el,
            "[[:graph:]]"_el,
            "[[:lower:]]"_el,
            "[[:print:]]"_el,
            "[[:punct:]]"_el,
            "[[:space:]]"_el,
            "[[:upper:]]"_el,
            "[[:word:]]"_el,
            "[[:xdigit:]]"_el,
        };
        for (const auto &pat : patterns) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    parseAndTest(
                        pat,
                        {
                            "Group(size=1,flags=a)",
                            "  Sequence(*",
                        },
                        {
                            "    CharacterClass(*",
                            "    CharacterCategory(*",
                        },
                        {},
                        ascii);
                },
                [&]() -> std::string {
                    return std::format("Failed at pattern \"{}\"", pat.toSafeString(el::unit::CpLength{200U}));
                });
        }
    }

    void testUnionOfPosixClasses() {
        // Unions like [[:alpha:][:digit:]] should parse as a single range/category node.
        parseAndTest(
            "[[:alpha:][:digit:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
            },
            {
                "    CharacterClass(*",
                "    CharacterCategory(*",
            });

        // More unions
        parseAndTest(
            "[[:alpha:][:digit:][:space:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
            },
            {
                "    CharacterClass(*",
                "    CharacterCategory(*",
            });
    }

    void testEmbeddedSequenceEndDetection() {
        // Ensure the end of the POSIX class is detected and following chars are outside the class.
        parseAndTest(
            "[[:digit:]]x"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
            },
            {
                "    CharacterClass(*",
                "    CharacterCategory(*",
            },
            {
                "    CharacterSequence(\"x\")",
            });

        // And a prefix literal followed by POSIX class
        parseAndTest(
            "x[[:alpha:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=2)",
                "    CharacterSequence(\"x\")",
            },
            {
                "    CharacterClass(*",
                "    CharacterCategory(*",
            });
    }

    void testBranches_CategoryOnly_RangeOnly_Combined_Negated() {
        // Category-only (Unicode): [:alpha:] -> categories
        parseAndTest(
            "[[:alpha:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
            },
            {
                "    CharacterCategory(*",
                "    CharacterClass(*",
            });
        // Range-only (ASCII): [:alpha:] under ASCII -> ranges
        parseAndTest(
            "[[:alpha:]]"_el,
            {
                "Group(size=1,flags=a)",
                "  Sequence(size=1)",
            },
            {
                "    CharacterClass(*",
            },
            {},
            GroupFlags{GroupFlag::Ascii});
        // Combined (Unicode blank adds ranges + Zs category)
        parseAndTest(
            "[[:blank:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
            },
            {
                "    CharacterClass(*",
                "    CharacterCategory(*",
            });
        // Explicit checks for current representation
        parseAndTest(
            "[[:graph:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory((Letter|Mark|Number|Punctuation|Symbol))",
            });
        parseAndTest(
            "[[:print:]]"_el,
            {
                "Group(size=1)",
                "  Sequence(size=1)",
                "    CharacterCategory((Letter|Mark|Number|Punctuation|Symbol|Separator))",
            });
    }

    void testErrors() {
        // 1) Unknown name
        parser = Parser{"[[:unknown:]]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 2) Missing name after '[:'
        parser = Parser{"[[:]]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 3) Missing ':]' terminator (e.g., stop after name)
        parser = Parser{"[[:alpha]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 4) Mixing POSIX classes with normal characters inside class
        parser = Parser{"[[:alpha:]a]"_el}; // after closing posix, next char not ']' or '[' before end
        REQUIRE_THROWS(node = parser.parse());

        // 5) Stray '[' inside class (e.g., nested start that is not a POSIX start)
        parser = Parser{"[[a]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 6) Premature end / unterminated class
        parser = Parser{"[[:digit:"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 7) Negated POSIX class combined with another POSIX class
        parser = Parser{"[[:^alpha:][:digit:]]"_el};
        REQUIRE_THROWS(node = parser.parse());

        // 8) Negated POSIX class appearing after an initial positive class
        parser = Parser{"[[:alpha:][:^digit:]]"_el};
        REQUIRE_THROWS(node = parser.parse());
    }
};
