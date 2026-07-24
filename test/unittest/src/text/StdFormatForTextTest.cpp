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

        REQUIRE_EQUAL(std::format("{}", text), std::string{"text"});
        REQUIRE_EQUAL(std::format("{:>6}", editor), std::string{"  edit"});
        REQUIRE_EQUAL(std::format("{}", text16), std::string{"text"});
        REQUIRE_EQUAL(std::format("{}", text32), std::string{"text"});
    }

    void testCharacter() { REQUIRE_EQUAL(std::format("{}", el::text::Char{U'ä'}), std::string{"ä"}); }

    void testCaseSensitivity() {
        REQUIRE_EQUAL(std::format("{}", el::text::cCaseInsensitive), std::string{"case-insensitive"});
    }
};
