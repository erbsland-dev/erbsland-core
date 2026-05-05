// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TestHelper.hpp"

#include <erbsland/cterm/Font.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>
#include <string_view>
#include <vector>

TESTED_TARGETS(Font)
class FontTest final : public el::UnitTest {
public:
    void testDefaultAsciiContainsLatinAndPunctuationGlyphs() {
        const auto font = Font::defaultAscii();
        REQUIRE_EQUAL(font->height(), 8);
        using namespace erbsland::text::literals;

        REQUIRE(font->glyph("A"_el) != nullptr);
        REQUIRE(font->glyph("\u00c4"_el) != nullptr);
        const auto punctuation = std::array<std::string_view, 23>{
            "!",
            "?",
            "<",
            ">",
            ":",
            ";",
            ".",
            ",",
            "'",
            "\"",
            "/",
            "[",
            "]",
            "-",
            "+",
            "=",
            "_",
            "(",
            ")",
            "*",
            "%",
            "$",
            "#"};
        for (std::size_t index = 0; index < punctuation.size(); ++index) {
            const auto name = punctuation[index];
            runWithContext(
                SOURCE_LOCATION(),
                [&]() { REQUIRE(font->glyph(erbsland::text::String{name}) != nullptr); },
                [&]() -> std::string { return std::format("index = {} / glyph = \"{}\"", index, name); });
        }
        REQUIRE(font->glyph("\u3042"_el) == nullptr);
    }

    void testDefaultFontFactoriesReturnIndependentInstances() {
        const auto first = Font::defaultAscii();
        const auto second = Font::defaultAscii();

        first->addGlyph("~"_el, FontGlyph{std::vector<uint64_t>{0x00, 0x00, 0x14, 0x2a, 0x00, 0x00, 0x00, 0x00}});

        using namespace erbsland::text::literals;

        REQUIRE(first->glyph("~"_el) != nullptr);
        REQUIRE(second->glyph("~"_el) == nullptr);
    }
};
