// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u8/impl/U8StringData.hpp>
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
#include <type_traits>

using namespace el::text;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8String U8StringView U8StringCharView U8StringLiteral U8StringComparisonTools BooleanFormat)
class U8StringTest final : public el::UnitTest {
public:
    void testDefaultStringIsEmpty() {
        const auto text = U8String{};

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{});
        REQUIRE(StringConverter{text}.toStdU8String().empty());
        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{0});
        REQUIRE_EQUAL(text.characterLength(), CpLength::zero());
        REQUIRE(text.isValidUtf8());
        REQUIRE_EQUAL(text.indexAt(StringSide::Front), ByteIndex::zero());
        REQUIRE_EQUAL(text.indexAt(StringSide::Back), ByteIndex::zero());
        REQUIRE(text.charAt(StringSide::Front).isNull());
        REQUIRE(text.charAt(StringSide::Back).isNull());
        REQUIRE(text.charAt(text.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(text.slice(StringSide::Front, ByteLength{1U}).isEmpty());
        REQUIRE(text.slice(StringSide::Back, ByteLength{1U}).isEmpty());
        const auto [emptyCharacter, emptyRemaining] = text.slice(StringSide::Front);
        REQUIRE(emptyCharacter.isEndOfData());
        REQUIRE(emptyRemaining.isEmpty());
    }

    void testStringFromStdString() {
        static_assert(std::is_constructible_v<U8String, std::string_view>);
        static_assert(!std::is_convertible_v<std::string_view, U8String>);
        using namespace el::text::literals;

        const auto text = U8String{std::string_view{"Hello"}};

        REQUIRE_FALSE(text.isEmpty());
        REQUIRE_EQUAL(text, "Hello"_el);
    }

    void testStringFromStdU8String() {
        static_assert(std::is_constructible_v<U8String, std::u8string_view>);
        static_assert(!std::is_convertible_v<std::u8string_view, U8String>);
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"Hello"}};

        REQUIRE_FALSE(text.isEmpty());
        REQUIRE_EQUAL(text, u8"Hello"_el);
    }

    void testStringCopiesLiteral() {
        using namespace el::text::literals;

        const auto text = "Hello"_els;

        REQUIRE_FALSE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
    }

    void testRepeatedAppend() {
        using namespace el::text::literals;

        const auto source = U8String{std::u8string_view{u8"xA¢"}};
        const auto view = U8StringView{source}.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}});

        auto repeatedText = U8String{};
        repeatedText.append(view, ElementCount{3U});
        REQUIRE_EQUAL(StringConverter{repeatedText}.toStdU8String(), std::u8string{u8"A¢A¢A¢"});
        REQUIRE_EQUAL(repeatedText.length(), ByteLength{9U});
        REQUIRE_EQUAL(repeatedText.characterLength(), CpLength{6U});

        auto repeatedCharacter = U8String{};
        repeatedCharacter.append(Char{U'\u20AC'}, CpLength{3U});
        REQUIRE_EQUAL(StringConverter{repeatedCharacter}.toStdU8String(), std::u8string{u8"€€€"});
        REQUIRE_EQUAL(repeatedCharacter.length(), ByteLength{9U});
        REQUIRE_EQUAL(repeatedCharacter.characterLength(), CpLength{3U});
        REQUIRE_EQUAL(U8String::fromCharacter(Char{U'\u20AC'}, CpLength{3U}), repeatedCharacter);
        REQUIRE_EQUAL(U8String::fromCharacter(Char{U'A'}), "A"_el);

        REQUIRE(U8String{}.append(view, ElementCount::zero()).isEmpty());
        REQUIRE(U8String{}.append(""_elv, ElementCount{5U}).isEmpty());
        REQUIRE(U8String{}.append(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE(U8String::fromCharacter(Char{U'A'}, CpLength::zero()).isEmpty());
        REQUIRE(U8String::fromCharacter(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE_THROWS(U8String{}.append("x"_elv, ElementCount::infinite()));
        REQUIRE_THROWS(U8String::fromCharacter(Char{U'A'}, CpLength::infinite()));
    }

    void testBooleanConversion() {
        using namespace el::text::literals;

        REQUIRE_EQUAL(U8String::fromBoolean(true), "true"_el);
        REQUIRE_EQUAL(
            U8String::fromBoolean(false, BooleanFormat::yesNo().setCapitalization(Capitalization::Uppercase)), "NO"_el);
        REQUIRE_EQUAL(
            U8String::fromBoolean(true, BooleanFormat::enabledDisabled().setCapitalization(Capitalization::Titlecase)),
            "Enabled"_el);
    }

    void testClear() {
        auto text = U8String{std::string_view{"Hello"}};
        text.reserve(ByteLength{8U});

        text.clear();

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{});
        REQUIRE_EQUAL(text.capacity(), ByteLength{8U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(8U));
    }

    void testResetReleasesReservedStorage() {
        auto text = U8String{std::string_view{"Hello"}};
        text.reserve(ByteLength{8U});

        text.reset();

        REQUIRE(text.isEmpty());
        REQUIRE(text.capacity().isZero());
        REQUIRE(text.memoryUsage().isZero());
    }

    void testByteIndexedRead() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};

        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{10});
        REQUIRE_EQUAL(text.characterLength(), CpLength{4});
        REQUIRE(text.isValidUtf8());
        REQUIRE_EQUAL(text.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(StringSide::Back).toRawValue(), U'\U0001F600');
        {
            const auto [character, remaining] = text.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"¢€😀"});
        }
        {
            const auto [character, remaining] = text.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"A¢€"});
        }
        {
            const auto view = U8StringView{text};
            const auto [character, remaining] = view.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"¢€😀"});
        }
        {
            const auto charView = U8StringView{text}.toCharView();
            const auto [character, remaining] = charView.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"A¢€"});
        }
        {
            const auto single = U8String{std::u8string_view{u8"€"}};
            const auto [character, remaining] = single.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'\u20AC');
            REQUIRE(remaining.isEmpty());
        }
        REQUIRE_EQUAL(text.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(ByteIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(text.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(text.charAt(ByteIndex{6}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(text[ByteIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(text[ByteIndex{6}].toRawValue(), U'\U0001F600');
        REQUIRE(text.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(text.charAt(ByteIndex{11}).isNoCodePoint());
        REQUIRE(text.charAt(ByteIndex::noIndex()).isNoCodePoint());
        REQUIRE(text.charAt(ByteIndex{2}).isReplacement());
        REQUIRE_EQUAL(text.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE(text.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(text.charAt(ByteIndex::noIndex()).isNoCodePoint());
        REQUIRE_EQUAL(text.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(text.charAt(CpIndex{2}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(text.charAt(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(text[CpIndex{2}].toRawValue(), U'\u20AC');
        REQUIRE(text.charAt(CpIndex{4}).isEndOfData());
        REQUIRE(text.charAt(CpIndex{5}).isNoCodePoint());
        REQUIRE(text.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(text[CpIndex{4}].isEndOfData());
        REQUIRE(text[CpIndex::noIndex()].isNoCodePoint());
    }

    void testCaseMapping() {
        using namespace el::text::literals;

        const auto mixed = U8String{std::u8string_view{u8"AÄΣςK"}};
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::caseFolded)}.toStdU32String(), std::u32string{U"aäσσk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toLowercase)}.toStdU32String(), std::u32string{U"aäσςk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toUppercase)}.toStdU32String(), std::u32string{U"AÄΣΣK"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"aÄΣςk"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiUppercase)}.toStdU32String(), std::u32string{U"AÄΣςK"});

        const auto empty = U8String{};
        REQUIRE(empty.transformed(Char::toLowercase).isEmpty());
        REQUIRE(empty.transformed(Char::toAsciiUppercase).isEmpty());
        REQUIRE(U8StringView{}.transformed(Char::caseFolded).isEmpty());
        REQUIRE_EQUAL(mixed.transformed(nullptr), mixed);

        const auto unchanged = U8String{std::string_view{"abc"}};
        REQUIRE_EQUAL(unchanged.transformed(Char::toAsciiLowercase).storageId(), unchanged.storageId());
        REQUIRE_EQUAL(unchanged.transformed(Char::caseFolded).storageId(), unchanged.storageId());

        const auto sharedView = U8StringView{unchanged};
        REQUIRE_EQUAL(
            StringConverter{sharedView.transformed(Char::toAsciiLowercase)}.toStdString(), std::string{"abc"});

        const auto literalView = "abc"_elv;
        REQUIRE_EQUAL(
            StringConverter{literalView.transformed(Char::toAsciiLowercase)}.toStdString(), std::string{"abc"});

        const auto slicedView = "xabcx"_elv.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}});
        const auto slicedLower = slicedView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedLower}.toStdString(), std::string{"abc"});

        const auto fullCharViewLower = literalView.toCharView().transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{fullCharViewLower}.toStdString(), std::string{"abc"});
        const auto slicedCharViewLower = slicedView.toCharView().transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedCharViewLower}.toStdString(), std::string{"abc"});

        auto invalidOnlyBytes = std::string{"1"};
        invalidOnlyBytes.push_back(static_cast<char>(0xC0U));
        const auto invalidOnly = U8String{std::string_view{invalidOnlyBytes}};
        REQUIRE_EQUAL(invalidOnly.transformed(Char::toAsciiLowercase).storageId(), invalidOnly.storageId());
        REQUIRE_EQUAL(
            StringConverter{U8StringView{invalidOnly}.transformed(Char::toAsciiLowercase)}.toStdString(),
            th::stdStringFromHex("31 EF BF BD"));

        auto invalidBytes = std::string{"A"};
        invalidBytes.push_back(static_cast<char>(0xC0U));
        invalidBytes.push_back('B');
        const auto invalid = U8String{std::string_view{invalidBytes}};
        REQUIRE_EQUAL(
            StringConverter{invalid.transformed(Char::toAsciiLowercase)}.toStdString(),
            th::stdStringFromHex("61 EF BF BD 62"));
        REQUIRE_EQUAL(
            StringConverter{U8StringView{invalid}.transformed(Char::toAsciiLowercase)}.toStdString(),
            th::stdStringFromHex("61 EF BF BD 62"));
    }

    void testAdvance() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        auto index = ByteIndex::zero();

        REQUIRE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE(text.advance(index, CpLength{2}));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});
        REQUIRE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
        REQUIRE_FALSE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex{4};
        REQUIRE_FALSE(text.advance(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        REQUIRE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        index = ByteIndex::zero();
        REQUIRE(text.advance(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex{99};
        REQUIRE_FALSE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex::noIndex();
        REQUIRE_FALSE(text.advance(index));
        REQUIRE(index.isNoIndex());
    }

    void testRetreat() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        auto index = ByteIndex{5};

        REQUIRE_FALSE(text.retreat(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        REQUIRE(text.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        index = ByteIndex{99};
        REQUIRE(text.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});

        // `true`, because an actual movement was performed.
        REQUIRE(text.retreat(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{0});
    }

    void testIndexConversions() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};

        REQUIRE_EQUAL(text.indexAt(CpIndex{0}), ByteIndex{0});
        REQUIRE_EQUAL(text.indexAt(CpIndex{1}), ByteIndex{1});
        REQUIRE_EQUAL(text.indexAt(CpIndex{2}), ByteIndex{3});
        REQUIRE_EQUAL(text.indexAt(CpIndex{3}), ByteIndex{6});
        REQUIRE_EQUAL(text.indexAt(CpIndex{4}), text.indexAt(StringSide::Back));
        REQUIRE(text.indexAt(CpIndex{5}).isNoIndex());
        REQUIRE(text.indexAt(CpIndex::noIndex()).isNoIndex());

        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{0}), CpIndex{0});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{2}), CpIndex{1});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{3}), CpIndex{2});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{5}), CpIndex{2});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{6}), CpIndex{3});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{9}), CpIndex{3});
        REQUIRE_EQUAL(text.toCharIndex(text.indexAt(StringSide::Back)), CpIndex{4});
        REQUIRE(text.toCharIndex(ByteIndex{11}).isNoIndex());
        REQUIRE(text.toCharIndex(ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8Read() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};

        REQUIRE_FALSE(text.isValidUtf8());
        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(text.characterLength(), CpLength{3});
        REQUIRE_EQUAL(text.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE(text.charAt(ByteIndex{1}).isReplacement());
        REQUIRE_EQUAL(text.charAt(ByteIndex{2}).toRawValue(), U'B');
        REQUIRE(text.charAt(ByteIndex{1}).isReplacement());
        REQUIRE_EQUAL(text.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(text.charAt(CpIndex{1}).isReplacement());
        REQUIRE(text[CpIndex{1}].isReplacement());
        REQUIRE_EQUAL(text.charAt(CpIndex{2}).toRawValue(), U'B');

        auto index = ByteIndex{1};
        REQUIRE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(text.indexAt(CpIndex{2}), ByteIndex{2});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(text.toCharIndex(ByteIndex{2}), CpIndex{2});
    }

    void testSlice() {
        const auto text = U8String{std::string_view{"abcdef"}};

        REQUIRE_EQUAL(StringConverter{text.slice(ByteRange{ByteIndex{2}, ByteLength{3}})}.toStdString(), "cde");
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Front, ByteLength{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteLength{2})}.toStdString(), "ef");
        REQUIRE(text.slice(ByteRange{ByteIndex{2}, ByteLength{0}}).isEmpty());
        REQUIRE(text.slice(ByteRange{ByteIndex{9}, ByteLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(ByteRange{ByteIndex{4}, ByteLength{99}})}.toStdString(), "ef");
        REQUIRE_EQUAL(
            StringConverter{text.slice(ByteRange{ByteIndex{3}, ByteLength::infinite()})}.toStdString(), "def");
        REQUIRE(text.slice(ByteRange::noRange()).isEmpty());
        REQUIRE(text.slice(ByteRange{ByteIndex::noIndex(), ByteLength{1}}).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(text.slice(StringSide::Back, ByteLength{0}).isEmpty());

        const auto unicode = U8String{std::u8string_view{u8"A¢€😀BC"}};
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
        const auto first = text.slice(ByteRange{ByteIndex{1}, ByteLength{4}});
        const auto second = first.slice(ByteRange{ByteIndex{1}, ByteLength{2}});

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), "bcde");
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), "cd");
    }

    void testSliceThroughUtf8Sequence() {
        const auto text = U8String{std::u8string_view{u8"A¢€"}};
        const auto slice = text.slice(ByteRange{ByteIndex{2}, ByteLength{2}});

        REQUIRE_EQUAL(th::toStdU32String(StringConverter{slice}.toStdString()), std::u32string{U"\uFFFD\uFFFD"});
        REQUIRE_FALSE(slice.isValidUtf8());
        REQUIRE(slice.charAt(ByteIndex::zero()).isReplacement());
    }

    void testPredicateChecks() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE(text.startsWith(u8"A¢"_elv));
        REQUIRE(text.startsWith(u8"A"_elv));
        REQUIRE(text.startsWith(U8StringView{}));
        REQUIRE_FALSE(text.startsWith(u8"¢"_elv));
        REQUIRE(text.endsWith(u8"😀"_elv));
        REQUIRE(text.endsWith(U8StringView{}));
        REQUIRE_FALSE(text.endsWith(u8"€"_elv));
        REQUIRE(text.contains(u8"¢€"_elv));
        REQUIRE(text.contains(u8"€"_elv));
        REQUIRE(text.contains(U8StringView{}));
        REQUIRE_FALSE(text.contains(u8"€¢"_elv));
        REQUIRE_EQUAL(text.count(u8"¢€"_elv), ElementCount{1U});
        REQUIRE_EQUAL(text.count(u8"€"_elv), ElementCount{1U});
        REQUIRE_EQUAL(text.count(U8StringView{}), ElementCount::zero());
        REQUIRE_EQUAL(u8"¢€¢€"_els.count(u8"¢€"_elv), ElementCount{2U});
        REQUIRE_EQUAL("aaaa"_els.count("aa"_elv), ElementCount{2U});
        REQUIRE(text.containsOneOf(CharSet{u8"z😀"_elv}));
        REQUIRE(text.containsOneOf(CharSet{Char{0x20ACU}}));
        REQUIRE_FALSE(text.containsOneOf(CharSet{"xyz"_elv}));
        REQUIRE_FALSE(text.containsOneOf(CharSet{}));
        REQUIRE(text.containsOnly(CharSet{u8"A¢€😀"_elv}));
        REQUIRE_FALSE(text.containsOnly(CharSet{u8"A¢€"_elv}));
        REQUIRE_FALSE(text.containsOnly(CharSet{}));
    }

    void testComparisonChecks() {
        using namespace el::text::literals;

        const auto first = U8String{std::u8string_view{u8"A¢"}};
        const auto same = U8String{std::u8string_view{u8"A¢"}};
        const auto greater = U8String{std::u8string_view{u8"A€"}};
        const auto view = U8StringView{same};
        const auto charView = view.toCharView();
        const auto asciiFoldedCompare =
            CharCompareFn{[](const Char left, const Char right) noexcept -> std::strong_ordering {
                return left.toAsciiLowercase() <=> right.toAsciiLowercase();
            }};
        const auto reverseCompare = CharCompareFn{
            [](const Char left, const Char right) noexcept -> std::strong_ordering { return right <=> left; }};

        REQUIRE_EQUAL(first.compare(same), std::strong_ordering::equal);
        REQUIRE_EQUAL(first.compare(view), std::strong_ordering::equal);
        REQUIRE_EQUAL(first.compare(u8"a¢"_elv, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(view.compare(u8"a¢"_els, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(u8"a"_els.compare(u8"b"_elv, reverseCompare), std::strong_ordering::greater);
        REQUIRE(first.compare(greater) < 0);
        REQUIRE(view.compare(first) == std::strong_ordering::equal);
        REQUIRE(charView.startsWith(u8"A"_elv));
        REQUIRE(charView.endsWith(u8"¢"_elv));
        REQUIRE(charView.contains(u8"A¢"_elv));

        const auto invalid = U8String{std::string_view{th::stdStringFromHex("41 C0 42")}};
        const auto replacement = U8String{std::u8string_view{u8"A\uFFFDB"}};
        REQUIRE_EQUAL(invalid.compare(replacement), std::strong_ordering::equal);
    }

    void testCaseInsensitiveComparisonChecks() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"ÄbcK"}};
        const auto view = U8StringView{text};
        const auto charView = view.toCharView();

        REQUIRE_EQUAL(text.compare(u8"äbcK"_elv, Char::compareCaseFolded), std::strong_ordering::equal);
        REQUIRE_EQUAL(view.compare(u8"äbcK"_elv, Char::compareCaseFolded), std::strong_ordering::equal);
        REQUIRE(text.startsWith(u8"äB"_elv, Char::compareCaseFolded));
        REQUIRE(text.startsWith(u8"ä"_elv, Char::compareCaseFolded));
        REQUIRE(text.endsWith(u8"k"_elv, Char::compareCaseFolded));
        REQUIRE(text.contains(u8"BC"_elv, Char::compareCaseFolded));
        REQUIRE_EQUAL(text.count(u8"BC"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(text.count(u8"k"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(text.count(U8StringView{}, Char::compareCaseFolded), ElementCount::zero());
        REQUIRE_EQUAL(view.count(u8"ä"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(view.count(u8"X"_elv, Char::compareCaseFolded), ElementCount::zero());
        REQUIRE(charView.startsWith(u8"äB"_elv, Char::compareCaseFolded));
        REQUIRE(charView.endsWith(u8"k"_elv, Char::compareCaseFolded));
        REQUIRE(charView.contains(u8"BC"_elv, Char::compareCaseFolded));
        REQUIRE_FALSE(text.contains(u8"bd"_elv, Char::compareCaseFolded));
    }

    void testInvalidUtf8PredicateChecks() {
        using namespace el::text::literals;

        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};

        REQUIRE_FALSE(text.isValidUtf8());
        REQUIRE(text.contains(u8"\uFFFD"_elv));
        REQUIRE_EQUAL(text.count(u8"\uFFFD"_elv), ElementCount{1U});
        REQUIRE_EQUAL(text.count(u8"\uFFFD"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE(text.containsOneOf(CharSet{Char::replacement()}));
        REQUIRE_FALSE(text.startsWith(u8"\uFFFD"_elv));
        REQUIRE_FALSE(text.endsWith(u8"\uFFFD"_elv));
    }

    void testForwardFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(text.find(u8"¢€"_elv), ByteIndex{1U});
        REQUIRE_EQUAL(text.find(U8StringView{}, ByteIndex{3U}), ByteIndex{3U});
        REQUIRE(text.find(u8"€¢"_elv).isNoIndex());
        REQUIRE_EQUAL(text.findFirstOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findFirstOf(CharSet{Char{0x20ACU}}, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findFirstOf(CharSet{u8"z😀"_elv}), ByteIndex{6U});
        REQUIRE(text.findFirstOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{u8"A¢"_elv}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{}), ByteIndex{0U});
        REQUIRE(text.findFirstOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testReverseFind() {
        using namespace el::text::literals;

        const auto text = U8String{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(text.findLastOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findLastOf(CharSet{Char{0x20ACU}}, ByteIndex{6U}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findLastOf(CharSet{u8"z😀"_elv}), ByteIndex{6U});
        REQUIRE(text.findLastOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{Char{0x1F600U}}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{u8"€😀"_elv}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{}), ByteIndex{6U});
        REQUIRE(text.findLastOf(CharSet{Char{0x41U}}, ByteIndex::zero()).isNoIndex());
        REQUIRE(text.findLastOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8ForwardFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};

        REQUIRE_EQUAL(text.findFirstOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
    }

    void testInvalidUtf8ReverseFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8String{std::string_view{data}};

        REQUIRE_EQUAL(text.findLastOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{Char{0x42U}}), ByteIndex{1U});
    }

    void testDetach() {
        auto first = U8String{std::string_view{"Hello"}};
        const auto second = first;
        const auto *firstData = el::text::impl::UnsafeU8StringAccess{first}.data();
        const auto *secondData = el::text::impl::UnsafeU8StringAccess{second}.data();

        REQUIRE_EQUAL(firstData, secondData);

        first.detach();

        REQUIRE_NOT_EQUAL(el::text::impl::UnsafeU8StringAccess{first}.data(), firstData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{second}.data(), secondData);
        REQUIRE_EQUAL(StringConverter{first}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), std::string{"Hello"});
    }

    void testReserveOnEmptyStringCreatesCapacity() {
        auto text = U8String{};

        text.reserve(ByteLength{7U});

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(text.capacity(), ByteLength{7U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(7U));
    }

    void testReserveOnNonEmptyStringGrowsCapacityWithoutChangingText() {
        auto text = U8String{std::string_view{"Hello"}};
        const auto *originalData = el::text::impl::UnsafeU8StringAccess{text}.data();

        text.reserve(ByteLength{9U});

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{9U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(9U));
        REQUIRE_NOT_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data(), originalData);

        const auto *reservedData = el::text::impl::UnsafeU8StringAccess{text}.data();
        text.reserve(ByteLength{3U});
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data(), reservedData);
    }

    void testReserveOnSliceMaterializesStandaloneStorage() {
        auto text = U8String{std::string_view{"abcdef"}}.slice(ByteRange{ByteIndex{2U}, ByteLength{2U}});

        text.reserve(ByteLength{6U});

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"cd"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{6U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(6U));
    }

    void testShrinkToFitReleasesUnusedCapacity() {
        auto text = U8String{std::string_view{"Hello"}};
        text.reserve(ByteLength{9U});

        text.shrinkToFit();

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{5U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(5U));
    }

    void testShrinkToFitMaterializesSliceToExactSize() {
        auto text = U8String{std::string_view{"abcdef"}}.slice(ByteRange{ByteIndex{2U}, ByteLength{2U}});

        text.shrinkToFit();

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"cd"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{2U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(2U));
    }

    void testStdConversions() {
        const auto text = U8String{std::u8string_view{u8"A¢€😀"}};
        const auto slice = U8String{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{text}.toStdWString()), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{slice}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{slice}.toStdU8String(), std::u8string{u8"A¢€😀"});
        REQUIRE_EQUAL(StringConverter{slice}.toStdU16String(), th::stdU16StringFromHex("0041 00A2 20AC D83D DE00"));
        REQUIRE_EQUAL(StringConverter{slice}.toStdU32String(), std::u32string{U"A¢€😀"});
    }

    void testStdConversionsOnInvalidUtf8() {
        const auto text = U8String{std::string_view{invalidUtf8Data()}};

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{text}.toStdWString()), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A\uFFFDB"});
    }

    void testFromUtf8AllModes() {
        const auto valid = StringConverter{std::string_view{"Hello"}}.toU8String(EncodingErrorMode::Throw);

        REQUIRE_EQUAL(StringConverter{valid}.toStdString(), std::string{"Hello"});
        REQUIRE(
            StringConverter{StringConverter{std::string_view{}}.toU8String(EncodingErrorMode::Throw)}
                .toStdString()
                .empty());
        REQUIRE_FALSE(
            StringConverter{StringConverter{std::string_view{invalidUtf8Data()}}.toU8String(EncodingErrorMode::Ignore)}
                .toStdString()
                .empty());

        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireFromUtf8Modes(error));
        }
    }

    void testFromUtf16AllModes() {
        const auto valid =
            StringConverter{th::stdU16StringFromHex("0041 00A2 20AC D83D DE00")}.toU8String(EncodingErrorMode::Throw);

        REQUIRE_EQUAL(StringConverter{valid}.toStdU32String(), std::u32string{U"A¢€😀"});

        const auto invalid = th::stdU16StringFromHex("D800 0041 DC00 D800 D83D DE00");
        const auto invalidOnly = th::stdU16StringFromHex("D800 D800");

        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU8String(EncodingErrorMode::Replace)}.toStdU32String(),
            std::u32string{U"\uFFFDA\uFFFD\uFFFD😀"});
        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU8String(EncodingErrorMode::Ignore)}.toStdU32String(),
            std::u32string{U"A😀"});
        REQUIRE(
            StringConverter{StringConverter{invalidOnly}.toU8String(EncodingErrorMode::Ignore)}.toStdString().empty());
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingErrorMode::Throw));
    }

    void testFromUtf32AllModes() {
        const auto valid = StringConverter{std::u32string_view{U"A¢€😀"}}.toU8String(EncodingErrorMode::Throw);

        REQUIRE_EQUAL(StringConverter{valid}.toStdU32String(), std::u32string{U"A¢€😀"});

        const auto invalid = std::u32string{U'A', char32_t{0xD800U}, char32_t{0x110000U}, U'B'};

        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU8String(EncodingErrorMode::Replace)}.toStdU32String(),
            std::u32string{U"A\uFFFD\uFFFDB"});
        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU8String(EncodingErrorMode::Ignore)}.toStdU32String(),
            std::u32string{U"AB"});
        REQUIRE(
            StringConverter{StringConverter{std::u32string{char32_t{0xD800U}, char32_t{0x110000U}}}.toU8String(
                                EncodingErrorMode::Ignore)}
                .toStdString()
                .empty());
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingErrorMode::Throw));
    }

    void testFromWideAllModes() {
#ifdef ERBSLAND_WCHAR_16BIT
        const auto invalid = th::stdWStringFromHex("D800 0041 DC00 D800 D83D DE00");
        const auto valid = th::stdWStringFromHex("0041 00A2 20AC D83D DE00");
#else
        const auto invalid = th::stdWStringFromHex("00000041 0000D800 00110000 00000042");
        const auto valid = th::stdWStringFromHex("00000041 000000A2 000020AC 0001F600");
#endif

        REQUIRE_EQUAL(
            StringConverter{StringConverter{valid}.toU8String(EncodingErrorMode::Throw)}.toStdU32String(),
            std::u32string{U"A¢€😀"});
        REQUIRE_FALSE(
            StringConverter{StringConverter{invalid}.toU8String(EncodingErrorMode::Replace)}.toStdU32String().empty());
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingErrorMode::Throw));
    }

