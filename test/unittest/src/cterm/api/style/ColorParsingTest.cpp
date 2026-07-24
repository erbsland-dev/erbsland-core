// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/err/ParseError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

TESTED_TARGETS(Color ColorBase ColorPart Foreground Background BlockAttributes BlockStyle)
class ColorParsingTest final : public el::UnitTest {
public:
    void testEveryColorPartRoundTripsWithCanonicalNames() {
        constexpr auto names = std::array{
            "black"_el,
            "red"_el,
            "green"_el,
            "yellow"_el,
            "blue"_el,
            "magenta"_el,
            "cyan"_el,
            "white"_el,
            "bright_black"_el,
            "bright_red"_el,
            "bright_green"_el,
            "bright_yellow"_el,
            "bright_blue"_el,
            "bright_magenta"_el,
            "bright_cyan"_el,
            "bright_white"_el,
            "default"_el,
            "inherited"_el,
        };
        constexpr auto foregrounds = std::array{
            Foreground{fg::Black},
            Foreground{fg::Red},
            Foreground{fg::Green},
            Foreground{fg::Yellow},
            Foreground{fg::Blue},
            Foreground{fg::Magenta},
            Foreground{fg::Cyan},
            Foreground{fg::White},
            Foreground{fg::BrightBlack},
            Foreground{fg::BrightRed},
            Foreground{fg::BrightGreen},
            Foreground{fg::BrightYellow},
            Foreground{fg::BrightBlue},
            Foreground{fg::BrightMagenta},
            Foreground{fg::BrightCyan},
            Foreground{fg::BrightWhite},
            Foreground{fg::Default},
            Foreground{fg::Inherited},
        };
        for (auto index = std::size_t{0}; index < names.size(); ++index) {
            REQUIRE_EQUAL(Foreground::fromStringOrThrow(names[index]), foregrounds[index]);
            REQUIRE_EQUAL(Foreground::fromStringOrThrow(names[index]).toString(), names[index]);
            REQUIRE_EQUAL(Background::fromStringOrThrow(names[index]).toString(), names[index]);
        }
    }

