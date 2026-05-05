// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StringBuilder.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringLiteral.hpp>
#include <erbsland/text/u16/U16StringView.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringLiteral.hpp>
#include <erbsland/text/u32/U32StringView.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringLiteral.hpp>
#include <erbsland/text/u8/U8StringView.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unit/U16DataIndex.hpp>
#include <erbsland/unit/U16DataLength.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <string>
#include <string_view>

using el::unit::ByteLength;
using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::ElementCount;
using el::unit::U16DataIndex;
using el::unit::U16DataLength;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(StringBuilder StringKind StringBuilderBase U8StringBuilder U16StringBuilder U32StringBuilder)
class StringBuilderTest final : public el::UnitTest {
public:
    void testConstruction() {
        const auto defaultBuilder = StringBuilder{};
        const auto u8Builder = StringBuilder{StringKind::U8};
        const auto u16Builder = StringBuilder{StringKind::U16};
        const auto u32Builder = StringBuilder{StringKind::U32};
        const auto u8FactoryBuilder = StringBuilder::u8();
        const auto u16FactoryBuilder = StringBuilder::u16();
        const auto u32FactoryBuilder = StringBuilder::u32();
        const auto u8CapacityBuilder = StringBuilder::withCapacity(StringKind::U8, CpLength{1U});
        const auto u16CapacityBuilder = StringBuilder::withCapacity(StringKind::U16, CpLength{1U});
        const auto u32CapacityBuilder = StringBuilder::withCapacity(StringKind::U32, CpLength{1U});

        REQUIRE_EQUAL(defaultBuilder.kind(), StringKind::U8);
        REQUIRE(defaultBuilder.isEmpty());
        REQUIRE(defaultBuilder.length().isZero());
        REQUIRE_EQUAL(u8Builder.kind(), StringKind::U8);
        REQUIRE_EQUAL(u16Builder.kind(), StringKind::U16);
        REQUIRE_EQUAL(u32Builder.kind(), StringKind::U32);
        REQUIRE_EQUAL(u8FactoryBuilder.kind(), StringKind::U8);
        REQUIRE_EQUAL(u16FactoryBuilder.kind(), StringKind::U16);
        REQUIRE_EQUAL(u32FactoryBuilder.kind(), StringKind::U32);
        REQUIRE_EQUAL(u8CapacityBuilder.kind(), StringKind::U8);
        REQUIRE_EQUAL(u16CapacityBuilder.kind(), StringKind::U16);
        REQUIRE_EQUAL(u32CapacityBuilder.kind(), StringKind::U32);
    }

    void testNativeCapacityFactories() {
        const auto u8Capacity = ByteLength{12U};
        const auto u16Capacity = U16DataLength{8U};
        const auto u32Capacity = CpLength{6U};

        REQUIRE(StringBuilder::u8(u8Capacity).takeU8String().capacity() >= u8Capacity);
        REQUIRE(StringBuilder::u16(u16Capacity).takeU16String().capacity() >= u16Capacity);
        REQUIRE(StringBuilder::u32(u32Capacity).takeU32String().capacity() >= u32Capacity);
    }

    void testGenericCapacityFactory() {
        REQUIRE(StringBuilder::withCapacity(StringKind::U8, CpLength{3U}).takeU8String().capacity() >= ByteLength{12U});
        REQUIRE(
            StringBuilder::withCapacity(StringKind::U16, CpLength{3U}).takeU16String().capacity() >= U16DataLength{6U});
        REQUIRE(StringBuilder::withCapacity(StringKind::U32, CpLength{3U}).takeU32String().capacity() >= CpLength{3U});
    }

