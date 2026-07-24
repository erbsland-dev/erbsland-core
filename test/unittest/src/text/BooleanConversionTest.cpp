// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace el::text;
using namespace el::text::literals;
using namespace el::unit;

TESTED_TARGETS(U8String U8StringEditor U16String U16StringEditor U32String U32StringEditor)
class BooleanConversionTest final : public el::UnitTest {
private:
    template <typename T>
    void requireValue(const T &text, const bool expected) {
        REQUIRE_EQUAL(text.toBoolean(!expected), expected);
        REQUIRE_EQUAL(text.toBooleanOrThrow(), expected);
    }

    template <typename T>
    void requireInvalid(const T &text) {
        REQUIRE_FALSE(text.toBoolean(false));
        REQUIRE(text.toBoolean(true));
        REQUIRE_THROWS_AS(el::err::ParseError, text.toBooleanOrThrow());
    }

    void requireValueForAllWidths(const String &text, const bool expected) {
        WITH_CONTEXT(requireValue(U8String{text}, expected));
        WITH_CONTEXT(requireValue(U8StringEditor{text}, expected));
        const auto u16Text = StringConverter{text}.toU16String();
        WITH_CONTEXT(requireValue(u16Text, expected));
        WITH_CONTEXT(requireValue(U16StringEditor{u16Text}, expected));
        const auto u32Text = StringConverter{text}.toU32String();
        WITH_CONTEXT(requireValue(u32Text, expected));
        WITH_CONTEXT(requireValue(U32StringEditor{u32Text}, expected));
    }

    void requireInvalidForAllWidths(const String &text) {
        WITH_CONTEXT(requireInvalid(U8String{text}));
        WITH_CONTEXT(requireInvalid(U8StringEditor{text}));
        const auto u16Text = StringConverter{text}.toU16String();
        WITH_CONTEXT(requireInvalid(u16Text));
        WITH_CONTEXT(requireInvalid(U16StringEditor{u16Text}));
        const auto u32Text = StringConverter{text}.toU32String();
        WITH_CONTEXT(requireInvalid(u32Text));
        WITH_CONTEXT(requireInvalid(U32StringEditor{u32Text}));
    }

public:
    void testAllBooleanLiterals() {
        const auto cases = std::vector<std::pair<String, bool>>{
            {"true"_el, true},
            {"TRUE"_el, true},
            {"TrUe"_el, true},
            {"false"_el, false},
            {"FALSE"_el, false},
            {"FaLsE"_el, false},
            {"yes"_el, true},
            {"YES"_el, true},
            {"YeS"_el, true},
            {"no"_el, false},
            {"NO"_el, false},
            {"No"_el, false},
            {"on"_el, true},
            {"ON"_el, true},
            {"On"_el, true},
            {"off"_el, false},
            {"OFF"_el, false},
            {"OfF"_el, false},
            {"enabled"_el, true},
            {"ENABLED"_el, true},
            {"eNaBlEd"_el, true},
            {"disabled"_el, false},
            {"DISABLED"_el, false},
            {"dIsAbLeD"_el, false},
        };
        for (const auto &[text, expected] : cases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void { WITH_CONTEXT(requireValueForAllWidths(text, expected)); },
                [&]() -> std::string { return StringConverter{text}.toStdString(); });
        }
    }

    void testInvalidBooleanLiterals() {
        const auto cases = std::vector<String>{
            {},
            "oo"_el,
            "yep"_el,
            "tru!"_el,
            "fals!"_el,
            "enable!"_el,
            "disable!"_el,
            " true"_el,
            "false "_el,
            "enabled!"_el,
            "disabled?"_el,
            "truth"_el,
            "1"_el,
            "✓"_el,
        };
        for (const auto &text : cases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void { WITH_CONTEXT(requireInvalidForAllWidths(text)); },
                [&]() -> std::string { return StringConverter{text}.toStdString(); });
        }
    }

    void testBooleanLiteralsInSlices() {
        WITH_CONTEXT(requireValue(U8String{"--TrUe++"_el}.slice(ByteRange{ByteIndex{2U}, ByteLength{4U}}), true));
        WITH_CONTEXT(
            requireValue(U8StringEditor{"--FaLsE++"_el}.slice(ByteRange{ByteIndex{2U}, ByteLength{5U}}), false));
        WITH_CONTEXT(
            requireValue(U16String{u"--EnAbLeD++"_el}.slice(U16DataRange{U16DataIndex{2U}, U16DataLength{7U}}), true));
        WITH_CONTEXT(requireValue(
            U16StringEditor{u"--DiSaBlEd++"_el}.slice(U16DataRange{U16DataIndex{2U}, U16DataLength{8U}}), false));
        WITH_CONTEXT(requireValue(U32String{U"--YeS++"_el}.slice(CpRange{CpIndex{2U}, CpLength{3U}}), true));
        WITH_CONTEXT(requireValue(U32StringEditor{U"--OfF++"_el}.slice(CpRange{CpIndex{2U}, CpLength{3U}}), false));
    }

    void testMalformedEncodingIsInvalid() {
        const auto u8Text = U8StringEditor{std::string_view{el::unittest::th::stdStringFromHex("74 72 C0 65")}};
        WITH_CONTEXT(requireInvalid(u8Text));
        WITH_CONTEXT(requireInvalid(U8String{u8Text}));

        const auto u16Text = U16StringEditor{std::u16string{u't', u'r', char16_t{0xD800U}, u'e'}};
        WITH_CONTEXT(requireInvalid(u16Text));
        WITH_CONTEXT(requireInvalid(U16String{u16Text}));

        const auto u32Text = U32StringEditor{std::u32string{U't', U'r', char32_t{0x110000U}, U'e'}};
        WITH_CONTEXT(requireInvalid(u32Text));
        WITH_CONTEXT(requireInvalid(U32String{u32Text}));
    }
};
