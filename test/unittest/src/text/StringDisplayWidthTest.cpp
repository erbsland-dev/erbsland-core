// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <string_view>

using namespace el::text;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8StringEditor U8String U16StringEditor U16String U32StringEditor U32String)
class StringDisplayWidthTest final : public el::UnitTest {
public:
    void testUtf8StringAndViewDisplayWidth() {
        const auto text = U8StringEditor{
            std::string_view{th::stdStringFromHex("2D 2D 41 E7 95 8C F0 9F 98 80 65 CC 81 0A 42 2D 2D")}};
        const auto view = U8String{text}.slice(CpRange{CpIndex{2}, CpLength{7}});
        const auto malformed = U8StringEditor{std::string_view{th::stdStringFromHex("41 C0 0A 42")}};

        REQUIRE_EQUAL(text.displayWidth(), 11);
        REQUIRE_EQUAL(U8String{text}.displayWidth(), 11);
        REQUIRE_EQUAL(view.displayWidth(), 7);
        REQUIRE_EQUAL(malformed.displayWidth(), 3);
        REQUIRE_EQUAL(U8String{malformed}.displayWidth(), 3);
    }

    void testUtf16StringAndViewDisplayWidth() {
        constexpr auto malformedData = std::array<char16_t, 4>{u'A', 0xD800U, u'\n', u'B'};
        const auto text = U16StringEditor{std::u16string_view{u"--A\u754C\U0001F600e\u0301\nB--"}};
        const auto view = U16String{text}.slice(CpRange{CpIndex{2}, CpLength{7}});
        const auto malformed = U16StringEditor{std::u16string_view{malformedData.data(), malformedData.size()}};

        REQUIRE_EQUAL(text.displayWidth(), 11);
        REQUIRE_EQUAL(U16String{text}.displayWidth(), 11);
        REQUIRE_EQUAL(view.displayWidth(), 7);
        REQUIRE_EQUAL(malformed.displayWidth(), 3);
        REQUIRE_EQUAL(U16String{malformed}.displayWidth(), 3);
    }

    void testUtf32StringAndViewDisplayWidth() {
        constexpr auto malformedData = std::array<char32_t, 4>{U'A', 0x110000U, U'\n', U'B'};
        const auto text = U32StringEditor{std::u32string_view{U"--A\u754C\U0001F600e\u0301\nB--"}};
        const auto view = U32String{text}.slice(CpRange{CpIndex{2}, CpLength{7}});
        const auto malformed = U32StringEditor{std::u32string_view{malformedData.data(), malformedData.size()}};

        REQUIRE_EQUAL(text.displayWidth(), 11);
        REQUIRE_EQUAL(U32String{text}.displayWidth(), 11);
        REQUIRE_EQUAL(view.displayWidth(), 7);
        REQUIRE_EQUAL(malformed.displayWidth(), 3);
        REQUIRE_EQUAL(U32String{malformed}.displayWidth(), 3);
    }
};
