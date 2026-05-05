// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/Alignment.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/SafeStringFlag.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/TruncateMode.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringCharView.hpp>
#include <erbsland/text/u16/U16StringView.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringView.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringCharView.hpp>
#include <erbsland/text/u8/U8StringView.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using el::bgeo::Alignment;
using el::unit::CpLength;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(
    U8String U8StringView U8StringCharView U16String U16StringView U16StringCharView U32String U32StringView
        TruncateMode SafeStringFlag SafeStringFlags SafeStringEscapeTools)
class StringTransformTest final : public el::UnitTest {
public:
    void testU8TruncationModes() {
        using namespace el::text::literals;

        auto text = U8String{std::u8string_view{u8"A¢€😀Z"}};

        REQUIRE_EQUAL(StringConverter{text.truncated(CpLength{3U})}.toStdU8String(), std::u8string{u8"A¢€"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{3U}, TruncateMode::Begin)}.toStdU8String(),
            std::u8string{u8"€😀Z"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{3U}, TruncateMode::Middle)}.toStdU8String(),
            std::u8string{u8"A¢Z"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{4U}, TruncateMode::Middle, "."_elv)}.toStdU8String(),
            std::u8string{u8"A¢.Z"});
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength{2U}, TruncateMode::End, "..."_elv)}.toStdU8String(),
            std::u8string{u8"A¢"});
        REQUIRE(text.truncated(CpLength::zero()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{text.truncated(CpLength::infinite())}.toStdU8String(),
            StringConverter{text}.toStdU8String());

        text.truncate(CpLength{4U}, TruncateMode::Begin, "."_elv);
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8".€😀Z"});
    }

    void testNativeTruncationApis() {
        using namespace el::text::literals;

        const auto u8Text = U8String{std::u8string_view{u8"A¢€😀Z"}};
        REQUIRE_EQUAL(
            StringConverter{U8StringView{u8Text}.toCharView().truncated(CpLength{4U}, TruncateMode::End, "."_elv)}
                .toStdU8String(),
            std::u8string{u8"A¢€."});

        const auto u16Text = U16String{std::u16string_view{u"A¢€😀Z"}};
        REQUIRE_EQUAL(
            StringConverter{u16Text.truncated(CpLength{4U}, TruncateMode::End, u"…"_elv)}.toStdU16String(),
            std::u16string{u"A¢€…"});
        REQUIRE_EQUAL(
            StringConverter{U16StringView{u16Text}.toCharView().truncated(CpLength{3U}, TruncateMode::Middle)}
                .toStdU16String(),
            std::u16string{u"A¢Z"});

        const auto u32Text = U32String{std::u32string_view{U"A¢€😀Z"}};
        REQUIRE_EQUAL(
            StringConverter{u32Text.truncated(CpLength{4U}, TruncateMode::Begin, U"…"_elv)}.toStdU32String(),
            std::u32string{U"…€😀Z"});
        REQUIRE_EQUAL(
            StringConverter{U32StringView{u32Text}.truncated(CpLength{4U}, TruncateMode::Middle, U"."_elv)}
                .toStdU32String(),
            std::u32string{U"A¢.Z"});
    }

    void testAlignment() {
        const auto text = U8String{std::u8string_view{u8"Ab¢"}};

        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Left, U'.')}.toStdU8String(),
            std::u8string{u8"Ab¢..."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Right, U'.')}.toStdU8String(),
            std::u8string{u8"...Ab¢"});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Center, U'.')}.toStdU8String(),
            std::u8string{u8".Ab¢.."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{6U}, Alignment::Top, U'.')}.toStdU8String(),
            std::u8string{u8"Ab¢..."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{5U}, Alignment::Left, U'.')}.toStdU8String(),
            std::u8string{u8"Ab¢.."});
        REQUIRE_EQUAL(
            StringConverter{text.aligned(CpLength{5U}, Alignment::Right, U'.')}.toStdU8String(),
            std::u8string{u8"..Ab¢"});

        const auto u16Text = U16String{std::u16string_view{u"Ab¢"}};
        REQUIRE_EQUAL(
            StringConverter{u16Text.aligned(CpLength{5U}, Alignment::Right, U'.')}.toStdU16String(),
            std::u16string{u"..Ab¢"});

        const auto u32Text = U32String{std::u32string_view{U"Ab¢"}};
        REQUIRE_EQUAL(
            StringConverter{u32Text.aligned(CpLength{5U}, Alignment::Center, U'.')}.toStdU32String(),
            std::u32string{U".Ab¢."});
    }

    void testSafeString() {
        const auto text = U8String{std::u8string_view{u8"A\né Z"}};

        REQUIRE_EQUAL(
            StringConverter{text.toSafeString(CpLength{100U})}.toStdU8String(), std::u8string{u8"\"A\\né Z\""});
        REQUIRE_EQUAL(
            StringConverter{text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii)}.toStdU8String(),
            std::u8string{u8"A\\n\\u00E9 Z"});
        REQUIRE_EQUAL(
            StringConverter{text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii | SafeStringFlag::AutoQuotes)}
                .toStdU8String(),
            std::u8string{u8"\"A\\n\\u00E9 Z\""});

        const auto longText = U8String{std::string_view{"abcdefghijklmnopqrstuvwxyz"}};
        REQUIRE_EQUAL(
            StringConverter{longText.toSafeString(CpLength{20U})}.toStdString(), std::string{"\"abcd(... +22 more)\""});
        REQUIRE_EQUAL(StringConverter{longText.toSafeString(CpLength{3U})}.toStdString(), std::string{"abc"});

        const auto invalidUtf8 = U8String{std::string_view{th::stdStringFromHex("41 C0 42")}};
        REQUIRE_EQUAL(
            StringConverter{invalidUtf8.toSafeString(
                                CpLength{20U}, SafeStringFlags{SafeStringFlag::OnlyAscii, SafeStringFlag::AutoQuotes})}
                .toStdString(),
            std::string{"\"A\\uFFFDB\""});

        const auto u16Text = U16String{std::u16string_view{u"A\né"}};
        REQUIRE_EQUAL(
            StringConverter{u16Text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii)}.toStdU16String(),
            std::u16string{u"A\\n\\u00E9"});

        const auto u32Text = U32String{std::u32string_view{U"A\né"}};
        REQUIRE_EQUAL(
            StringConverter{u32Text.toSafeString(CpLength{100U}, SafeStringFlag::OnlyAscii)}.toStdU32String(),
            std::u32string{U"A\\n\\u00E9"});
    }
};
