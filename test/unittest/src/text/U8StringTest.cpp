// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace el::text::literals;

using namespace el::text;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8String)
class U8StringTest final : public el::UnitTest {
public:
    void testDefaultViewIsEmpty() {
        const auto string = U8String{};

        REQUIRE(string.isEmpty());
        REQUIRE_EQUAL(StringConverter{string}.toStdString(), std::string{});
        REQUIRE(StringConverter{string}.toStdU8String().empty());
        REQUIRE_EQUAL(string.length().toSizeT(), std::size_t{0});
        REQUIRE_EQUAL(string.characterLength(), CpLength::zero());
        REQUIRE(string.isValidUtf8());
        REQUIRE_EQUAL(string.indexAt(StringSide::Front), ByteIndex::zero());
        REQUIRE_EQUAL(string.indexAt(StringSide::Back), ByteIndex::zero());
        REQUIRE(string.charAt(StringSide::Front).isNull());
        REQUIRE(string.charAt(StringSide::Back).isNull());
        REQUIRE(string.charAt(string.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(string.slice(StringSide::Front, ByteLength{1U}).isEmpty());
        REQUIRE(string.slice(StringSide::Back, ByteLength{1U}).isEmpty());
    }

    void testConstructionFactoriesAndSwap() {
        static_assert(std::is_constructible_v<U8String, std::string_view>);
        static_assert(std::is_constructible_v<U8String, std::u8string_view>);
        static_assert(!std::is_convertible_v<std::string_view, U8String>);
        static_assert(!std::is_convertible_v<std::u8string_view, U8String>);

        REQUIRE_EQUAL(U8String{std::string_view{"narrow"}}, "narrow"_el);
        REQUIRE_EQUAL(U8String{std::u8string_view{u8"utf-8"}}, u8"utf-8"_el);
        REQUIRE_EQUAL(U8String::fromCharacter(Char{U'\u20AC'}, CpLength{2U}), u8"€€"_el);
        REQUIRE_EQUAL(U8String::fromJoined({"one"_el, "-"_el, "two"_el}), "one-two"_el);
        REQUIRE_EQUAL(U8String::fromInteger(-42), "-42"_el);
        REQUIRE_EQUAL(U8String::fromFloat(1.25), U8String{U8StringEditor::fromFloat(1.25)});
        REQUIRE_EQUAL(U8String::fromBoolean(true), "true"_el);

        const auto bytes = el::mem::ByteBlock{std::vector<std::uint8_t>{0x12U, 0x34U}};
        REQUIRE_EQUAL(U8String::fromByteBlock(bytes), "1234"_el);

        auto first = U8String{std::string_view{"first"}};
        auto second = U8String{std::string_view{"second"}};
        swap(first, second);
        REQUIRE_EQUAL(first, "second"_el);
        REQUIRE_EQUAL(second, "first"_el);
    }

    void testViewFromStringKeepsDataAlive() {
        auto text = U8StringEditor{std::string_view{"Hello"}};
        const auto string = U8String{text};

        text.clear();

        REQUIRE(text.isEmpty());
        REQUIRE_FALSE(string.isEmpty());
        REQUIRE_EQUAL(StringConverter{string}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{string}.toStdU8String(), std::u8string{u8"Hello"});
    }

    void testViewFromLiteral() {

        const auto string = "Hello"_el;

        REQUIRE_FALSE(string.isEmpty());
        REQUIRE_EQUAL(StringConverter{string}.toStdString(), std::string{"Hello"});
    }

    void testViewFromU8Literal() {

        const auto string = u8"Hello"_el;

        REQUIRE_FALSE(string.isEmpty());
        REQUIRE_EQUAL(StringConverter{string}.toStdString(), std::string{"Hello"});
    }

    void testCopyAndMoveView() {
        auto text = U8StringEditor{std::string_view{"Hello"}};
        auto first = U8String{text};
        auto second = first;
        auto third = U8String{std::move(first)};

        REQUIRE_EQUAL(StringConverter{second}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{third}.toStdString(), std::string{"Hello"});
    }

    void testCopyMaterializesString() {

        const auto text = U8StringEditor{std::string_view{"xHellox"}};
        const auto string = U8String{text}.slice(ByteRange{ByteIndex{1U}, ByteLength{5U}});
        const auto copy = string.copy();

        REQUIRE_EQUAL(copy, "Hello"_el);
        REQUIRE_NOT_EQUAL(copy.storageId(), string.storageId());
    }

    void testByteIndexedRead() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        const auto string = U8String{text};

        REQUIRE_EQUAL(string.length().toSizeT(), std::size_t{10});
        REQUIRE_EQUAL(string.characterLength(), CpLength{4});
        REQUIRE(string.isValidUtf8());
        REQUIRE_EQUAL(string.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(string.charAt(StringSide::Back).toRawValue(), U'\U0001F600');
        {
            const auto character = string.charAt(StringSide::Front);
            const auto remaining = string.slice(StringSide::Back, string.length() - character.utf8Size());
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"¢€😀"});
        }
        {
            const auto character = string.charAt(StringSide::Back);
            const auto remaining = string.slice(StringSide::Front, string.length() - character.utf8Size());
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"A¢€"});
        }
        {
            const auto singleText = U8StringEditor{std::u8string_view{u8"€"}};
            const auto singleView = U8String{singleText};
            const auto character = singleView.charAt(StringSide::Back);
            const auto remaining = singleView.slice(StringSide::Front, singleView.length() - character.utf8Size());
            REQUIRE_EQUAL(character.toRawValue(), U'\u20AC');
            REQUIRE(remaining.isEmpty());
        }
        REQUIRE_EQUAL(string.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(string.charAt(ByteIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(string.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(string.charAt(ByteIndex{6}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(string[ByteIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(string[ByteIndex{6}].toRawValue(), U'\U0001F600');
        REQUIRE(string.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(string.charAt(ByteIndex{11}).isNoCodePoint());
        REQUIRE(string.charAt(ByteIndex::noIndex()).isNoCodePoint());
        REQUIRE(string.charAt(ByteIndex{2}).isReplacement());
        REQUIRE_EQUAL(string.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE(string.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(string.charAt(ByteIndex::noIndex()).isNoCodePoint());
        REQUIRE_EQUAL(string.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(string.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(string.charAt(CpIndex{2}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(string.charAt(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(string[CpIndex{2}].toRawValue(), U'\u20AC');
        REQUIRE(string.charAt(CpIndex{4}).isEndOfData());
        REQUIRE(string.charAt(CpIndex{5}).isNoCodePoint());
        REQUIRE(string.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(string[CpIndex{4}].isEndOfData());
        REQUIRE(string[CpIndex::noIndex()].isNoCodePoint());
    }

    void testAdvance() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        const auto string = U8String{text};
        auto index = ByteIndex::zero();

        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE(string.advance(index, CpLength{2}));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});
        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
        REQUIRE_FALSE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex{4};
        REQUIRE_FALSE(string.advance(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        index = ByteIndex::zero();
        REQUIRE(string.advance(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
    }

    void testStrictIndexedSequentialRead() {
        const auto string = U8String{U8StringEditor{std::u8string_view{u8"A¢€😀"}}};
        auto index = ByteIndex::zero();

        REQUIRE_EQUAL(string.readCharAndAdvanceOrThrow(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index, ByteIndex{1U});
        REQUIRE_EQUAL(string.readCharAndAdvanceOrThrow(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index, ByteIndex{3U});
        REQUIRE_EQUAL(string.readCharAndAdvanceOrThrow(index).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(index, ByteIndex{6U});
        REQUIRE_EQUAL(string.readCharAndAdvanceOrThrow(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index, ByteIndex{10U});

        REQUIRE_THROWS_AS(el::err::OutOfRangeError, string.readCharAndAdvanceOrThrow(index));
        REQUIRE_EQUAL(index, ByteIndex{10U});
        index = ByteIndex::noIndex();
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, string.readCharAndAdvanceOrThrow(index));
        REQUIRE(index.isNoIndex());

        const auto invalid = U8String{U8StringEditor{std::string_view{invalidUtf8Data()}}};
        index = ByteIndex{1U};
        REQUIRE_THROWS_AS(EncodingError, invalid.readCharAndAdvanceOrThrow(index));
        REQUIRE_EQUAL(index, ByteIndex{1U});
    }

    void testRetreat() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        const auto string = U8String{text};
        auto index = ByteIndex{5};

        REQUIRE_FALSE(string.retreat(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        REQUIRE(string.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        index = ByteIndex{99};
        REQUIRE(string.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});

        // `true` because an actual movement was performed.
        REQUIRE(string.retreat(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{0});
    }

    void testIndexConversions() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        const auto string = U8String{text};

        REQUIRE_EQUAL(string.indexAt(CpIndex{0}), ByteIndex{0});
        REQUIRE_EQUAL(string.indexAt(CpIndex{1}), ByteIndex{1});
        REQUIRE_EQUAL(string.indexAt(CpIndex{2}), ByteIndex{3});
        REQUIRE_EQUAL(string.indexAt(CpIndex{3}), ByteIndex{6});
        REQUIRE_EQUAL(string.indexAt(CpIndex{4}), string.indexAt(StringSide::Back));
        REQUIRE(string.indexAt(CpIndex{5}).isNoIndex());
        REQUIRE(string.indexAt(CpIndex::noIndex()).isNoIndex());

        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{0}), CpIndex{0});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{2}), CpIndex{1});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{3}), CpIndex{2});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{5}), CpIndex{2});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{6}), CpIndex{3});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{9}), CpIndex{3});
        REQUIRE_EQUAL(string.toCharIndex(string.indexAt(StringSide::Back)), CpIndex{4});
        REQUIRE(string.toCharIndex(ByteIndex{11}).isNoIndex());
        REQUIRE(string.toCharIndex(ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8Read() {
        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};
        const auto string = U8String{text};

        REQUIRE_FALSE(string.isValidUtf8());
        REQUIRE_EQUAL(string.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(string.characterLength(), CpLength{3});
        REQUIRE_EQUAL(string.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE(string.charAt(ByteIndex{1}).isReplacement());
        REQUIRE_EQUAL(string.charAt(ByteIndex{2}).toRawValue(), U'B');
        REQUIRE(string.charAt(ByteIndex{1}).isReplacement());
        REQUIRE_EQUAL(string.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(string.charAt(CpIndex{1}).isReplacement());
        REQUIRE(string[CpIndex{1}].isReplacement());
        REQUIRE_EQUAL(string.charAt(CpIndex{2}).toRawValue(), U'B');

        auto index = ByteIndex{1};
        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(string.indexAt(CpIndex{2}), ByteIndex{2});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(string.toCharIndex(ByteIndex{2}), CpIndex{2});
    }

    void testSlice() {
        const auto text = U8StringEditor{std::string_view{"abcdef"}};
        const auto string = U8String{text};

        REQUIRE_EQUAL(StringConverter{string.slice(ByteRange{ByteIndex{2}, ByteLength{3}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteLength{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteLength{2})}.toStdString(), "ef");
        REQUIRE(string.slice(ByteRange{ByteIndex{2}, ByteLength{0}}).isEmpty());
        REQUIRE(string.slice(ByteRange{ByteIndex{9}, ByteLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(ByteRange{ByteIndex{4}, ByteLength{99}})}.toStdString(), "ef");
        REQUIRE_EQUAL(
            StringConverter{string.slice(ByteRange{ByteIndex{3}, ByteLength::infinite()})}.toStdString(), "def");
        REQUIRE(string.slice(ByteRange::noRange()).isEmpty());
        REQUIRE(string.slice(ByteRange{ByteIndex::noIndex(), ByteLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Front, ByteLength{0}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Back, ByteLength{0}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteIndex{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteIndex{2})}.toStdString(), "cdef");
        REQUIRE(string.slice(StringSide::Front, ByteIndex::zero()).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteIndex::zero())}.toStdString(), "abcdef");
        REQUIRE_EQUAL(
            StringConverter{string.slice(StringSide::Front, string.indexAt(StringSide::Back))}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Back, string.indexAt(StringSide::Back)).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteIndex::noIndex())}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Back, ByteIndex::noIndex()).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteIndex{99})}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Back, ByteIndex{99}).isEmpty());

        const auto unicodeText = U8StringEditor{std::u8string_view{u8"xxA¢€😀BCyy"}};
        const auto unicode = U8String{unicodeText}.slice(ByteRange{ByteIndex{2}, ByteLength{12}});
        const auto charView = unicode;
        const auto range = CpRange{CpIndex{1}, CpLength{3}};

        REQUIRE_EQUAL(StringConverter{unicode.slice(range)}.toStdU32String(), std::u32string{U"¢€\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(range)}.toStdU32String(),
            StringConverter{charView.slice(range)}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpLength{4})}.toStdU32String(),
            std::u32string{U"A¢€\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, ByteIndex{3})}.toStdU32String(),
            std::u32string{U"A\u00A2"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, ByteIndex{3})}.toStdU32String(),
            std::u32string{U"\u20AC\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpIndex{3})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpIndex{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Front, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(StringSide::Back, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(CpRange::noRange()).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex::noIndex(), CpLength{1}}).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex{99}, CpLength{1}}).isEmpty());

        const auto nested = unicode.slice(CpRange{CpIndex{1}, CpLength{4}});
        REQUIRE_EQUAL(
            StringConverter{nested.slice(StringSide::Back, CpLength{2})}.toStdU32String(),
            std::u32string{U"\U0001F600B"});

        {
            const auto [left, right] = string.splitAt(ByteIndex{2});
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "ab");
            REQUIRE_EQUAL(StringConverter{right}.toStdString(), "cdef");
        }
        {
            const auto [left, right] = string.splitAt(ByteIndex::zero());
            REQUIRE(left.isEmpty());
            REQUIRE_EQUAL(StringConverter{right}.toStdString(), "abcdef");
        }
        {
            const auto [left, right] = string.splitAt(string.indexAt(StringSide::Back));
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "abcdef");
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = string.splitAt(ByteIndex::noIndex());
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "abcdef");
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = string.splitAt(ByteIndex{99});
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "abcdef");
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = unicode.splitAt(ByteIndex{3});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), std::u32string{U"A¢"});
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), std::u32string{U"€\U0001F600BC"});
        }
        {
            const auto [left, right] = unicode.splitAt(CpIndex{3});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), std::u32string{U"A¢€"});
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), std::u32string{U"\U0001F600BC"});
        }
    }

    void testNestedSlice() {
        const auto text = U8StringEditor{std::string_view{"abcdef"}};
        const auto string = U8String{text};
        const auto first = string.slice(ByteRange{ByteIndex{1}, ByteLength{4}});
        const auto second = first.slice(ByteRange{ByteIndex{1}, ByteLength{2}});

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), "bcde");
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), "cd");
    }

    void testLiteralSlice() {

        const auto string = U8String{"abcdef"_el};
        const auto slice = string.slice(ByteRange{ByteIndex{2}, ByteLength{3}});

        REQUIRE_EQUAL(StringConverter{slice}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{slice.slice(StringSide::Back, ByteLength{2})}.toStdString(), "de");
    }

    void testSliceThroughUtf8Sequence() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€"}};
        const auto string = U8String{text};
        const auto slice = string.slice(ByteRange{ByteIndex{2}, ByteLength{2}});

        REQUIRE_EQUAL(th::toStdU32String(StringConverter{slice}.toStdString()), std::u32string{U"\uFFFD\uFFFD"});
        REQUIRE_FALSE(slice.isValidUtf8());
        REQUIRE(slice.charAt(ByteIndex::zero()).isReplacement());
    }

    void testPredicateChecks() {

        const auto text = U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto string = U8String{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE(string.startsWith(u8"A¢"_el));
        REQUIRE(string.startsWith(u8"A"_el));
        REQUIRE(string.startsWith(U8String{}));
        REQUIRE_FALSE(string.startsWith(u8"¢"_el));
        REQUIRE(string.endsWith(u8"😀"_el));
        REQUIRE(string.endsWith(U8String{}));
        REQUIRE_FALSE(string.endsWith(u8"€"_el));
        REQUIRE(string.contains(u8"¢€"_el));
        REQUIRE(string.contains(u8"€"_el));
        REQUIRE(string.contains(U8String{}));
        REQUIRE_FALSE(string.contains(u8"€¢"_el));
        REQUIRE_EQUAL(string.count(u8"¢€"_el), ElementCount{1U});
        REQUIRE_EQUAL(string.count(u8"€"_el), ElementCount{1U});
        REQUIRE_EQUAL(string.count(U8String{}), ElementCount::zero());
        REQUIRE_EQUAL(U8String{"aaaa"_el}.count("aa"_el), ElementCount{2U});
        REQUIRE(string.containsOneOf(CharSet{u8"z😀"_el}));
        REQUIRE(string.containsOneOf(CharSet{Char{0x20ACU}}));
        REQUIRE_FALSE(string.containsOneOf(CharSet{"xyz"_el}));
        REQUIRE_FALSE(string.containsOneOf(CharSet{}));
        REQUIRE(string.containsOnly(CharSet{u8"A¢€😀"_el}));
        REQUIRE_FALSE(string.containsOnly(CharSet{u8"A¢€"_el}));
        REQUIRE_FALSE(string.containsOnly(CharSet{}));
    }

    void testInvalidUtf8PredicateChecks() {

        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};
        const auto string = U8String{text};

        REQUIRE_FALSE(string.isValidUtf8());
        REQUIRE(string.contains(u8"\uFFFD"_el));
        REQUIRE_EQUAL(string.count(u8"\uFFFD"_el), ElementCount{1U});
        REQUIRE_EQUAL(string.count(u8"\uFFFD"_el, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE(string.containsOneOf(CharSet{Char::replacement()}));
        REQUIRE_FALSE(string.startsWith(u8"\uFFFD"_el));
        REQUIRE_FALSE(string.endsWith(u8"\uFFFD"_el));
    }

    void testForwardFind() {

        const auto text = U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto string = U8String{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(string.find(u8"¢€"_el), ByteIndex{1U});
        REQUIRE_EQUAL(string.find(U8String{}, ByteIndex{3U}), ByteIndex{3U});
        REQUIRE(string.find(u8"A"_el, ByteIndex::noIndex()).isNoIndex());
        REQUIRE(string.find(u8"€¢"_el).isNoIndex());
        REQUIRE_EQUAL(string.findFirstOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(string.findFirstOf(CharSet{Char{0x20ACU}}, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(string.findFirstOf(CharSet{u8"z😀"_el}), ByteIndex{6U});
        REQUIRE(string.findFirstOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(string.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
        REQUIRE_EQUAL(string.findFirstNotOf(CharSet{u8"A¢"_el}), ByteIndex{3U});
        REQUIRE_EQUAL(string.findFirstNotOf(CharSet{}), ByteIndex{0U});
        REQUIRE(string.findFirstOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
        REQUIRE(string.findFirstNotOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testReverseFind() {

        const auto text = U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto string = U8String{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(string.findLastOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(string.findLastOf(CharSet{Char{0x20ACU}}, ByteIndex{6U}), ByteIndex{3U});
        REQUIRE_EQUAL(string.findLastOf(CharSet{u8"z😀"_el}), ByteIndex{6U});
        REQUIRE(string.findLastOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(string.findLastNotOf(CharSet{Char{0x1F600U}}), ByteIndex{3U});
        REQUIRE_EQUAL(string.findLastNotOf(CharSet{u8"€😀"_el}), ByteIndex{1U});
        REQUIRE_EQUAL(string.findLastNotOf(CharSet{}), ByteIndex{6U});
        REQUIRE(string.findLastOf(CharSet{Char{0x41U}}, ByteIndex::zero()).isNoIndex());
        REQUIRE(string.findLastOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
        REQUIRE(string.findLastNotOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8ForwardFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};
        const auto string = U8String{text};

        REQUIRE_EQUAL(string.findFirstOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(string.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
    }

    void testInvalidUtf8ReverseFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};
        const auto string = U8String{text};

        REQUIRE_EQUAL(string.findLastOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(string.findLastNotOf(CharSet{Char{0x42U}}), ByteIndex{1U});
    }

    void testStdConversionsFromViewAndSlice() {
        const auto text = U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto string = U8String{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(StringConverter{string}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{string}.toStdWString()), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{string}.toString().storageId(), string.storageId());
        REQUIRE_EQUAL(StringConverter{string}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{string}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testStdConversionsFromLiteralView() {

        const auto string = u8"A¢€😀"_el;

        REQUIRE_EQUAL(StringConverter{string}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{string}.toStdWString()), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{string}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{string}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testStdConversionsFromInvalidUtf8View() {
        const auto text = U8StringEditor{std::string_view{invalidUtf8Data()}};
        const auto string = U8String{text};

        REQUIRE_EQUAL(StringConverter{string}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU8String(), std::u8string{u8"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{string}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{string}.toStdWString()), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{string}.toString().storageId(), string.storageId());
        REQUIRE_EQUAL(StringConverter{string}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{string}.toStdU32String(), std::u32string{U"A\uFFFDB"});
    }

private:
    [[nodiscard]] static auto invalidUtf8Data() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }
};