    void testBasedOnAndTemplateConversion() {
        const auto u8Initial = U8String{std::u8string_view{u8"A"}};
        const auto u16Initial = U16String{std::u16string_view{u"β"}};
        const auto u32Initial = U32String{std::u32string_view{U"中"}};

        auto u8Builder = StringBuilder::basedOn(U8StringView{u8Initial}, ByteLength{8U});
        auto u16Builder = StringBuilder::basedOn(U16StringView{u16Initial}, U16DataLength{6U});
        auto u32Builder = StringBuilder::basedOn(U32StringView{u32Initial}, CpLength{4U});

        u8Builder.append(U8StringView{U8String{std::u8string_view{u8"!"}}});
        u16Builder.append(U16StringView{U16String{std::u16string_view{u"!"}}});
        u32Builder.append(U32StringView{U32String{std::u32string_view{U"!"}}});

        REQUIRE_EQUAL(StringConverter{u8Builder.to<U8String>()}.toStdString(), std::string{"A!"});
        REQUIRE_EQUAL(StringConverter{u16Builder.to<U16String>()}.toStdU16String(), std::u16string{u"β!"});
        REQUIRE_EQUAL(StringConverter{u32Builder.to<U32String>()}.toStdU32String(), std::u32string{U"中!"});
        REQUIRE(u8Builder.to<U8String>().capacity() >= ByteLength{9U});
        REQUIRE(u16Builder.to<U16String>().capacity() >= U16DataLength{7U});
        REQUIRE(u32Builder.to<U32String>().capacity() >= CpLength{5U});
    }

    void testAppendCharacters() {
        auto builder = StringBuilder{StringKind::U8};

        builder.append(U'A').append(U'\u00E1').append(U'\U0001F600').append(Char::replacement());
        REQUIRE_EQUAL(builder.length(), CpLength{4U});
        REQUIRE_EQUAL(
            StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"A\u00E1\U0001F600\uFFFD"});

