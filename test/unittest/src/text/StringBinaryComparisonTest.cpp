// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/AnyString.hpp>
#include <erbsland/AnyStringEditor.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/U16String.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/U32StringEditor.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <bit>
#include <string_view>

using namespace el::text::literals;

TESTED_TARGETS(U8String U8StringEditor U16String U16StringEditor U32String U32StringEditor AnyString AnyStringEditor)
class StringBinaryComparisonTest final : public el::UnitTest {
public:
    void testEveryWidthAndEditor() {
        WITH_CONTEXT((requireBasic<el::U8String, el::U8StringEditor>("abc"_el, "abd"_el, "abcd"_el)));
        WITH_CONTEXT((requireBasic<el::U16String, el::U16StringEditor>(u"abc"_el, u"abd"_el, u"abcd"_el)));
        WITH_CONTEXT((requireBasic<el::U32String, el::U32StringEditor>(U"abc"_el, U"abd"_el, U"abcd"_el)));
        REQUIRE_EQUAL(el::String{"abc"_el}.compareBinary("abc"_el), std::strong_ordering::equal);
        REQUIRE_EQUAL(el::StringEditor{"abc"_el}.compareBinary("abc"_el), std::strong_ordering::equal);
    }

    void testVisibleRangesAndEmbeddedNulls() {
        const auto range = el::CpRange{el::CpIndex{1}, el::CpCount{3}};
        REQUIRE_EQUAL(el::U8String{"xabcx"_el}.slice(range).compareBinary("abc"_el), std::strong_ordering::equal);
        REQUIRE_EQUAL(el::U16String{u"xabcx"_el}.slice(range).compareBinary(u"abc"_el), std::strong_ordering::equal);
        REQUIRE_EQUAL(el::U32String{U"xabcx"_el}.slice(range).compareBinary(U"abc"_el), std::strong_ordering::equal);
        WITH_CONTEXT((requireBasic<el::U8String, el::U8StringEditor>("a\0b"_el, "a\0c"_el, "a\0bd"_el)));
        WITH_CONTEXT((requireBasic<el::U16String, el::U16StringEditor>(u"a\0b"_el, u"a\0c"_el, u"a\0bd"_el)));
        WITH_CONTEXT((requireBasic<el::U32String, el::U32StringEditor>(U"a\0b"_el, U"a\0c"_el, U"a\0bd"_el)));
        auto editor = el::StringEditor{"abc"_el};
        editor.reserve(el::ByteCount{1024});
        REQUIRE_EQUAL(editor.compareBinary("abc"_el), std::strong_ordering::equal);
        const auto shared = el::String{editor};
        REQUIRE_EQUAL(
            shared.slice(el::StringSide::Front, el::ByteCount{2}).compareBinary(shared), std::strong_ordering::less);
    }

    void testMalformedDataAndUnsignedBytes() {
        const auto first = el::String{erbsland::unittest::th::stdStringFromHex("80")};
        const auto second = el::String{erbsland::unittest::th::stdStringFromHex("81")};
        REQUIRE_EQUAL(first.compare(second), std::strong_ordering::equal);
        REQUIRE_EQUAL(first.toHash(), second.toHash());
        REQUIRE_LESS(first.compareBinary(second), 0);
        REQUIRE_GREATER(first.compareBinary("z"_el), 0);
        const char16_t left16[]{0xd800};
        const char16_t right16[]{0xd801};
        const auto a16 = el::U16String{std::u16string_view{left16, 1}};
        const auto b16 = el::U16String{std::u16string_view{right16, 1}};
        REQUIRE_EQUAL(a16.compare(b16), std::strong_ordering::equal);
        REQUIRE_LESS(a16.compareBinary(b16), 0);
        const char32_t left32[]{0x110000};
        const char32_t right32[]{0x110001};
        const auto a32 = el::U32String{std::u32string_view{left32, 1}};
        const auto b32 = el::U32String{std::u32string_view{right32, 1}};
        REQUIRE_EQUAL(a32.compare(b32), std::strong_ordering::equal);
        REQUIRE_LESS(a32.compareBinary(b32), 0);
        REQUIRE_LESS(el::U16StringEditor{a16}.compareBinary(b16), 0);
        REQUIRE_LESS(el::U32StringEditor{a32}.compareBinary(b32), 0);
    }

