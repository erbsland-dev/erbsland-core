// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u16/U16StringLiteral.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u32/U32StringLiteral.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/text/u8/U8StringLiteral.hpp>
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
#include <type_traits>
#include <utility>

using el::unit::ByteLength;
using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::ElementCount;
using el::unit::U16DataIndex;
using el::unit::U16DataLength;
using namespace el::text;

namespace th = erbsland::unittest::th;

static_assert(std::is_same_v<String, U8String>);
static_assert(std::is_same_v<StringEditor, U8StringEditor>);
static_assert(std::is_same_v<decltype(std::declval<const AnyStringBuilder &>().toString()), String>);
static_assert(std::is_same_v<decltype(std::declval<const AnyStringBuilder &>().toAnyString()), AnyString>);
static_assert(std::is_same_v<decltype(std::declval<AnyStringBuilder &>().takeString()), String>);
static_assert(std::is_same_v<decltype(std::declval<const AnyStringBuilder &>().toStringEditor()), StringEditor>);
static_assert(std::is_same_v<decltype(std::declval<AnyStringBuilder &>().takeStringEditor()), StringEditor>);

TESTED_TARGETS(AnyStringBuilder StringKind AnyStringBuilderBase U8StringBuilder U16StringBuilder U32StringBuilder)
class AnyStringBuilderTest final : public el::UnitTest {
public:
    void testConstruction() {
        const auto defaultBuilder = AnyStringBuilder{};
        const auto u8Builder = AnyStringBuilder{StringKind::U8};
        const auto u16Builder = AnyStringBuilder{StringKind::U16};
        const auto u32Builder = AnyStringBuilder{StringKind::U32};
        const auto u8FactoryBuilder = AnyStringBuilder::u8();
        const auto u16FactoryBuilder = AnyStringBuilder::u16();
        const auto u32FactoryBuilder = AnyStringBuilder::u32();
        const auto u8CapacityBuilder = AnyStringBuilder::withCapacity(StringKind::U8, CpLength{1U});
        const auto u16CapacityBuilder = AnyStringBuilder::withCapacity(StringKind::U16, CpLength{1U});
        const auto u32CapacityBuilder = AnyStringBuilder::withCapacity(StringKind::U32, CpLength{1U});

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

        REQUIRE(AnyStringBuilder::u8(u8Capacity).takeU8StringEditor().capacity() >= u8Capacity);
        REQUIRE(AnyStringBuilder::u16(u16Capacity).takeU16StringEditor().capacity() >= u16Capacity);
        REQUIRE(AnyStringBuilder::u32(u32Capacity).takeU32StringEditor().capacity() >= u32Capacity);
    }

    void testGenericCapacityFactory() {
        REQUIRE(
            AnyStringBuilder::withCapacity(StringKind::U8, CpLength{3U}).takeU8StringEditor().capacity() >=
            ByteLength{12U});
        REQUIRE(
            AnyStringBuilder::withCapacity(StringKind::U16, CpLength{3U}).takeU16StringEditor().capacity() >=
            U16DataLength{6U});
        REQUIRE(
            AnyStringBuilder::withCapacity(StringKind::U32, CpLength{3U}).takeU32StringEditor().capacity() >=
            CpLength{3U});
    }

    void testBasedOnAndTemplateConversion() {
        const auto u8Initial = U8StringEditor{std::u8string_view{u8"A"}};
        const auto u16Initial = U16StringEditor{std::u16string_view{u"β"}};
        const auto u32Initial = U32StringEditor{std::u32string_view{U"中"}};

        auto u8Builder = AnyStringBuilder::basedOn(U8String{u8Initial}, ByteLength{8U});
        auto u16Builder = AnyStringBuilder::basedOn(U16String{u16Initial}, U16DataLength{6U});
        auto u32Builder = AnyStringBuilder::basedOn(U32String{u32Initial}, CpLength{4U});

        u8Builder.append(U8String{U8StringEditor{std::u8string_view{u8"!"}}});
        u16Builder.append(U16String{U16StringEditor{std::u16string_view{u"!"}}});
        u32Builder.append(U32String{U32StringEditor{std::u32string_view{U"!"}}});

        REQUIRE_EQUAL(StringConverter{u8Builder.toEditor<U8StringEditor>()}.toStdString(), std::string{"A!"});
        REQUIRE_EQUAL(StringConverter{u16Builder.toEditor<U16StringEditor>()}.toStdU16String(), std::u16string{u"β!"});
        REQUIRE_EQUAL(StringConverter{u32Builder.toEditor<U32StringEditor>()}.toStdU32String(), std::u32string{U"中!"});
        REQUIRE(u8Builder.toEditor<U8StringEditor>().capacity() >= ByteLength{9U});
        REQUIRE(u16Builder.toEditor<U16StringEditor>().capacity() >= U16DataLength{7U});
        REQUIRE(u32Builder.toEditor<U32StringEditor>().capacity() >= CpLength{5U});
    }

