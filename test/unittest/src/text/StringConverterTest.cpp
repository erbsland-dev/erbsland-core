// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using namespace el::text::literals;

using namespace el::text;

namespace th = erbsland::unittest::th;

template <typename T>
concept HasModeToU8String = requires(const T &converter) { converter.toU8String(EncodingMode::Strict); };

template <typename T>
concept HasModeToStdString = requires(const T &converter) { converter.toStdString(EncodingMode::Strict); };

TESTED_TARGETS(StringConverter StringConverterTraits)
class StringConverterTest final : public el::UnitTest {
public:
    void testStdUtf8ToErbslandStrings() {
        const auto source = std::string{th::stdStringFromHex("41 C2 A2 E2 82 AC F0 9F 98 80")};

        REQUIRE_EQUAL(StringConverter{source}.toStdString(), source);
        REQUIRE_EQUAL(
            StringConverter{std::string_view{source}}.toStdU16String(),
            th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{std::u8string_view{u8"A¢€😀"}}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testStdUtf16Utf32AndWideToErbslandStrings() {
        REQUIRE_EQUAL(
            StringConverter{std::u16string_view{u"A¢€😀"}}.toStdString(),
            th::stdStringFromHex("41 C2 A2 E2 82 AC F0 9F 98 80"));
        REQUIRE_EQUAL(StringConverter{std::u32string_view{U"A¢€😀"}}.toStdU16String(), std::u16string{u"A¢€😀"});

#ifdef ERBSLAND_WCHAR_16BIT
        const auto wide = th::stdWStringFromHex("0041 00A2 20AC D83D DE00");
#else
        const auto wide = th::stdWStringFromHex("00000041 000000A2 000020AC 0001F600");
#endif
        REQUIRE_EQUAL(StringConverter{wide}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testErbslandToStdStrings() {
        const auto u8Text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        const auto u16Text = U16StringEditor{std::u16string_view{u"A¢€😀"}};
        const auto u32Text = U32StringEditor{std::u32string_view{U"A¢€😀"}};

        REQUIRE_EQUAL(StringConverter{u8Text}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{U16String{u16Text}}.toStdU16String(), std::u16string{u"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{u32Text}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{u8Text}.toStdWString()), std::u32string{U"A¢€😀"});
    }

    void testCrossWidthConversions() {

        const auto u16Text = StringConverter{u8"A¢€😀"_el}.toU16String();
        const auto u32Text = StringConverter{u16Text}.toU32String();
        const auto u8Text = StringConverter{u32Text}.toU8String();

        REQUIRE_EQUAL(StringConverter{u16Text}.toStdU16String(), std::u16string{u"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{u32Text}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{u8Text}.toStdU8String(), std::u8string{u8"A¢€😀"});
    }

    void testInvalidInputModes() {
        const auto invalid = std::string{th::stdStringFromHex("41 C0 42")};
        const auto invalidCore = U8String{U8StringEditor{std::string_view{invalid}}};

        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU32String()}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingMode::Strict));
        REQUIRE_THROWS(StringConverter{invalid}.toStdString(EncodingMode::Strict));
        REQUIRE_THROWS(StringConverter{invalidCore}.toStdString(EncodingMode::Strict));
        REQUIRE_EQUAL(StringConverter{invalidCore}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
    }

    void testEncodingModeIsAvailableForAllConversions() {
        using StdConverter = StringConverter<std::string>;
        using CoreConverter = StringConverter<U8String>;

        static_assert(HasModeToU8String<StdConverter>);
        static_assert(HasModeToU8String<CoreConverter>);
        static_assert(HasModeToStdString<StdConverter>);
        static_assert(HasModeToStdString<CoreConverter>);
    }

    void testStringLifetimeAndAliasing() {
        const auto owningView = StringConverter{std::string{th::stdStringFromHex("41 C2 A2")}}.toString();
        REQUIRE_EQUAL(StringConverter{owningView}.toStdString(), th::stdStringFromHex("41 C2 A2"));

        const auto text = U8StringEditor{std::string_view{"alias"}};
        const auto sourceView = U8String{text};
        const auto aliasView = StringConverter{sourceView}.toString();
        REQUIRE_EQUAL(aliasView.storageId(), sourceView.storageId());
    }
};
