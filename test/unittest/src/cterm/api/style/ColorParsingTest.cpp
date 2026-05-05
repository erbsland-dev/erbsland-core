// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(Color)
class ColorParsingTest final : public el::UnitTest {
public:
    void testColorPartsParseNormalizeAndBrightenAsExpected() {
        REQUIRE_EQUAL(Foreground::fromString("Bright Blue"_el), fg(fg::BrightBlue));
        REQUIRE_EQUAL(Background::fromString("bright blue"_el), bg(bg::BrightBlue));
        REQUIRE_EQUAL(Background::fromString("bright_blue"_el), bg(bg::BrightBlue));
        REQUIRE_EQUAL(Foreground::fromString(" inherited "_el), fg(fg::Inherited));
        REQUIRE_EQUAL(fg(fg::Red).toString(), "red"_el);
        REQUIRE_EQUAL(bg(bg::Default).toString(), "default"_el);
        REQUIRE_EQUAL(fg(fg::Green).brighter(), fg(fg::BrightGreen));
        REQUIRE_EQUAL(fg(fg::BrightGreen).brighter(), fg(fg::BrightGreen));
        REQUIRE_EQUAL(fg::fromIndex16(-1), fg(fg::Inherited));
        REQUIRE_EQUAL(bg::fromIndex16(16), bg(bg::Default));

        const auto baseColors = fg::allBaseColors();
        REQUIRE_EQUAL(baseColors.size(), std::size_t{8});
        REQUIRE_EQUAL(baseColors[0], fg(fg::Black));
        REQUIRE_EQUAL(baseColors[7], fg(fg::White));

        REQUIRE_THROWS_AS(std::invalid_argument, fg::fromString("unknown"_el));
        REQUIRE_THROWS_AS(std::invalid_argument, Background::fromString("bright-blue"_el));
    }

    void testColorsParseFromSingleAndPairedStrings() {
        REQUIRE_EQUAL(Color::fromString("green"_el), Color(fg::Green, bg::Inherited));
        REQUIRE_EQUAL(Color::fromString("bright white : blue"_el), Color(fg::BrightWhite, bg::Blue));
        REQUIRE_EQUAL(Color::fromString("default:bright_black"_el), Color(fg::Default, bg::BrightBlack));
        REQUIRE_EQUAL(Color::fromIndex16(-1, 99), Color(fg::Inherited, bg::Default));
        REQUIRE_EQUAL(Color::reset(), Color(fg::Default, bg::Default));

        REQUIRE_THROWS_AS(std::invalid_argument, Color::fromString("green:unknown"_el));
    }
};
