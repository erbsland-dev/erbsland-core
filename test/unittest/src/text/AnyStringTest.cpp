// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/AnyStringEditor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string_view>

using el::unit::CpLength;
using namespace el::text;

TESTED_TARGETS(AnyStringEditor AnyString)
class AnyStringTest final : public el::UnitTest {
public:
    void testStringAccessors() {
        using namespace el::text::literals;

        const auto u8Text = StringEditor{"Aé"_el};
        const auto u16Text = U16StringEditor{std::u16string_view{u"β\n"}};
        const auto u32Text = U32StringEditor{std::u32string_view{U"中!"}};
        const auto u8 = AnyString{String{u8Text}};
        const auto u16 = AnyString{U16String{u16Text}};
        const auto u32 = AnyString{U32String{u32Text}};

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

        const auto u8 = AnyStringEditor{StringEditor{"Aé"_el}};
        const auto u16 = AnyStringEditor{U16StringEditor{std::u16string_view{u"β\n"}}};
        const auto u32 = AnyStringEditor{U32StringEditor{std::u32string_view{U"中!"}}};

        REQUIRE_EQUAL(u8.characterLength(), CpLength{2U});
        REQUIRE(u8.isEncodingValid());
        REQUIRE_EQUAL(u16.characterLength(), CpLength{2U});
        REQUIRE(u16.isEncodingValid());
        REQUIRE_EQUAL(u32.characterLength(), CpLength{2U});
        REQUIRE(u32.isEncodingValid());
    }

    void testConversion() {
        const auto u16Text = U16StringEditor{std::u16string_view{u"Grüezi"}};
        const auto text = AnyString{U16String{u16Text}};

        REQUIRE_EQUAL(StringConverter{text.toU8String()}.toStdString(), "Grüezi");
    }

    void testSameWidthConversionSharesSliceStorage() {
        using namespace el::text::literals;

        const auto editor = StringEditor{"oversized backing storage with a compact visible value"_el};
        const auto slice = String{editor}.slice({el::unit::ByteIndex{33U}, el::unit::ByteLength{7U}});
        const auto converted = AnyString{slice}.toU8String();

        REQUIRE_EQUAL(converted, "compact"_el);
        REQUIRE_EQUAL(converted.storageId(), slice.storageId());
    }
};
