// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringCharView.hpp>
#include <erbsland/text/u8/U8StringView.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <string>
#include <string_view>

using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::CpRange;
using namespace el::text;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8StringCharView)
class U8StringCharViewTest final : public el::UnitTest {
public:
    void testEmptyView() {
        const auto text = U8String{};
        const auto view = text.toCharView();

        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{0});
        REQUIRE(view.charAt(StringSide::Front).isNull());
        REQUIRE(view.charAt(StringSide::Back).isNull());
        REQUIRE(view.charAt(view.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(view.slice(StringSide::Front, CpLength{1U}).isEmpty());
        REQUIRE(view.slice(StringSide::Back, CpLength{1U}).isEmpty());
        REQUIRE(view.charAt(CpIndex{1}).isNoCodePoint());
        REQUIRE(view.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(view.charAt(CpIndex::zero()).isEndOfData());
    }

    void testStringViewRead() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = text.toCharView();

        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{4});
        REQUIRE_EQUAL(view.indexAt(StringSide::Front), CpIndex::zero());
        REQUIRE_EQUAL(view.indexAt(StringSide::Back), CpIndex{4});
        REQUIRE_EQUAL(view.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(view.charAt(StringSide::Back).toRawValue(), U'\U0001F600');
        {
            const auto character = view.charAt(StringSide::Front);
            const auto remaining = view.slice(StringSide::Back, view.length() - CpLength::one());
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"¢€😀"});
        }
        {
            const auto character = view.charAt(StringSide::Back);
            const auto remaining = view.slice(StringSide::Front, view.length() - CpLength::one());
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"A¢€"});
        }
        {
            const auto single = U8String{std::u8string_view{u8"€"}};
            const auto singleView = single.toCharView();
            const auto character = singleView.charAt(StringSide::Front);
            const auto remaining = singleView.slice(StringSide::Back, singleView.length() - CpLength::one());
            REQUIRE_EQUAL(character.toRawValue(), U'\u20AC');
            REQUIRE(remaining.isEmpty());
        }
        REQUIRE_EQUAL(view.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(view.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(view.charAt(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE(view.charAt(CpIndex{4}).isEndOfData());
        REQUIRE(view.charAt(CpIndex{5}).isNoCodePoint());
        REQUIRE_EQUAL(view.charAt(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE(view.charAt(CpIndex{4}).isEndOfData());
    }

    void testLiteralViewRead() {
        using namespace el::text::literals;

        const auto textView = u8"Ol\u00E1"_elv;
        const auto view = textView.toCharView();

        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(view.charAt(CpIndex{0}).toRawValue(), U'O');
        REQUIRE_EQUAL(view.charAt(CpIndex{1}).toRawValue(), U'l');
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'\u00E1');
    }

    void testInvalidUtf8Read() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = text.toCharView();

        REQUIRE_EQUAL(view.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(view.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(view.charAt(CpIndex{1}).isReplacement());
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'B');
        REQUIRE(view.charAt(CpIndex{1}).isReplacement());
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'B');
    }

    void testPredicateChecks() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = text.toCharView();

        REQUIRE(view.containsOnly(CharSet{u8"A¢€😀"_elv}));
        REQUIRE_FALSE(view.containsOnly(CharSet{u8"A¢€"_elv}));
        REQUIRE_FALSE(view.containsOnly(CharSet{}));

        const auto mixed = U8String{std::u8string_view{u8"ÄxK"}};
        REQUIRE(mixed.toCharView().containsOnly(CharSet{u8"ÄäxXkK"_elv}));
        REQUIRE_FALSE(mixed.toCharView().containsOnly(CharSet{u8"ÄäxX"_elv}));
    }

    void testSlice() {
        const auto text = U8String{std::string_view{"abcdef"}};
        const auto view = text.toCharView();

        REQUIRE_EQUAL(StringConverter{view.slice(CpRange{CpIndex{2}, CpLength{3}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{view.slice(CpRange{CpIndex{2}, CpIndex{5}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{view.slice(CpRange{CpIndex{2}, CpLength{3}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Front, CpLength{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, CpLength{2})}.toStdString(), "ef");
        REQUIRE(view.slice(CpRange{CpIndex{2}, CpLength{0}}).isEmpty());
        REQUIRE(view.slice(CpRange{CpIndex{9}, CpLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{view.slice(CpRange{CpIndex{4}, CpLength{99}})}.toStdString(), "ef");
        REQUIRE_EQUAL(StringConverter{view.slice(CpRange{CpIndex{3}, CpLength::infinite()})}.toStdString(), "def");
        REQUIRE(view.slice(CpRange::noRange()).isEmpty());
        REQUIRE(view.slice(CpRange{CpIndex::noIndex(), CpLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Front, CpLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Front, CpLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(view.slice(StringSide::Front, CpLength{0}).isEmpty());
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, CpLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, CpLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(view.slice(StringSide::Back, CpLength{0}).isEmpty());
    }

    void testUtf8Slice() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto view = text.toCharView();

        REQUIRE_EQUAL(StringConverter{view.slice(CpRange{CpIndex{1}, CpLength{2}})}.toStdU8String(), u8"¢€");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Front, CpLength{3})}.toStdU8String(), u8"A¢€");
        REQUIRE_EQUAL(StringConverter{view.slice(StringSide::Back, CpLength{2})}.toStdU8String(), u8"€😀");
    }

    void testLiteralSlice() {
        using namespace el::text::literals;

        const auto textView = u8"Ol\u00E1"_elv;
        const auto slice = textView.toCharView().slice(CpRange{CpIndex{1}, CpLength{2}});

        REQUIRE_EQUAL(StringConverter{slice}.toStdU8String(), u8"l\u00E1");
        REQUIRE_EQUAL(
            StringConverter{slice.toCharView().slice(StringSide::Back, CpLength{1})}.toStdU8String(), u8"\u00E1");
    }

    void testNestedSlice() {
        const auto text = U8String{std::string_view{"abcdef"}};
        const auto first = text.toCharView().slice(CpRange{CpIndex{1}, CpLength{4}});
        const auto second = first.toCharView().slice(CpRange{CpIndex{1}, CpLength{2}});

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), "bcde");
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), "cd");
    }

    void testInvalidUtf8Slice() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = text.toCharView();
        const auto invalidOnly = view.slice(CpRange{CpIndex{1}, CpLength{1}});
        const auto invalidAndTail = view.slice(CpRange{CpIndex{1}, CpLength{2}});

        REQUIRE_EQUAL(th::toStdU32String(StringConverter{invalidOnly}.toStdString()), std::u32string{U"\uFFFD"});
        REQUIRE_FALSE(invalidOnly.isValidUtf8());
        REQUIRE(invalidOnly.toCharView().charAt(CpIndex::zero()).isReplacement());
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{invalidAndTail}.toStdString()), std::u32string{U"\uFFFDB"});
    }

    void testForwardFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = text.toCharView().slice(CpRange{CpIndex{2}, CpLength{4}}).toCharView();

        REQUIRE_EQUAL(view.find(u8"¢€"_elv), CpIndex{1U});
        REQUIRE_EQUAL(view.find(U8StringView{}, CpIndex{3U}), CpIndex{3U});
        REQUIRE_EQUAL(view.find(U8StringView{}, view.indexAt(StringSide::Back)), view.indexAt(StringSide::Back));
        REQUIRE(view.find(u8"€¢"_elv).isNoIndex());
        REQUIRE(view.find(u8"A"_elv, CpIndex{5U}).isNoIndex());
        REQUIRE_EQUAL(view.findFirstOf(CharSet{Char{0x20ACU}}), CpIndex{2U});
        REQUIRE_EQUAL(view.findFirstOf(CharSet{Char{0x20ACU}}, CpIndex{1U}), CpIndex{2U});
        REQUIRE_EQUAL(view.findFirstOf(CharSet{u8"z😀"_elv}), CpIndex{3U});
        REQUIRE(view.findFirstOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{Char{0x41U}}), CpIndex{1U});
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{u8"A¢"_elv}), CpIndex{2U});
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{}), CpIndex{0U});
        REQUIRE(view.findFirstOf(CharSet{Char{0x41U}}, CpIndex::noIndex()).isNoIndex());
        REQUIRE(view.findFirstOf(CharSet{Char{0x41U}}, view.indexAt(StringSide::Back)).isNoIndex());
        REQUIRE(view.find(U8StringView{}, CpIndex{5U}).isNoIndex());
    }

    void testReverseFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = text.toCharView().slice(CpRange{CpIndex{2}, CpLength{4}}).toCharView();

        REQUIRE_EQUAL(view.findLastOf(CharSet{Char{0x20ACU}}), CpIndex{2U});
        REQUIRE_EQUAL(view.findLastOf(CharSet{Char{0x20ACU}}, CpIndex{3U}), CpIndex{2U});
        REQUIRE_EQUAL(view.findLastOf(CharSet{u8"z😀"_elv}), CpIndex{3U});
        REQUIRE(view.findLastOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{Char{0x1F600U}}), CpIndex{2U});
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{u8"€😀"_elv}), CpIndex{1U});
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{}), CpIndex{3U});
        REQUIRE(view.findLastOf(CharSet{Char{0x41U}}, CpIndex::zero()).isNoIndex());
        REQUIRE(view.findLastOf(CharSet{Char{0x41U}}, CpIndex::noIndex()).isNoIndex());
        REQUIRE(view.findLastOf(CharSet{Char{0x41U}}, CpIndex{5U}).isNoIndex());
    }

    void testInvalidUtf8Find() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};
        const auto view = text.toCharView();
        const auto invalidNeedleBytes = th::stdStringFromHex("C0");
        const auto invalidNeedle = U8String{std::string_view{invalidNeedleBytes}};

        REQUIRE_EQUAL(view.findFirstOf(CharSet{Char::replacement()}), CpIndex{1U});
        REQUIRE_EQUAL(view.findFirstNotOf(CharSet{Char{0x41U}}), CpIndex{1U});
        REQUIRE_EQUAL(view.find(U8StringView{invalidNeedle}), CpIndex{1U});
        REQUIRE_EQUAL(view.findLastOf(CharSet{Char::replacement()}), CpIndex{1U});
        REQUIRE_EQUAL(view.findLastNotOf(CharSet{Char{0x42U}}), CpIndex{1U});
    }

    void testNestedSliceFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto first = text.toCharView().slice(CpRange{CpIndex{1}, CpLength{6}}).toCharView();
        const auto second = first.slice(CpRange{CpIndex{1}, CpLength{4}}).toCharView();

        REQUIRE_EQUAL(
            StringConverter{first.slice(CpRange{first.indexAt(StringSide::Front), first.indexAt(StringSide::Back)})}
                .toStdU8String(),
            u8"xA¢€😀y");
        REQUIRE_EQUAL(
            StringConverter{second.slice(CpRange{second.indexAt(StringSide::Front), second.indexAt(StringSide::Back)})}
                .toStdU8String(),
            u8"A¢€😀");
        REQUIRE_EQUAL(first.find(u8"€😀"_elv), CpIndex{3U});
        REQUIRE_EQUAL(second.find(u8"€😀"_elv), CpIndex{2U});
        REQUIRE_EQUAL(second.findFirstOf(CharSet{u8"😀"_elv}), CpIndex{3U});
        REQUIRE_EQUAL(second.findLastNotOf(CharSet{u8"€😀"_elv}), CpIndex{1U});
    }

private:
    [[nodiscard]] static auto invalidUtf8Data() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }
};