    void testColorPartsNormalizeIdentifiersAndUseFallbacks() {
        REQUIRE_EQUAL(Foreground::fromStringOrThrow("Bright Blue"_el), Foreground{fg::BrightBlue});
        REQUIRE_EQUAL(Background::fromStringOrThrow("bright blue"_el), Background{bg::BrightBlue});
        REQUIRE_EQUAL(Background::fromStringOrThrow("bright_blue"_el), Background{bg::BrightBlue});
        REQUIRE_EQUAL(Foreground::fromString("unknown"_el, Foreground{fg::Yellow}), Foreground{fg::Yellow});
        REQUIRE_EQUAL(Background::fromString("reset"_el, Background{bg::Cyan}), Background{bg::Cyan});

        REQUIRE_THROWS_AS(erbsland::err::ParseError, Foreground::fromStringOrThrow("unknown"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, Background::fromStringOrThrow("bright-blue"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, Foreground::fromStringOrThrow(" inherited "_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, Foreground::fromStringOrThrow("reset"_el));
    }

    void testColorPartUtilitiesRemainAvailable() {
        REQUIRE_EQUAL(fg(fg::Green).brighter(), fg(fg::BrightGreen));
        REQUIRE_EQUAL(fg(fg::BrightGreen).brighter(), fg(fg::BrightGreen));
        REQUIRE_EQUAL(fg::fromIndex16(-1), fg(fg::Inherited));
        REQUIRE_EQUAL(bg::fromIndex16(16), bg(bg::Default));

        const auto baseColors = fg::allBaseColors();
        REQUIRE_EQUAL(baseColors.size(), std::size_t{8});
        REQUIRE_EQUAL(baseColors[0], fg(fg::Black));
        REQUIRE_EQUAL(baseColors[7], fg(fg::White));
    }

    void testColorsParseSerializeAndUseFallbacks() {
        REQUIRE_EQUAL(Color::fromStringOrThrow("green"_el), Color(fg::Green, bg::Inherited));
        REQUIRE_EQUAL(Color::fromStringOrThrow("Bright White:blue"_el), Color(fg::BrightWhite, bg::Blue));
        REQUIRE_EQUAL(Color::fromStringOrThrow("default:bright_black"_el), Color(fg::Default, bg::BrightBlack));
        REQUIRE_EQUAL((Color{fg::Green, bg::Inherited}.toString()), "green"_el);
        REQUIRE_EQUAL((Color{fg::Default, bg::BrightBlack}.toString()), "default:bright_black"_el);
        REQUIRE_EQUAL(Color::fromString("green:unknown"_el, Color{fg::Red, bg::Blue}), Color(fg::Red, bg::Blue));
        REQUIRE_EQUAL(Color::fromIndex16(-1, 99), Color(fg::Inherited, bg::Default));
        REQUIRE_EQUAL(Color::reset(), Color(fg::Default, bg::Default));

        REQUIRE_THROWS_AS(erbsland::err::ParseError, Color::fromStringOrThrow("green:unknown"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, Color::fromStringOrThrow("green :blue"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, Color::fromStringOrThrow("green:blue:red"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, Color::fromStringOrThrow(":blue"_el));
    }

    void testBlockAttributesParseSerializeAndRejectAmbiguity() {
        auto expected = BlockAttributes{};
        expected.setBold(true);
        expected.setItalic(false);
        expected.setStrikethrough(true);

        REQUIRE_EQUAL(BlockAttributes::fromStringOrThrow("inherited"_el), BlockAttributes{});
        REQUIRE_EQUAL(BlockAttributes::fromStringOrThrow("Inherited"_el), BlockAttributes{});
        REQUIRE_EQUAL(BlockAttributes::fromStringOrThrow("+Bold,-italic,strikethrough"_el), expected);
        REQUIRE_EQUAL(expected.toString(), "bold,-italic,strikethrough"_el);
        REQUIRE_EQUAL(
            BlockAttributes::reset().toString(),
            "-bold,-dim,-italic,-underline,-blink,-reverse,-hidden,-strikethrough"_el);
        REQUIRE_EQUAL(BlockAttributes::fromString("bold,bold"_el, BlockAttributes::reset()), BlockAttributes::reset());

        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow(""_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("bold,bold"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("bold,-bold"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("inherited,bold"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("+inherited"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("bold=on"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("bold, italic"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockAttributes::fromStringOrThrow("bold,"_el));
    }

    void testBlockStylesParseSerializeAndRoundTrip() {
        auto attributes = BlockAttributes{};
        attributes.setBold(true);
        attributes.setUnderline(false);
        const auto oneField = BlockStyle{fg::BrightCyan};
        const auto twoFields = BlockStyle{fg::BrightCyan, bg::Black};
        const auto threeFields = BlockStyle{Color{fg::BrightCyan, bg::Inherited}, attributes};

        REQUIRE_EQUAL(BlockStyle::fromStringOrThrow("bright_cyan"_el), oneField);
        REQUIRE_EQUAL(BlockStyle::fromStringOrThrow("bright_cyan:black"_el), twoFields);
        REQUIRE_EQUAL(BlockStyle::fromStringOrThrow("bright_cyan:inherited:bold,-underline"_el), threeFields);
        REQUIRE_EQUAL(oneField.toString(), "bright_cyan"_el);
        REQUIRE_EQUAL(twoFields.toString(), "bright_cyan:black"_el);
        REQUIRE_EQUAL(threeFields.toString(), "bright_cyan:inherited:bold,-underline"_el);
        REQUIRE_EQUAL(BlockStyle::fromStringOrThrow(threeFields.toString()), threeFields);
        REQUIRE_EQUAL(BlockStyle::fromString("red:blue:bold:bold"_el, BlockStyle::reset()), BlockStyle::reset());

        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockStyle::fromStringOrThrow("red:blue:"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockStyle::fromStringOrThrow("red::bold"_el));
        REQUIRE_THROWS_AS(erbsland::err::ParseError, BlockStyle::fromStringOrThrow("red:blue:bold:dim"_el));
    }
};
