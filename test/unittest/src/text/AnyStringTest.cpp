// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/AnyStringEditor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <string>
#include <string_view>

using namespace el::text::literals;

using el::unit::CpLength;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(AnyStringEditor AnyString)
class AnyStringTest final : public el::UnitTest {
public:
    void testStringAccessors() {

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

        const auto editor = StringEditor{"oversized backing storage with a compact visible value"_el};
        const auto slice = String{editor}.slice({el::unit::ByteIndex{33U}, el::unit::ByteLength{7U}});
        const auto converted = AnyString{slice}.toU8String();

        REQUIRE_EQUAL(converted, "compact"_el);
        REQUIRE_EQUAL(converted.storageId(), slice.storageId());
    }

    void testComparisonWidthMatrix() {

        const auto u8 = AnyString{String{u8"Aé😀"_el}};
        const auto u16 = AnyString{U16String{u"Aé😀"_el}};
        const auto u32 = AnyString{U32String{U"Aé😀"_el}};

        WITH_CONTEXT(requireEqualComparison(u8, u8));
        WITH_CONTEXT(requireEqualComparison(u8, u16));
        WITH_CONTEXT(requireEqualComparison(u8, u32));
        WITH_CONTEXT(requireEqualComparison(u16, u8));
        WITH_CONTEXT(requireEqualComparison(u16, u16));
        WITH_CONTEXT(requireEqualComparison(u16, u32));
        WITH_CONTEXT(requireEqualComparison(u32, u8));
        WITH_CONTEXT(requireEqualComparison(u32, u16));
        WITH_CONTEXT(requireEqualComparison(u32, u32));
    }

    void testComparisonOperatorsAndImplicitConversions() {

        const auto text = AnyString{U16String{u"middle"_el}};
        const auto u8Editor = StringEditor{"middle"_el};
        const auto u32Editor = U32StringEditor{U"middle"_el};
        const auto anyEditor = AnyStringEditor{U16StringEditor{u"middle"_el}};

        REQUIRE_EQUAL(text.compare("middle"_el), std::strong_ordering::equal);
        REQUIRE(text == "middle"_el);
        REQUIRE(text == u8"middle"_el);
        REQUIRE(text == u"middle"_el);
        REQUIRE(text == U"middle"_el);
        REQUIRE(text == u8Editor);
        REQUIRE(text == u32Editor);
        REQUIRE(text == anyEditor);
        REQUIRE(text != "other"_el);
        REQUIRE(text < U"next"_el);
        REQUIRE(text <= u"middle"_el);
        REQUIRE(text > u8"lower"_el);
        REQUIRE(text >= "middle"_el);
        REQUIRE(U"next"_el > text);
        REQUIRE(u"middle"_el <= text);
        REQUIRE(u8"lower"_el < text);
        REQUIRE("middle"_el >= text);
        REQUIRE_EQUAL(text <=> U"next"_el, std::strong_ordering::less);
        REQUIRE_EQUAL(U"lower"_el <=> text, std::strong_ordering::less);
    }

    void testComparisonOrderingAndLength() {

        const auto prefix = AnyString{String{"Grüezi"_el}};
        const auto longer = AnyString{U32String{U"Grüezi!"_el}};
        const auto alpha = AnyString{U16String{u"ä"_el}};
        const auto beta = AnyString{U32String{U"β"_el}};

        REQUIRE_EQUAL(prefix.compare(longer), std::strong_ordering::less);
        REQUIRE_EQUAL(longer.compare(prefix), std::strong_ordering::greater);
        REQUIRE_EQUAL(alpha.compare(beta), std::strong_ordering::less);
        REQUIRE_EQUAL(beta.compare(alpha), std::strong_ordering::greater);
    }

    void testComparisonWithCustomCharacterFunction() {

        const auto upper = AnyString{U16String{u"Grüezi"_el}};
        const auto lower = AnyString{U32String{U"grüezi"_el}};

        REQUIRE_EQUAL(upper.compare(lower), std::strong_ordering::less);
        REQUIRE_EQUAL(upper.compare(lower, Char::compareAsciiFolded), std::strong_ordering::equal);
    }

    void testComparisonReplacesMalformedEncoding() {
        const auto invalidU8 = AnyString{String{StringEditor{std::string_view{th::stdStringFromHex("41 C0 42")}}}};
        const auto invalidU16 = AnyString{U16String{U16StringEditor{std::u16string{u'A', char16_t{0xD800U}, u'B'}}}};
        const auto invalidU32 = AnyString{U32String{U32StringEditor{std::u32string{U'A', char32_t{0x110000U}, U'B'}}}};

        WITH_CONTEXT(requireEqualComparison(invalidU8, invalidU16));
        WITH_CONTEXT(requireEqualComparison(invalidU8, invalidU32));
        WITH_CONTEXT(requireEqualComparison(invalidU16, invalidU32));
    }

    void testComparisonOfEmptyValues() {

        const auto empty = AnyString{};
        const auto emptyU8 = AnyString{String{}};
        const auto emptyU16 = AnyString{U16String{}};
        const auto emptyU32 = AnyString{U32String{}};

        WITH_CONTEXT(requireEqualComparison(empty, emptyU8));
        WITH_CONTEXT(requireEqualComparison(empty, emptyU16));
        WITH_CONTEXT(requireEqualComparison(empty, emptyU32));
        REQUIRE(empty == ""_el);
        REQUIRE(empty == u""_el);
        REQUIRE(empty < U"a"_el);
        REQUIRE(u8"a"_el > empty);
    }

private:
    void requireEqualComparison(const AnyString &left, const AnyString &right) {
        REQUIRE_EQUAL(left.compare(right), std::strong_ordering::equal);
        REQUIRE(left == right);
        REQUIRE_FALSE(left != right);
        REQUIRE_FALSE(left < right);
        REQUIRE(left <= right);
        REQUIRE_FALSE(left > right);
        REQUIRE(left >= right);
        REQUIRE_EQUAL(left <=> right, std::strong_ordering::equal);
    }
};
