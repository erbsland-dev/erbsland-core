// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/AnyStringView.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string_view>

using el::unit::CpLength;
using namespace el::text;

TESTED_TARGETS(AnyString AnyStringView)
class AnyStringTest final : public el::UnitTest {
public:
    void testStringViewAccessors() {
        using namespace el::text::literals;

        const auto u8Text = String{"Aé"_el};
        const auto u16Text = U16String{std::u16string_view{u"β\n"}};
        const auto u32Text = U32String{std::u32string_view{U"中!"}};
        const auto u8 = AnyStringView{StringView{u8Text}};
        const auto u16 = AnyStringView{U16StringView{u16Text}};
        const auto u32 = AnyStringView{U32StringView{u32Text}};

        REQUIRE_EQUAL(u8.kind().value(), StringKind::U8);
        REQUIRE_EQUAL(u8.characterLength(), CpLength{2U});
        REQUIRE(u8.isEncodingValid());
        REQUIRE_EQUAL(u16.kind().value(), StringKind::U16);
        REQUIRE_EQUAL(u16.characterLength(), CpLength{2U});
        REQUIRE(u16.isEncodingValid());
        REQUIRE_EQUAL(u32.kind().value(), StringKind::U32);
        REQUIRE_EQUAL(u32.characterLength(), CpLength{2U});
        REQUIRE(u32.isEncodingValid());
    }

    void testOwningStringAccessors() {
        using namespace el::text::literals;

        const auto u8 = AnyString{String{"Aé"_el}};
        const auto u16 = AnyString{U16String{std::u16string_view{u"β\n"}}};
        const auto u32 = AnyString{U32String{std::u32string_view{U"中!"}}};

        REQUIRE_EQUAL(u8.characterLength(), CpLength{2U});
        REQUIRE(u8.isEncodingValid());
        REQUIRE_EQUAL(u16.characterLength(), CpLength{2U});
        REQUIRE(u16.isEncodingValid());
        REQUIRE_EQUAL(u32.characterLength(), CpLength{2U});
        REQUIRE(u32.isEncodingValid());
    }

    void testConversion() {
        const auto u16Text = U16String{std::u16string_view{u"Grüezi"}};
        const auto text = AnyStringView{U16StringView{u16Text}};

        REQUIRE_EQUAL(StringConverter{text.toU8String()}.toStdString(), "Grüezi");
    }
};