    void testAppendCharacters() {
        auto builder = AnyStringBuilder{StringKind::U8};

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
        auto builder = AnyStringBuilder{StringKind::U16};

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
        const auto u8Text = U8StringEditor{std::u8string_view{u8"Aé"}};
        const auto u16Text = U16StringEditor{std::u16string_view{u"β😀"}};
        const auto u32Text = U32StringEditor{std::u32string_view{U"中"}};

        for (const auto kind : {StringKind::U8, StringKind::U16, StringKind::U32}) {
            auto builder = AnyStringBuilder{kind};
            builder.append(U8String{u8Text});
            builder.append(U16String{u16Text});
            builder.append(U32String{u32Text});
            builder.append(U8String{u8Text}, ElementCount{2U});
            builder.append(U16String{u16Text}, ElementCount::zero());
            builder.append(U32String{u32Text}, ElementCount{2U});

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
        auto builder = AnyStringBuilder{StringKind::U32};

        builder.append(u8Literal).append(u8Literal8).append(u16Literal).append(u32Literal);

        REQUIRE_EQUAL(builder.length(), CpLength{10U});
        REQUIRE_EQUAL(StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"HolaÅbo東京λ"});
    }

    void testSameEncodingAppendPreservesExistingBehavior() {
        const auto invalidUtf8 = U8StringEditor{std::string_view{th::stdStringFromHex("41 C0 42")}};
        auto u8Builder = AnyStringBuilder{StringKind::U8};

        u8Builder.append(U8String{invalidUtf8});
        const auto u8Result = u8Builder.takeU8String();
        REQUIRE_EQUAL(u8Result.length(), ByteLength{3U});
        REQUIRE_EQUAL(StringConverter{u8Result}.toStdU32String(), std::u32string{U"A\uFFFDB"});

        const auto invalidUtf16 = U16StringEditor{std::u16string{u'A', char16_t{0xD800U}, u'B'}};
        auto u16Builder = AnyStringBuilder{StringKind::U16};

        u16Builder.append(U16String{invalidUtf16});
        const auto u16Result = u16Builder.takeU16String();
        REQUIRE_EQUAL(u16Result.length().toSizeT(), std::size_t{3U});
        REQUIRE(u16Result.charAt(U16DataIndex{1U}).isReplacement());

        const auto invalidUtf32 = U32StringEditor{std::u32string{U'A', char32_t{0x110000U}, U'B'}};
        auto u32Builder = AnyStringBuilder{StringKind::U32};

        u32Builder.append(U32String{invalidUtf32});
        const auto u32Result = u32Builder.takeU32String();
        REQUIRE_EQUAL(u32Result.length(), CpLength{3U});
        REQUIRE(u32Result.charAt(CpIndex{1U}).isReplacement());
    }

    void testCrossEncodingAppendReplacesMalformedInput() {
        const auto invalidUtf8 = U8StringEditor{std::string_view{th::stdStringFromHex("41 C0 42")}};
        auto u16Builder = AnyStringBuilder{StringKind::U16};

        u16Builder.append(U8String{invalidUtf8});
        const auto u16Result = u16Builder.takeU16String();
        REQUIRE_EQUAL(StringConverter{u16Result}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{u16Result}.toStdU16String(), std::u16string{u"A\uFFFDB"});

        const auto invalidUtf16 = U16StringEditor{std::u16string{u'A', char16_t{0xD800U}, u'B'}};
        auto u8Builder = AnyStringBuilder{StringKind::U8};

        u8Builder.append(U16String{invalidUtf16});
        REQUIRE_EQUAL(StringConverter{u8Builder.toU32String()}.toStdU32String(), std::u32string{U"A\uFFFDB"});
    }

    void testToStringKeepsBuilderUsable() {
        auto builder = AnyStringBuilder{StringKind::U8};
        builder.append(U'a');

        const auto first = builder.toU8String();
        builder.append(U'b');

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), std::string{"a"});
        REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"ab"});
        REQUIRE_EQUAL(builder.length(), CpLength{2U});
    }

    void testTakeStringResetsBuilder() {
        auto builder = AnyStringBuilder{StringKind::U32};
        builder.append(U8StringEditor{std::u8string_view{u8"Salut"}}).append(U'!');

        const auto result = builder.takeU8String();

        REQUIRE_EQUAL(StringConverter{result}.toStdString(), std::string{"Salut!"});
        REQUIRE(builder.isEmpty());
        REQUIRE(builder.length().isZero());
        REQUIRE_EQUAL(builder.kind(), StringKind::U32);

        builder.append(U'Z');
        REQUIRE_EQUAL(StringConverter{builder.toU32String()}.toStdU32String(), std::u32string{U"Z"});
    }

    void testCopiesAreIndependent() {
        auto first = AnyStringBuilder{StringKind::U8};
        first.append(U8StringEditor{std::string_view{"ab"}});
        auto second = first;

        second.append(U'c');
        first.append(U'!');

        REQUIRE_EQUAL(StringConverter{first.toU8String()}.toStdString(), std::string{"ab!"});
        REQUIRE_EQUAL(StringConverter{second.toU8String()}.toStdString(), std::string{"abc"});
        REQUIRE_EQUAL(first.length(), CpLength{3U});
        REQUIRE_EQUAL(second.length(), CpLength{3U});
    }
};