    void testNativeByteOrderAndLongPrefixes() {
        auto expected = std::strong_ordering::less;
        if constexpr (std::endian::native == std::endian::little) {
            expected = std::strong_ordering::greater;
        }
        REQUIRE_EQUAL(el::U16String{u"ÿ"_el}.compareBinary(u"Ā"_el), expected);
        REQUIRE_EQUAL(el::U32String{U"ÿ"_el}.compareBinary(U"Ā"_el), expected);
        const auto prefix = el::String::fromCharacter(el::Char{U'a'}, el::CpCount{4096});
        const auto left = el::join(prefix, "b"_el);
        const auto right = el::join(prefix, "c"_el);
        REQUIRE_LESS(left.compareBinary(right), 0);
        auto sensitive = el::StringEditor{left};
        sensitive.markAsSensitive();
        REQUIRE_LESS(sensitive.compareBinary(right), 0);
        REQUIRE(sensitive.isSensitive());
    }

    void testAnyEncodingAndTypedEmpties() {
        const auto u8 = el::AnyString{"same"_el};
        const auto u16 = el::AnyString{u"same"_el};
        const auto u32 = el::AnyString{U"same"_el};
        REQUIRE_EQUAL(u8.compare(u16), std::strong_ordering::equal);
        REQUIRE_LESS(u8.compareBinary(u16), 0);
        REQUIRE_LESS(u16.compareBinary(u32), 0);
        REQUIRE_GREATER(u32.compareBinary(u8), 0);
        REQUIRE_LESS(el::AnyString{u"a"_el}.compareBinary(el::AnyString{u"b"_el}), 0);
        REQUIRE_EQUAL(
            el::AnyStringEditor{el::U32StringEditor{U"same"_el}}.compareBinary(u32), std::strong_ordering::equal);
        WITH_CONTEXT((requireEmpty<el::U8String, el::U8StringEditor>()));
        WITH_CONTEXT((requireEmpty<el::U16String, el::U16StringEditor>()));
        WITH_CONTEXT((requireEmpty<el::U32String, el::U32StringEditor>()));
    }

private:
    template <typename String, typename Editor>
    void requireBasic(const String &left, const String &right, const String &longer) {
        REQUIRE_LESS(left.compareBinary(right), 0);
        REQUIRE_GREATER(right.compareBinary(left), 0);
        REQUIRE_LESS(left.compareBinary(longer), 0);
        REQUIRE_EQUAL(left.compareBinary(left), std::strong_ordering::equal);
        const auto separate = String{Editor{left}};
        REQUIRE_EQUAL(left.compareBinary(separate), std::strong_ordering::equal);
        REQUIRE_LESS(Editor{left}.compareBinary(right), 0);
        REQUIRE_EQUAL(Editor{left}.compareBinary(separate), std::strong_ordering::equal);
        REQUIRE_LESS(String{}.compareBinary(left), 0);
        REQUIRE_GREATER(left.compareBinary(String{}), 0);
        REQUIRE_EQUAL(Editor{}.compareBinary(String{}), std::strong_ordering::equal);
    }

    template <typename String, typename Editor>
    void requireEmpty() {
        auto assigned = el::AnyString{"previous"_el};
        assigned = String{};
        REQUIRE_EQUAL(assigned.compareBinary(el::AnyString{}), std::strong_ordering::equal);
        REQUIRE_EQUAL(el::AnyString{}.compareBinary(assigned), std::strong_ordering::equal);
        REQUIRE_LESS(assigned.compareBinary(el::AnyString{"a"_el}), 0);
        auto editor = el::AnyStringEditor{el::StringEditor{"previous"_el}};
        editor = Editor{};
        REQUIRE_EQUAL(editor.compareBinary(assigned), std::strong_ordering::equal);
        REQUIRE_EQUAL(assigned.compareBinary(editor), std::strong_ordering::equal);
    }
};