        builder.append(Char{char32_t{0x110000U}});
        REQUIRE_EQUAL(builder.length(), CpLength{4U});
        REQUIRE_EQUAL(
            StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"A\u00E1\U0001F600\uFFFD"});
    }

    void testAppendRepeatedCharactersAndClear() {
        auto builder = StringBuilder{StringKind::U16};

        builder.append(Char{U'x'}, CpLength{3U});
        REQUIRE_EQUAL(builder.length(), CpLength{3U});
        REQUIRE_EQUAL(StringConverter{builder.toU16String()}.toStdU16String(), std::u16string{u"xxx"});

        builder.append(Char{char32_t{0x110000U}}, CpLength{2U});
        REQUIRE_EQUAL(builder.length(), CpLength{3U});
        REQUIRE_EQUAL(StringConverter{builder.toU16String()}.toStdU16String(), std::u16string{u"xxx"});

        builder.clear();
        REQUIRE(builder.isEmpty());
        REQUIRE(builder.length().isZero());
        REQUIRE_EQUAL(builder.kind(), StringKind::U16);
        REQUIRE(builder.toU16String().isEmpty());
    }

    void testAppendViewsToAllKinds() {
        const auto u8Text = U8String{std::u8string_view{u8"Aé"}};
        const auto u16Text = U16String{std::u16string_view{u"β😀"}};
        const auto u32Text = U32String{std::u32string_view{U"中"}};

        for (const auto kind : {StringKind::U8, StringKind::U16, StringKind::U32}) {
            auto builder = StringBuilder{kind};
            builder.append(U8StringView{u8Text});
            builder.append(U16StringView{u16Text});
            builder.append(U32StringView{u32Text});
            builder.append(U8StringView{u8Text}, ElementCount{2U});
            builder.append(U16StringView{u16Text}, ElementCount::zero());
            builder.append(U32StringView{u32Text}, ElementCount{2U});

            REQUIRE_EQUAL(builder.length(), CpLength{11U});
            REQUIRE_EQUAL(StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"Aéβ😀中AéAé中中"});
            REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdU32String(), std::u32string{U"Aéβ😀中AéAé中中"});
            REQUIRE_EQUAL(StringConverter{builder.toU16String()}.toStdU32String(), std::u32string{U"Aéβ😀中AéAé中中"});
        }
    }

    void testAppendLiterals() {
        const auto u8Literal = U8StringLiteral{"Hola"};
        const auto u8Literal8 = U8StringLiteral{u8"Åbo"};
        const auto u16Literal = U16StringLiteral{u"東京"};
        const auto u32Literal = U32StringLiteral{U"λ"};
        auto builder = StringBuilder{StringKind::U32};

        builder.append(u8Literal).append(u8Literal8).append(u16Literal).append(u32Literal);

        REQUIRE_EQUAL(builder.length(), CpLength{10U});
        REQUIRE_EQUAL(StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"HolaÅbo東京λ"});
    }

    void testSameEncodingAppendPreservesExistingBehavior() {
        const auto invalidUtf8 = U8String{std::string_view{th::stdStringFromHex("41 C0 42")}};
        auto u8Builder = StringBuilder{StringKind::U8};

        u8Builder.append(U8StringView{invalidUtf8});
        const auto u8Result = u8Builder.takeU8String();
        REQUIRE_EQUAL(u8Result.length(), ByteLength{3U});
        REQUIRE_EQUAL(StringConverter{u8Result}.toStdU32String(), std::u32string{U"A\uFFFDB"});

        const auto invalidUtf16 = U16String{std::u16string{u'A', char16_t{0xD800U}, u'B'}};
        auto u16Builder = StringBuilder{StringKind::U16};

        u16Builder.append(U16StringView{invalidUtf16});
        const auto u16Result = u16Builder.takeU16String();
        REQUIRE_EQUAL(u16Result.length().toSizeT(), std::size_t{3U});
        REQUIRE(u16Result.charAt(U16DataIndex{1U}).isReplacement());

        const auto invalidUtf32 = U32String{std::u32string{U'A', char32_t{0x110000U}, U'B'}};
        auto u32Builder = StringBuilder{StringKind::U32};

        u32Builder.append(U32StringView{invalidUtf32});
        const auto u32Result = u32Builder.takeU32String();
        REQUIRE_EQUAL(u32Result.length(), CpLength{3U});
        REQUIRE(u32Result.charAt(CpIndex{1U}).isReplacement());
    }

    void testCrossEncodingAppendReplacesMalformedInput() {
        const auto invalidUtf8 = U8String{std::string_view{th::stdStringFromHex("41 C0 42")}};
        auto u16Builder = StringBuilder{StringKind::U16};

        u16Builder.append(U8StringView{invalidUtf8});
        const auto u16Result = u16Builder.takeU16String();
        REQUIRE_EQUAL(StringConverter{u16Result}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{u16Result}.toStdU16String(), std::u16string{u"A\uFFFDB"});

        const auto invalidUtf16 = U16String{std::u16string{u'A', char16_t{0xD800U}, u'B'}};
        auto u8Builder = StringBuilder{StringKind::U8};

        u8Builder.append(U16StringView{invalidUtf16});
        REQUIRE_EQUAL(StringConverter{u8Builder.toU32String()}.toStdU32String(), std::u32string{U"A\uFFFDB"});
    }

    void testToStringKeepsBuilderUsable() {
        auto builder = StringBuilder{StringKind::U8};
        builder.append(U'a');

        const auto first = builder.toU8String();
        builder.append(U'b');

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), std::string{"a"});
        REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"ab"});
        REQUIRE_EQUAL(builder.length(), CpLength{2U});
    }

    void testTakeStringResetsBuilder() {
        auto builder = StringBuilder{StringKind::U32};
        builder.append(U8String{std::u8string_view{u8"Salut"}}).append(U'!');

        const auto result = builder.takeU8String();

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"Salut!"});
        REQUIRE(builder.isEmpty());
        REQUIRE(builder.length().isZero());
        REQUIRE_EQUAL(builder.kind(), StringKind::U32);

        builder.append(U'Z');
        REQUIRE_EQUAL(StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"Z"});
    }

    void testCopiesAreIndependent() {
        auto first = StringBuilder{StringKind::U8};
        first.append(U8String{std::string_view{"ab"}});
        auto second = first;

        second.append(U'c');
        first.append(U'!');

        REQUIRE_EQUAL(StringConverter{first.toU8String()}.toStdString(), std::string{"ab!"});
        REQUIRE_EQUAL(StringConverter{second.toU8String()}.toStdString(), std::string{"abc"});
        REQUIRE_EQUAL(first.length(), CpLength{3U});
        REQUIRE_EQUAL(second.length(), CpLength{3U});
    }
};
