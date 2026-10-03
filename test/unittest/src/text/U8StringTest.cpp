// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/AsciiCategory.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/U16StringEditor.hpp>
#include <erbsland/U32StringEditor.hpp>
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

TESTED_TARGETS(String)
class U8StringTest final : public el::UnitTest {
public:
    void testDefaultViewIsEmpty() {
        const auto string = String{};

        REQUIRE(string.isEmpty());
        REQUIRE_EQUAL(StringConverter{string}.toStdString(), std::string{});
        REQUIRE(StringConverter{string}.toStdU8String().empty());
        REQUIRE_EQUAL(string.length().toSizeT(), std::size_t{0});
        REQUIRE_EQUAL(string.characterLength(), CpCount::zero());
        REQUIRE(string.isEncodingValid());
        REQUIRE_EQUAL(string.indexAt(StringSide::Front), ByteIndex::zero());
        REQUIRE_EQUAL(string.indexAt(StringSide::Back), ByteIndex::zero());
        REQUIRE(string.charAt(StringSide::Front).isNull());
        REQUIRE(string.charAt(StringSide::Back).isNull());
        REQUIRE(string.charAt(string.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(string.slice(StringSide::Front, ByteCount{1U}).isEmpty());
        REQUIRE(string.slice(StringSide::Back, ByteCount{1U}).isEmpty());
    }

    void testConstructionFactoriesAndSwap() {
        static_assert(std::is_constructible_v<String, std::string_view>);
        static_assert(std::is_constructible_v<String, std::u8string_view>);
        static_assert(!std::is_convertible_v<std::string_view, String>);
        static_assert(!std::is_convertible_v<std::u8string_view, String>);

        REQUIRE_EQUAL(String{std::string_view{"narrow"}}, "narrow"_el);
        REQUIRE_EQUAL(String{std::u8string_view{u8"utf-8"}}, u8"utf-8"_el);
        REQUIRE_EQUAL(String::fromCharacter(Char{U'\u20AC'}, CpCount{2U}), u8"€€"_el);
        REQUIRE_EQUAL(String::fromJoined({"one"_el, "-"_el, "two"_el}), "one-two"_el);
        REQUIRE_EQUAL(String::fromInteger(-42), "-42"_el);
        REQUIRE_EQUAL(String::fromFloat(1.25), String{StringEditor::fromFloat(1.25)});
        REQUIRE_EQUAL(String::fromBoolean(true), "true"_el);

        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<std::uint8_t>{0x12U, 0x34U});
        REQUIRE_EQUAL(String::fromByteBlock(bytes), "1234"_el);

        auto first = String{std::string_view{"first"}};
        auto second = String{std::string_view{"second"}};
        swap(first, second);
        REQUIRE_EQUAL(first, "second"_el);
        REQUIRE_EQUAL(second, "first"_el);
    }

    void testViewFromStringKeepsDataAlive() {
        auto text = StringEditor{"Hello"_el};
        const auto string = String{text};

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
        auto text = StringEditor{"Hello"_el};
        auto first = String{text};
        auto second = first;
        auto third = String{std::move(first)};

        REQUIRE_EQUAL(StringConverter{second}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{third}.toStdString(), std::string{"Hello"});
    }

    void testSensitivityFollowsUtf8Storage() {
        auto sharedEditor = StringEditor{"secret"_el};
        auto marked = String{sharedEditor};
        auto alias = marked;

        REQUIRE_FALSE(marked.isSensitive());
        marked.markAsSensitive();
        REQUIRE(marked.isSensitive());
        REQUIRE(alias.isSensitive());
        REQUIRE(sharedEditor.isSensitive());
        auto slice = marked.slice(CpRange{CpIndex{1U}, CpCount{3U}});
        REQUIRE(slice.isSensitive());
        REQUIRE(marked.copy().isSensitive());
        REQUIRE(marked.trimmed().isSensitive());
        REQUIRE(marked.replaced(CpRange{CpIndex{}, CpCount::one()}, "S"_el).isSensitive());
        REQUIRE(marked.toEscaped(EscapeFormat::Json).isSensitive());
        REQUIRE(marked.toSafeString(CpCount{100U}).isSensitive());
        REQUIRE(StringFormat{"{}"_el}.build(marked).isSensitive());
        REQUIRE_FALSE(StringEncoder{marked}.encode(StringEncoding::Utf8).isSensitive());

        auto ordinary = StringEditor{"prefix"_el};
        ordinary.append(marked);
        REQUIRE(ordinary.isSensitive());
        ordinary.insert(ByteIndex{}, marked);
        REQUIRE(ordinary.isSensitive());
        ordinary.replace(ByteRange{ByteIndex{}, ByteCount{1U}}, marked);
        REQUIRE(ordinary.isSensitive());
        REQUIRE(String::fromJoined({"prefix"_el, marked}).isSensitive());

        auto sensitiveEditor = StringEditor{marked};
        REQUIRE(sensitiveEditor.isSensitive());
        sensitiveEditor.append(" suffix"_el);
        REQUIRE(sensitiveEditor.isSensitive());
        sensitiveEditor.clear();
        REQUIRE(sensitiveEditor.isSensitive());
        sensitiveEditor.reset();
        REQUIRE_FALSE(sensitiveEditor.isSensitive());

        auto literal = String{"literal"_el};
        const auto literalAlias = literal;
        literal.markAsSensitive();
        REQUIRE(literal.isSensitive());
        REQUIRE_FALSE(literalAlias.isSensitive());
        auto empty = String{};
        empty.markAsSensitive();
        REQUIRE(empty.isSensitive());
        REQUIRE(empty.copy().isSensitive());
        REQUIRE(marked.replacedAll(marked, ""_el).isSensitive());
        REQUIRE(marked.replacedAll(CharSet{"secret"_el}, ""_el).isSensitive());
        REQUIRE(marked.truncated(CpCount{0U}).isSensitive());
        REQUIRE(marked.slice(CpRange{CpIndex{}, CpCount{0U}}).isSensitive());
        REQUIRE(marked.kept(CpRange{CpIndex{}, CpCount{0U}}).isSensitive());
        auto emptyEditor = StringEditor{};
        emptyEditor.markAsSensitive();
        REQUIRE(emptyEditor.isSensitive());
        REQUIRE(StringEditor::fromJoined({"prefix"_el, marked}).isSensitive());
        REQUIRE(StringEditor::fromJoined({empty}).isSensitive());
        REQUIRE(String{"secret"_el}.replacedAll("secret"_el, marked).isSensitive());
        REQUIRE(String{"secret"_el}.replacedFirst("secret"_el, marked).isSensitive());
        REQUIRE(String{"secret"_el}.replaced(CpRange{CpIndex{}, CpCount{6U}}, marked).isSensitive());
        REQUIRE(String{"secret"_el}.inserted(ByteIndex{}, empty).isSensitive());
        REQUIRE(StringEditor{"secret"_el}.replacedAll("secret"_el, marked).isSensitive());
        REQUIRE(StringEditor{"secret"_el}.replacedFirst("secret"_el, marked).isSensitive());
        REQUIRE(StringEditor{"secret"_el}.replaced(CpRange{CpIndex{}, CpCount{6U}}, marked).isSensitive());
        REQUIRE(StringEditor{"secret"_el}.inserted(ByteIndex{}, empty).isSensitive());
    }

    void testCopyMaterializesString() {

        const auto text = StringEditor{"xHellox"_el};
        const auto string = String{text}.slice({ByteIndex{1U}, ByteCount{5U}});
        const auto copy = string.copy();

        REQUIRE_EQUAL(copy, "Hello"_el);
        REQUIRE_NOT_EQUAL(copy.storageId(), string.storageId());
    }

    void testByteIndexedRead() {
        const auto text = String{"A¢€😀"_el};
        const auto string = String{text};

        REQUIRE_EQUAL(string.length().toSizeT(), std::size_t{10});
        REQUIRE_EQUAL(string.characterLength(), CpCount{4});
        REQUIRE(string.isEncodingValid());
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
            const auto singleText = String{"€"_el};
            const auto singleView = String{singleText};
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
        const auto text = String{"A¢€😀"_el};
        const auto string = String{text};
        auto index = ByteIndex::zero();

        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE(string.advance(index, CpCount{2}));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});
        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
        REQUIRE_FALSE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex{4};
        REQUIRE_FALSE(string.advance(index, CpCount::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        REQUIRE(string.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        index = ByteIndex::zero();
        REQUIRE(string.advance(index, CpCount::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
    }

    void testRetreat() {
        const auto text = String{"A¢€😀"_el};
        const auto string = String{text};
        auto index = ByteIndex{5};

        REQUIRE_FALSE(string.retreat(index, CpCount::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        REQUIRE(string.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        index = ByteIndex{99};
        REQUIRE(string.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});

        // `true` because an actual movement was performed.
        REQUIRE(string.retreat(index, CpCount::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{0});
    }

    void testIndexConversions() {
        const auto text = String{"A¢€😀"_el};
        const auto string = String{text};

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
        const auto text = StringEditor{std::string_view{data}};
        const auto string = String{text};

        REQUIRE_FALSE(string.isEncodingValid());
        REQUIRE_EQUAL(string.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(string.characterLength(), CpCount{3});
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
        const auto text = StringEditor{"abcdef"_el};
        const auto string = String{text};

        REQUIRE_EQUAL(StringConverter{string.slice({ByteIndex{2}, ByteCount{3}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteCount{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteCount{2})}.toStdString(), "ef");
        REQUIRE(string.slice({ByteIndex{2}, ByteCount{0}}).isEmpty());
        REQUIRE(string.slice({ByteIndex{9}, ByteCount{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice({ByteIndex{4}, ByteCount{99}})}.toStdString(), "ef");
        REQUIRE_EQUAL(
            StringConverter{string.slice(ByteRange{ByteIndex{3}, ByteCount::infinite()})}.toStdString(), "def");
        REQUIRE(string.slice(ByteRange::noRange()).isEmpty());
        REQUIRE(string.slice(ByteRange{ByteIndex::noIndex(), ByteCount{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteCount{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Front, ByteCount::infinite())}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Front, ByteCount{0}).isEmpty());
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteCount{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{string.slice(StringSide::Back, ByteCount::infinite())}.toStdString(), "abcdef");
        REQUIRE(string.slice(StringSide::Back, ByteCount{0}).isEmpty());
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

        const auto unicodeText = String{"xxA¢€😀BCyy"_el};
        const auto unicode = String{unicodeText}.slice({ByteIndex{2}, ByteCount{12}});
        const auto charView = unicode;
        const auto range = CpRange{CpIndex{1}, CpCount{3}};

        REQUIRE_EQUAL(StringConverter{unicode.slice(range)}.toStdU32String(), std::u32string{U"¢€\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(range)}.toStdU32String(),
            StringConverter{charView.slice(range)}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpCount{4})}.toStdU32String(),
            std::u32string{U"A¢€\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpCount{3})}.toStdU32String(),
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
            StringConverter{unicode.slice(StringSide::Back, CpCount{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpCount::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpCount::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Front, CpCount::zero()).isEmpty());
        REQUIRE(unicode.slice(StringSide::Back, CpCount::zero()).isEmpty());
        REQUIRE(unicode.slice(CpRange::noRange()).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex::noIndex(), CpCount{1}}).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex{99}, CpCount{1}}).isEmpty());

        const auto nested = unicode.slice(CpRange{CpIndex{1}, CpCount{4}});
        REQUIRE_EQUAL(
            StringConverter{nested.slice(StringSide::Back, CpCount{2})}.toStdU32String(),
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
        const auto text = StringEditor{"abcdef"_el};
        const auto string = String{text};
        const auto first = string.slice({ByteIndex{1}, ByteCount{4}});
        const auto second = first.slice({ByteIndex{1}, ByteCount{2}});

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), "bcde");
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), "cd");
    }

    void testLiteralSlice() {

        const auto string = String{"abcdef"_el};
        const auto slice = string.slice({ByteIndex{2}, ByteCount{3}});

        REQUIRE_EQUAL(StringConverter{slice}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{slice.slice(StringSide::Back, ByteCount{2})}.toStdString(), "de");
    }

    void testSliceThroughUtf8Sequence() {
        const auto text = String{"A¢€"_el};
        const auto string = String{text};
        const auto slice = string.slice({ByteIndex{2}, ByteCount{2}});

        REQUIRE_EQUAL(th::toStdU32String(StringConverter{slice}.toStdString()), std::u32string{U"\uFFFD\uFFFD"});
        REQUIRE_FALSE(slice.isEncodingValid());
        REQUIRE(slice.charAt(ByteIndex::zero()).isReplacement());
    }

    void testPredicateChecks() {

        const auto text = String{"xxA¢€😀yy"_el};
        const auto string = String{text}.slice({ByteIndex{2}, ByteCount{10}});

        REQUIRE(string.startsWith(u8"A¢"_el));
        REQUIRE(string.startsWith(u8"A"_el));
        REQUIRE(string.startsWith(String{}));
        REQUIRE_FALSE(string.startsWith(u8"¢"_el));
        REQUIRE(string.endsWith(u8"😀"_el));
        REQUIRE(string.endsWith(String{}));
        REQUIRE_FALSE(string.endsWith(u8"€"_el));
        REQUIRE(string.contains(u8"¢€"_el));
        REQUIRE(string.contains(u8"€"_el));
        REQUIRE(string.contains(String{}));
        REQUIRE_FALSE(string.contains(u8"€¢"_el));
        REQUIRE_EQUAL(string.count(u8"¢€"_el), ItemCount{1U});
        REQUIRE_EQUAL(string.count(u8"€"_el), ItemCount{1U});
        REQUIRE_EQUAL(string.count(String{}), ItemCount::zero());
        REQUIRE_EQUAL(String{"aaaa"_el}.count("aa"_el), ItemCount{2U});
        REQUIRE(string.containsOneOf(CharSet{u8"z😀"_el}));
        REQUIRE(string.containsOneOf(CharSet{Char{0x20ACU}}));
        REQUIRE_FALSE(string.containsOneOf(CharSet{"xyz"_el}));
        REQUIRE_FALSE(string.containsOneOf(CharSet{}));
        REQUIRE(string.containsOnly(CharSet{u8"A¢€😀"_el}));
        REQUIRE_FALSE(string.containsOnly(CharSet{u8"A¢€"_el}));
        REQUIRE_FALSE(string.containsOnly(CharSet{}));
        REQUIRE(String{"Alpha_09"_el}.containsOnly(AsciiCategory::Word));
        REQUIRE(String{}.containsOnly(AsciiCategory::Word));
        REQUIRE_FALSE(String{"Alpha-09"_el}.containsOnly(AsciiCategory::Word));
        REQUIRE_FALSE(String{"Alphaä"_el}.containsOnly(AsciiCategory::WordWithHyphen));
    }

    void testInvalidUtf8PredicateChecks() {

        const auto data = invalidUtf8Data();
        const auto text = StringEditor{std::string_view{data}};
        const auto string = String{text};

        REQUIRE_FALSE(string.isEncodingValid());
        REQUIRE(string.contains(u8"\uFFFD"_el));
        REQUIRE_EQUAL(string.count(u8"\uFFFD"_el), ItemCount{1U});
        REQUIRE_EQUAL(string.count(u8"\uFFFD"_el, Char::compareCaseFolded), ItemCount{1U});
        REQUIRE(string.containsOneOf(CharSet{Char::replacement()}));
        REQUIRE_FALSE(string.containsOnly(AsciiCategory::Word));
        REQUIRE_FALSE(string.startsWith(u8"\uFFFD"_el));
        REQUIRE_FALSE(string.endsWith(u8"\uFFFD"_el));
    }

    void testForwardFind() {

        const auto text = String{"xxA¢€😀yy"_el};
        const auto string = String{text}.slice({ByteIndex{2}, ByteCount{10}});

        REQUIRE_EQUAL(string.find(u8"¢€"_el), ByteIndex{1U});
        REQUIRE_EQUAL(string.find(String{}, ByteIndex{3U}), ByteIndex{3U});
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

        const auto text = String{"xxA¢€😀yy"_el};
        const auto string = String{text}.slice({ByteIndex{2}, ByteCount{10}});

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
        const auto text = StringEditor{std::string_view{data}};
        const auto string = String{text};

        REQUIRE_EQUAL(string.findFirstOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(string.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
    }

    void testInvalidUtf8ReverseFind() {
        const auto data = invalidUtf8Data();
        const auto text = StringEditor{std::string_view{data}};
        const auto string = String{text};

        REQUIRE_EQUAL(string.findLastOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(string.findLastNotOf(CharSet{Char{0x42U}}), ByteIndex{1U});
    }

    void testStdConversionsFromViewAndSlice() {
        const auto text = String{"xxA¢€😀yy"_el};
        const auto string = String{text}.slice({ByteIndex{2}, ByteCount{10}});

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
        const auto text = StringEditor{std::string_view{invalidUtf8Data()}};
        const auto string = String{text};

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
