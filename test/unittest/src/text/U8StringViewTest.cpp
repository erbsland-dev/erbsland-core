// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringView.hpp>
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
#include <string>
#include <string_view>
#include <utility>

using namespace el::text;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8StringView)
class U8StringViewTest final : public el::UnitTest {
public:
    void testDefaultViewIsEmpty() {
        const auto view = U8StringView{};

        REQUIRE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{});
        REQUIRE(StringConverter{view}.toStdU8String().empty());
        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{0});
        REQUIRE_EQUAL(view.characterLength(), CpLength::zero());
        REQUIRE(view.isValidUtf8());
        REQUIRE_EQUAL(view.indexAt(StringSide::Front), ByteIndex::zero());
        REQUIRE_EQUAL(view.indexAt(StringSide::Back), ByteIndex::zero());
        REQUIRE(view.charAt(StringSide::Front).isNull());
        REQUIRE(view.charAt(StringSide::Back).isNull());
        REQUIRE(view.charAt(view.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(view.slice(StringSide::Front, ByteLength{1U}).isEmpty());
        REQUIRE(view.slice(StringSide::Back, ByteLength{1U}).isEmpty());
    }

    void testViewFromStringKeepsDataAlive() {
        auto text = U8String{std::string_view{"Hello"}};
        const auto view = U8StringView{text};

        text.clear();

        REQUIRE(text.isEmpty());
        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU8String(), std::u8string{u8"Hello"});
    }

    void testViewFromLiteral() {
        using namespace el::text::literals;

        const auto view = "Hello"_elv;

        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
    }

    void testViewFromU8Literal() {
        using namespace el::text::literals;

        const auto view = u8"Hello"_elv;

        REQUIRE_FALSE(view.isEmpty());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), std::string{"Hello"});
    }

    void testCopyAndMoveView() {
        auto text = U8String{std::string_view{"Hello"}};
        auto first = U8StringView{text};
        auto second = first;
        auto third = U8StringView{std::move(first)};

        REQUIRE_EQUAL(StringConverter{second}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{third}.toStdString(), std::string{"Hello"});
    }

    void testCopyMaterializesString() {
        using namespace el::text::literals;

        const auto text = U8String{std::string_view{"xHellox"}};
        const auto view = U8StringView{text}.slice(ByteRange{ByteIndex{1U}, ByteLength{5U}});
        const auto copy = view.copy();

        REQUIRE_EQUAL(copy, "Hello"_el);
        REQUIRE_NOT_EQUAL(copy.storageId(), view.storageId());
    }

    void testByteIndexedRead() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = U8StringView{text};

        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{10});
        REQUIRE_EQUAL(view.characterLength(), CpLength{4});
        REQUIRE(view.isValidUtf8());
        REQUIRE_EQUAL(view.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(view.charAt(StringSide::Back).toRawValue(), U'\U0001F600');
        {
            const auto character = view.charAt(StringSide::Front);
            const auto remaining = view.slice(StringSide::Back, view.length() - character.utf8Size());
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"¢€😀"});
        }
        {
            const auto character = view.charAt(StringSide::Back);
            const auto remaining = view.slice(StringSide::Front, view.length() - character.utf8Size());
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"A¢€"});
        }
        {
            const auto singleText = U8String{std::u8string_view{u8"€"}};
            const auto singleView = U8StringView{singleText};
            const auto character = singleView.charAt(StringSide::Back);
            const auto remaining = singleView.slice(StringSide::Front, singleView.length() - character.utf8Size());
            REQUIRE_EQUAL(character.toRawValue(), U'\u20AC');
            REQUIRE(remaining.isEmpty());
        }
        REQUIRE_EQUAL(view.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(view.charAt(ByteIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(view.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(view.charAt(ByteIndex{6}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(view[ByteIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(view[ByteIndex{6}].toRawValue(), U'\U0001F600');
        REQUIRE(view.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(view.charAt(ByteIndex{11}).isNoCodePoint());
        REQUIRE(view.charAt(ByteIndex::noIndex()).isNoCodePoint());
        REQUIRE(view.charAt(ByteIndex{2}).isReplacement());
        REQUIRE_EQUAL(view.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE(view.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(view.charAt(ByteIndex::noIndex()).isNoCodePoint());
        REQUIRE_EQUAL(view.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(view.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(view.charAt(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(view[CpIndex{2}].toRawValue(), U'\u20AC');
        REQUIRE(view.charAt(CpIndex{4}).isEndOfData());
        REQUIRE(view.charAt(CpIndex{5}).isNoCodePoint());
        REQUIRE(view.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(view[CpIndex{4}].isEndOfData());
        REQUIRE(view[CpIndex::noIndex()].isNoCodePoint());
    }

    void testAdvance() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = U8StringView{text};
        auto index = ByteIndex::zero();

        REQUIRE(view.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE(view.advance(index, CpLength{2}));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});
        REQUIRE(view.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
        REQUIRE_FALSE(view.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex{4};
        REQUIRE_FALSE(view.advance(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        REQUIRE(view.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        index = ByteIndex::zero();
        REQUIRE(view.advance(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
    }

    void testRetreat() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = U8StringView{text};
        auto index = ByteIndex{5};

        REQUIRE_FALSE(view.retreat(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        REQUIRE(view.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        index = ByteIndex{99};
        REQUIRE(view.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});

        // `true` because an actual movement was performed.
        REQUIRE(view.retreat(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{0});
    }

    void testIndexConversions() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = U8StringView{text};

        REQUIRE_EQUAL(view.indexAt(CpIndex{0}), ByteIndex{0});
        REQUIRE_EQUAL(view.indexAt(CpIndex{1}), ByteIndex{1});
        REQUIRE_EQUAL(view.indexAt(CpIndex{2}), ByteIndex{3});
        REQUIRE_EQUAL(view.indexAt(CpIndex{3}), ByteIndex{6});
        REQUIRE_EQUAL(view.indexAt(CpIndex{4}), view.indexAt(StringSide::Back));
        REQUIRE(view.indexAt(CpIndex{5}).isNoIndex());
        REQUIRE(view.indexAt(CpIndex::noIndex()).isNoIndex());

        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{0}), CpIndex{0});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{2}), CpIndex{1});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{3}), CpIndex{2});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{5}), CpIndex{2});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{6}), CpIndex{3});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{9}), CpIndex{3});
        REQUIRE_EQUAL(view.toCharIndex(view.indexAt(StringSide::Back)), CpIndex{4});
        REQUIRE(view.toCharIndex(ByteIndex{11}).isNoIndex());
        REQUIRE(view.toCharIndex(ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8Read() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = U8StringView{text};

        REQUIRE_FALSE(view.isValidUtf8());
        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(view.characterLength(), CpLength{3});
        REQUIRE_EQUAL(view.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE(view.charAt(ByteIndex{1}).isReplacement());
        REQUIRE_EQUAL(view.charAt(ByteIndex{2}).toRawValue(), U'B');
        REQUIRE(view.charAt(ByteIndex{1}).isReplacement());
        REQUIRE_EQUAL(view.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(view.charAt(CpIndex{1}).isReplacement());
        REQUIRE(view[CpIndex{1}].isReplacement());
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'B');

        auto index = ByteIndex{1};
        REQUIRE(view.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(view.indexAt(CpIndex{2}), ByteIndex{2});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(view.toCharIndex(ByteIndex{2}), CpIndex{2});
    }

    void testSlice() {
        const auto text = U8String{std::string_view{"abcdef"}};
        const auto view = U8StringView{text};

        REQUIRE_EQUAL(StringConverter{view.slice(ByteRange{ByteIndex{2}, ByteLength{3}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Front, ByteLength{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, ByteLength{2})}.toStdString(), "ef");
        REQUIRE(view.slice(ByteRange{ByteIndex{2}, ByteLength{0}}).isEmpty());
        REQUIRE(view.slice(ByteRange{ByteIndex{9}, ByteLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{view.slice(ByteRange{ByteIndex{4}, ByteLength{99}})}.toStdString(), "ef");
        REQUIRE_EQUAL(
            StringConverter{view.slice(ByteRange{ByteIndex{3}, ByteLength::infinite()})}.toStdString(), "def");
        REQUIRE(view.slice(ByteRange::noRange()).isEmpty());
        REQUIRE(view.slice(ByteRange{ByteIndex::noIndex(), ByteLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, ByteLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, ByteLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(view.slice(StringSide::Back, ByteLength{0}).isEmpty());

        const auto unicodeText = U8String{std::u8string_view{u8"xxA¢€😀BCyy"}};
        const auto unicode = U8StringView{unicodeText}.slice(ByteRange{ByteIndex{2}, ByteLength{12}});
        const auto charView = unicode.toCharView();
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
            StringConverter{unicode.slice(StringSide::Back, CpLength{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Back, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(CpRange::noRange()).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex{99}, CpLength{1}}).isEmpty());

        const auto nested = unicode.slice(CpRange{CpIndex{1}, CpLength{4}});
        REQUIRE_EQUAL(
            StringConverter{nested.slice(StringSide::Back, CpLength{2})}.toStdU32String(),
            std::u32string{U"\U0001F600B"});
    }

    void testNestedSlice() {
        const auto text = U8String{std::string_view{"abcdef"}};
        const auto view = U8StringView{text};
        const auto first = view.slice(ByteRange{ByteIndex{1}, ByteLength{4}});
        const auto second = first.slice(ByteRange{ByteIndex{1}, ByteLength{2}});

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), "bcde");
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), "cd");
    }

    void testLiteralSlice() {
        using namespace el::text::literals;

        const auto view = "abcdef"_elv;
        const auto slice = view.slice(ByteRange{ByteIndex{2}, ByteLength{3}});

        REQUIRE_EQUAL(StringConverter{slice}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{slice.slice(StringSide::Back, ByteLength{2})}.toStdString(), "de");
    }

    void testSliceThroughUtf8Sequence() {
        const auto text = U8String{std::u8string_view{u8"A¢€"}};
        const auto view = U8StringView{text};
        const auto slice = view.slice(ByteRange{ByteIndex{2}, ByteLength{2}});

        REQUIRE_EQUAL(th::toStdU32String(StringConverter{slice}.toStdString()), std::u32string{U"\uFFFD\uFFFD"});
        REQUIRE_FALSE(slice.isValidUtf8());
        REQUIRE(slice.charAt(ByteIndex::zero()).isReplacement());
    }

    void testPredicateChecks() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = U8StringView{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE(view.startsWith(u8"A¢"_elv));
        REQUIRE(view.startsWith(u8"A"_elv));
        REQUIRE(view.startsWith(U8StringView{}));
        REQUIRE_FALSE(view.startsWith(u8"¢"_elv));
        REQUIRE(view.endsWith(u8"😀"_elv));
        REQUIRE(view.endsWith(U8StringView{}));
        REQUIRE_FALSE(view.endsWith(u8"€"_elv));
        REQUIRE(view.contains(u8"¢€"_elv));
        REQUIRE(view.contains(u8"€"_elv));
        REQUIRE(view.contains(U8StringView{}));
        REQUIRE_FALSE(view.contains(u8"€¢"_elv));
        REQUIRE_EQUAL(view.count(u8"¢€"_elv), ElementCount{1U});
        REQUIRE_EQUAL(view.count(u8"€"_elv), ElementCount{1U});
        REQUIRE_EQUAL(view.count(U8StringView{}), ElementCount::zero());
        REQUIRE_EQUAL(U8StringView{"aaaa"_els}.count("aa"_elv), ElementCount{2U});
        REQUIRE(view.containsOneOf(CharSet{u8"z😀"_elv}));
        REQUIRE(view.containsOneOf(CharSet{Char{0x20ACU}}));
        REQUIRE_FALSE(view.containsOneOf(CharSet{"xyz"_elv}));
        REQUIRE_FALSE(view.containsOneOf(CharSet{}));
        REQUIRE(view.containsOnly(CharSet{u8"A¢€😀"_elv}));
        REQUIRE_FALSE(view.containsOnly(CharSet{u8"A¢€"_elv}));
        REQUIRE_FALSE(view.containsOnly(CharSet{}));
    }

    void testInvalidUtf8PredicateChecks() {
        using namespace el::text::literals;

        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = U8StringView{text};

        REQUIRE_FALSE(view.isValidUtf8());
        REQUIRE(view.contains(u8"\uFFFD"_elv));
        REQUIRE_EQUAL(view.count(u8"\uFFFD"_elv), ElementCount{1U});
        REQUIRE_EQUAL(view.count(u8"\uFFFD"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE(view.containsOneOf(CharSet{Char::replacement()}));
        REQUIRE_FALSE(view.startsWith(u8"\uFFFD"_elv));
        REQUIRE_FALSE(view.endsWith(u8"\uFFFD"_elv));
    }

    void testForwardFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = U8StringView{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(view.find(u8"¢€"_elv), ByteIndex{1U});
        REQUIRE_EQUAL(view.find(U8StringView{}, ByteIndex{3U}), ByteIndex{3U});
        REQUIRE(view.find(u8"€¢"_elv).isNoIndex());
        REQUIRE_EQUAL(view.findFirstOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(view.findFirstOf(CharSet{Char{0x20ACU}}, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(view.findFirstOf(CharSet{u8"z😀"_elv}), ByteIndex{6U});
        REQUIRE(view.findFirstOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{u8"A¢"_elv}), ByteIndex{3U});
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{}), ByteIndex{0U});
        REQUIRE(view.findFirstOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testReverseFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = U8StringView{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(view.findLastOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(view.findLastOf(CharSet{Char{0x20ACU}}, ByteIndex{6U}), ByteIndex{3U});
        REQUIRE_EQUAL(view.findLastOf(CharSet{u8"z😀"_elv}), ByteIndex{6U});
        REQUIRE(view.findLastOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{Char{0x1F600U}}), ByteIndex{3U});
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{u8"€😀"_elv}), ByteIndex{1U});
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{}), ByteIndex{6U});
        REQUIRE(view.findLastOf(CharSet{Char{0x41U}}, ByteIndex::zero()).isNoIndex());
        REQUIRE(view.findLastOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8ForwardFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = U8StringView{text};

        REQUIRE_EQUAL(view.findFirstOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
    }

    void testInvalidUtf8ReverseFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = U8StringView{text};

        REQUIRE_EQUAL(view.findLastOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{Char{0x42U}}), ByteIndex{1U});
    }

    void testStdConversionsFromViewAndSlice() {
        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = U8StringView{text}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{view}.toStdWString()), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{view}.toStringView().storageId(), view.storageId());
        REQUIRE_EQUAL(StringConverter{view}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testStdConversionsFromLiteralView() {
        using namespace el::text::literals;

        const auto view = u8"A¢€😀"_elv;

        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{view}.toStdWString()), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testStdConversionsFromInvalidUtf8View() {
        const auto text = U8String{std::string_view{invalidUtf8Data()}};
        const auto view = U8StringView{text};

        REQUIRE_EQUAL(StringConverter{view}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU8String(), std::u8string{u8"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{view}.toStdWString()), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{view}.toStringView().storageId(), view.storageId());
        REQUIRE_EQUAL(StringConverter{view}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A\uFFFDB"});
    }

private:
    [[nodiscard]] static auto invalidUtf8Data() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }
};
