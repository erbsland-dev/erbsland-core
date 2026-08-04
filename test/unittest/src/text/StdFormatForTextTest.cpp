// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

using namespace el::text::literals;

TESTED_TARGETS(StdFormatForText)
class StdFormatForTextTest final : public el::UnitTest {
public:
    void testStringsAndEditors() {
        const auto text = el::text::String{"text"_el};
        auto editor = el::text::StringEditor{"edit"_el};
        const auto text16 = el::text::StringConverter{text}.toU16String();
        const auto text32 = el::text::StringConverter{text}.toU32String();

        const auto formattedText = std::format("{}", text);
        const auto formattedEditor = std::format("{:>6}", editor);
        const auto formattedText16 = std::format("{}", text16);
        const auto formattedText32 = std::format("{}", text32);
        REQUIRE_EQUAL(formattedText, std::string{"text"});
        REQUIRE_EQUAL(formattedEditor, std::string{"  edit"});
        REQUIRE_EQUAL(formattedText16, std::string{"text"});
        REQUIRE_EQUAL(formattedText32, std::string{"text"});
    }

    void testCharacter() {
        const auto formatted = std::format("{}", el::text::Char{U'ä'});
        REQUIRE_EQUAL(formatted, std::string{"ä"});
    }

    void testCaseSensitivity() {
        const auto formatted = std::format("{}", el::text::cCaseInsensitive);
        REQUIRE_EQUAL(formatted, std::string{"case-insensitive"});
    }
};