private:
    [[nodiscard]] static auto expectedMemoryUsage(const std::size_t capacity) -> ByteLength {
        return ByteLength::fromSizeT(sizeof(el::text::impl::U8StringData) + capacity + 1U);
    }

    [[nodiscard]] static auto invalidUtf8Data() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }

    void requireFromUtf8Modes(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");

        REQUIRE_EQUAL(
            StringConverter{StringConverter{malformed}.toU8String(EncodingErrorMode::Replace)}.toStdU32String(),
            expectedReplaceDecodedText(error));
        REQUIRE_EQUAL(
            StringConverter{StringConverter{malformed}.toU8String(EncodingErrorMode::Ignore)}.toStdU32String(),
            expectedIgnoreDecodedText(error));
        REQUIRE_THROWS(StringConverter{malformed}.toU8String(EncodingErrorMode::Throw));
    }

    [[nodiscard]] static auto expectedReplaceDecodedText(const th::Utf8Error error) -> std::u32string {
        switch (error) {
        case th::Utf8Error::UnexpectedContinuationByte:
        case th::Utf8Error::Truncated2ByteSequence:
        case th::Utf8Error::SurrogateCodePoint:
        case th::Utf8Error::CodePointBeyondUnicodeRange:
        case th::Utf8Error::InvalidStartByte:
            return std::u32string{U"A\uFFFDB"};
        case th::Utf8Error::Overlong2ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn2ByteSequence:
            return std::u32string{U"A\uFFFD B"};
        case th::Utf8Error::Overlong3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::Truncated3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD B"};
        case th::Utf8Error::Overlong4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::Truncated4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFD B"};
        default:
            return {};
        }
    }

    [[nodiscard]] static auto expectedIgnoreDecodedText(const th::Utf8Error error) -> std::u32string {
        switch (error) {
        case th::Utf8Error::InvalidContinuationByteIn2ByteSequence:
        case th::Utf8Error::InvalidContinuationByteIn3ByteSequence:
        case th::Utf8Error::InvalidContinuationByteIn4ByteSequence:
            return std::u32string{U"A B"};
        default:
            return std::u32string{U"AB"};
        }
    }
};
