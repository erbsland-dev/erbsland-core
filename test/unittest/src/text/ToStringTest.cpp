// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/BooleanFormat.hpp>
#include <erbsland/text/EscapeFormat.hpp>
#include <erbsland/text/IntegerFormat.hpp>
#include <erbsland/text/IntegerFormatFlag.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/ToString.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <cstdint>
#include <type_traits>
#include <utility>

using namespace el::text::literals;

using namespace el::text;

static_assert(std::is_same_v<decltype(toString(std::declval<const String &>())), String>);
static_assert(std::is_same_v<decltype(toString(std::declval<const StringEditor &>())), String>);
static_assert(std::is_same_v<decltype(toString(42)), String>);

TESTED_TARGETS(toString BooleanFormat Capitalization U8StringEditor)
class ToStringTest final : public el::UnitTest {
public:
    void testStringsAndBooleans() {

        const auto string = StringEditor{"hello"_el};
        const auto view = String{"world"_el};

        REQUIRE_EQUAL(toString(string), "hello"_el);
        REQUIRE_EQUAL(toString(view), "world"_el);
        REQUIRE_EQUAL(toString(true), "true"_el);
        REQUIRE_EQUAL(toString(false), "false"_el);
        REQUIRE_EQUAL(
            toString(true, BooleanFormat::trueFalse().setCapitalization(Capitalization::Uppercase)), "TRUE"_el);
        REQUIRE_EQUAL(
            toString(false, BooleanFormat::trueFalse().setCapitalization(Capitalization::Titlecase)), "False"_el);
        REQUIRE_EQUAL(StringEditor::fromBoolean(true), "true"_el);
        REQUIRE_EQUAL(
            StringEditor::fromBoolean(false, BooleanFormat::trueFalse().setCapitalization(Capitalization::Uppercase)),
            "FALSE"_el);
    }

    void testBooleanFormats() {

        auto format = BooleanFormat{};
        REQUIRE_EQUAL(format.style(), BooleanFormat::Style::TrueFalse);
        REQUIRE_EQUAL(format.capitalization(), Capitalization::Lowercase);

        REQUIRE_EQUAL(toString(true, BooleanFormat::trueFalse()), "true"_el);
        REQUIRE_EQUAL(toString(false, BooleanFormat::trueFalse()), "false"_el);
        REQUIRE_EQUAL(toString(true, BooleanFormat::yesNo()), "yes"_el);
        REQUIRE_EQUAL(toString(false, BooleanFormat::yesNo()), "no"_el);
        REQUIRE_EQUAL(toString(true, BooleanFormat::onOff()), "on"_el);
        REQUIRE_EQUAL(toString(false, BooleanFormat::onOff()), "off"_el);
        REQUIRE_EQUAL(toString(true, BooleanFormat::enabledDisabled()), "enabled"_el);
        REQUIRE_EQUAL(toString(false, BooleanFormat::enabledDisabled()), "disabled"_el);

        format.setStyle(BooleanFormat::Style::YesNo).setCapitalization(Capitalization::Uppercase);
        REQUIRE_EQUAL(format.style(), BooleanFormat::Style::YesNo);
        REQUIRE_EQUAL(format.capitalization(), Capitalization::Uppercase);
        REQUIRE_EQUAL(toString(true, format), "YES"_el);
        REQUIRE_EQUAL(toString(false, format), "NO"_el);

        REQUIRE_EQUAL(toString(true, BooleanFormat::onOff().setCapitalization(Capitalization::Titlecase)), "On"_el);
        REQUIRE_EQUAL(
            toString(false, BooleanFormat::enabledDisabled().setCapitalization(Capitalization::Titlecase)),
            "Disabled"_el);
    }

    void testStrongOrdering() {

        REQUIRE_EQUAL(toString(std::strong_ordering::less), "less"_el);
        REQUIRE_EQUAL(toString(std::strong_ordering::equal), "equal"_el);
        REQUIRE_EQUAL(toString(std::strong_ordering::equivalent), "equal"_el);
        REQUIRE_EQUAL(toString(std::strong_ordering::greater), "greater"_el);
    }

    void testIntegerValues() {

        REQUIRE_EQUAL(toString(std::int32_t{-17}), "-17"_el);

        auto format = IntegerFormat::hexadecimal();
        format.setFlags(IntegerFormatFlag::BasePrefix);
        REQUIRE_EQUAL(toString(std::uint16_t{255U}, format), "0xff"_el);
    }
};
