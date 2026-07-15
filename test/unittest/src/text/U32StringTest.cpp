// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringDecoder.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringView.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::CpRange;
using el::unit::ElementCount;
using el::unit::ElementIndex;
using namespace el::text;

TESTED_TARGETS(U32String U32StringView U32StringLiteral U32StringEncodingTools U32StringModifyTools BooleanFormat)
class U32StringTest final : public el::UnitTest {
public:
    void testDefaultStringIsEmpty() {
        const auto text = U32String{};

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{});
        REQUIRE(StringConverter{text}.toStdU32String().empty());
        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{0});
        REQUIRE(text.isValidUtf32());
        REQUIRE(text.charAt(StringSide::Front).isNull());
        REQUIRE(text.charAt(StringSide::Back).isNull());
        REQUIRE(text.charAt(text.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(text.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(text.slice(StringSide::Front, CpLength{1U}).isEmpty());
        REQUIRE(text.slice(StringSide::Back, CpLength{1U}).isEmpty());
        REQUIRE(U32StringView{text}.slice(StringSide::Front, CpLength{1U}).isEmpty());
        const auto [emptyCharacter, emptyRemaining] = text.slice(StringSide::Front);
        REQUIRE(emptyCharacter.isEndOfData());
        REQUIRE(emptyRemaining.isEmpty());
        const auto [emptyViewCharacter, emptyViewRemaining] = U32StringView{text}.slice(StringSide::Front);
        REQUIRE(emptyViewCharacter.isEndOfData());
        REQUIRE(emptyViewRemaining.isEmpty());
    }

    void testRepeatedAppend() {
        using namespace el::text::literals;

        const auto source = U32String{std::u32string_view{U"xA\U0001F600"}};
        const auto view = U32StringView{source}.slice(CpRange{CpIndex{1U}, CpLength{2U}});

        auto repeatedText = U32String{};
        repeatedText.append(view, ElementCount{2U});
        REQUIRE_EQUAL(StringConverter{repeatedText}.toStdU32String(), std::u32string{U"A\U0001F600A\U0001F600"});
        REQUIRE_EQUAL(repeatedText.length(), CpLength{4U});

        auto repeatedCharacter = U32String{};
        repeatedCharacter.append(Char{U'\U0001F600'}, CpLength{3U});
        REQUIRE_EQUAL(
            StringConverter{repeatedCharacter}.toStdU32String(), std::u32string{U"\U0001F600\U0001F600\U0001F600"});
        REQUIRE_EQUAL(repeatedCharacter.length(), CpLength{3U});
        REQUIRE_EQUAL(U32String::fromCharacter(Char{U'\U0001F600'}, CpLength{3U}), repeatedCharacter);
        REQUIRE_EQUAL(U32String::fromCharacter(Char{U'A'}), U"A"_el);
        REQUIRE(U32String::fromJoined({}).isEmpty());
        REQUIRE_EQUAL(U32String::fromJoined({U"solo"_elv}), U"solo"_el);
        REQUIRE_EQUAL(
            StringConverter{U32String::fromJoined({U"prefix-"_elv, U""_elv, view, U"-suffix"_elv})}.toStdU32String(),
            std::u32string{U"prefix-A\U0001F600-suffix"});

        auto appended = U32String{std::u32string_view{U">"}};
        appended.append(view, ElementCount{2U}).append(Char{U'!'}, CpLength{2U});
        REQUIRE_EQUAL(StringConverter{appended}.toStdU32String(), std::u32string{U">A\U0001F600A\U0001F600!!"});
        REQUIRE_EQUAL(appended.length(), CpLength{7U});

        REQUIRE(U32String{}.append(view, ElementCount::zero()).isEmpty());
        REQUIRE(U32String{}.append(U32StringView{}, ElementCount{5U}).isEmpty());
        REQUIRE(U32String{}.append(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE(U32String::fromCharacter(Char{U'A'}, CpLength::zero()).isEmpty());
        REQUIRE(U32String::fromCharacter(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE_THROWS(U32String{}.append(view, ElementCount::infinite()));
        REQUIRE_THROWS(U32String::fromCharacter(Char{U'A'}, CpLength::infinite()));
    }

    void testBooleanConversion() {
        using namespace el::text::literals;

        REQUIRE_EQUAL(U32String::fromBoolean(true), U"true"_el);
        REQUIRE_EQUAL(
            U32String::fromBoolean(false, BooleanFormat::trueFalse().setCapitalization(Capitalization::Uppercase)),
            U"FALSE"_el);
        REQUIRE_EQUAL(U32String::fromBoolean(false, BooleanFormat::onOff()), U"off"_el);
        REQUIRE_EQUAL(
            U32String::fromBoolean(true, BooleanFormat::enabledDisabled().setCapitalization(Capitalization::Titlecase)),
            U"Enabled"_el);
    }

    void testStringFromStdU32StringViewAndViewCopy() {
        static_assert(std::is_constructible_v<U32String, std::u32string_view>);
        static_assert(!std::is_convertible_v<std::u32string_view, U32String>);
        using namespace el::text::literals;

        const auto text = U32String{std::u32string_view{U"xA\U0001F600x"}};
        const auto view = U32StringView{text}.slice(CpRange{CpIndex{1U}, CpLength{2U}});
        const auto copy = view.copy();

        REQUIRE_EQUAL(copy, U"A\U0001F600"_el);
        REQUIRE_NOT_EQUAL(copy.storageId(), view.storageId());
    }

    void testCodePointIndexedRead() {
        const auto text = U32String{std::u32string_view{U"A\u00A2\U0001F600"}};

        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{3});
        REQUIRE(text.isValidUtf32());
        REQUIRE_EQUAL(text.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(StringSide::Back).toRawValue(), U'\U0001F600');
        {
            const auto [character, remaining] = text.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU32String(), std::u32string{U"\u00A2\U0001F600"});
        }
        {
            const auto [character, remaining] = text.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU32String(), std::u32string{U"A\u00A2"});
        }
        {
            const auto view = U32StringView{text};
            const auto [character, remaining] = view.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU32String(), std::u32string{U"A\u00A2"});
        }
        {
            const auto single = U32String{std::u32string_view{U"\U0001F600"}};
            const auto [character, remaining] = single.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE(remaining.isEmpty());
        }
        REQUIRE_EQUAL(text.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(text.charAt(CpIndex{2}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(text[CpIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(text[CpIndex{1}].toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(text[CpIndex{2}].toRawValue(), U'\U0001F600');
        REQUIRE(text.charAt(CpIndex{3}).isEndOfData());
        REQUIRE(text.charAt(CpIndex{4}).isNoCodePoint());
        REQUIRE(text.charAt(CpIndex{3}).isEndOfData());
        REQUIRE(text[CpIndex{3}].isEndOfData());
        REQUIRE(text[CpIndex{4}].isNoCodePoint());
        REQUIRE(text[CpIndex::noIndex()].isNoCodePoint());

        const auto view = U32StringView{text};
        REQUIRE_EQUAL(view[CpIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(view[CpIndex{1}].toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(view[CpIndex{2}].toRawValue(), U'\U0001F600');
        REQUIRE(view[CpIndex{3}].isEndOfData());
        REQUIRE(view[CpIndex{4}].isNoCodePoint());
        REQUIRE(view[CpIndex::noIndex()].isNoCodePoint());
    }

    void testAdvanceRetreatAndIdentityIndexConversion() {
        const auto text = U32String{std::u32string_view{U"A\u00A2\U0001F600"}};
        auto index = CpIndex::zero();

        REQUIRE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE(text.advance(index, CpLength{2}));
        REQUIRE_EQUAL(index, text.indexAt(StringSide::Back));
        REQUIRE_FALSE(text.advance(index));

        REQUIRE(text.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE(text.retreat(index, CpLength::infinite()));
        REQUIRE(index.isZero());

        REQUIRE_EQUAL(text.indexAt(CpIndex{2}), CpIndex{2});
        REQUIRE_EQUAL(text.toCharIndex(text.indexAt(StringSide::Back)), text.indexAt(StringSide::Back));
    }

    void testIndexedSequentialRead() {
        const auto text = U32String{std::u32string_view{U"A\u00A2\U0001F600"}};
        auto index = CpIndex::zero();

        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{3});
        REQUIRE(text.readCharAndAdvance(index).isEndOfData());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{3});

        index = CpIndex{4U};
        REQUIRE(text.readCharAndAdvance(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        index = text.indexAt(StringSide::Back);
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'A');
        REQUIRE(index.isZero());
        REQUIRE(text.readCharAndRetreat(index).isEndOfData());
        REQUIRE(index.isZero());

        index = CpIndex{4U};
        REQUIRE(text.readCharAndRetreat(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        const auto view = U32StringView{text};
        index = CpIndex::zero();
        REQUIRE_EQUAL(view.readCharAndAdvance(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        index = view.indexAt(StringSide::Back);
        REQUIRE_EQUAL(view.readCharAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});

        const auto invalidData = std::u32string{U'A', static_cast<char32_t>(0x110000U), U'B'};
        const auto invalid = U32String{std::u32string_view{invalidData}};
        index = CpIndex{1U};
        REQUIRE(invalid.readCharAndAdvance(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE(invalid.readCharAndRetreat(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
    }

    void testInvalidCodePointIsTolerated() {
        const auto invalid = std::u32string{U'A', static_cast<char32_t>(0x110000U), U'B'};
        const auto text = U32String{std::u32string_view{invalid}};

        REQUIRE_FALSE(text.isValidUtf32());
        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(text.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(text.charAt(CpIndex{1}).isReplacement());
        REQUIRE_EQUAL(text.charAt(CpIndex{2}).toRawValue(), U'B');
        REQUIRE(text.charAt(CpIndex{1}).isReplacement());
        REQUIRE(text[CpIndex{1}].isReplacement());
        REQUIRE(U32StringView{text}[CpIndex{1}].isReplacement());
    }

    void testNoIndexFindPositions() {
        using namespace el::text::literals;

        const auto text = U32String{std::u32string_view{U"A\u00A2\u20AC\U0001F600"}};
        const auto view = U32StringView{text};
        const auto characters = CharSet{Char{U'A'}, Char{0x00A2U}};

        REQUIRE(text.find(U"A"_elv, CpIndex::noIndex()).isNoIndex());
        REQUIRE(text.findFirstOf(characters, CpIndex::noIndex()).isNoIndex());
        REQUIRE(text.findFirstNotOf(characters, CpIndex::noIndex()).isNoIndex());
        REQUIRE(text.findLastOf(characters, CpIndex::noIndex()).isNoIndex());
        REQUIRE(text.findLastNotOf(characters, CpIndex::noIndex()).isNoIndex());

        REQUIRE(view.find(U"A"_elv, CpIndex::noIndex()).isNoIndex());
        REQUIRE(view.findFirstOf(characters, CpIndex::noIndex()).isNoIndex());
        REQUIRE(view.findFirstNotOf(characters, CpIndex::noIndex()).isNoIndex());
        REQUIRE(view.findLastOf(characters, CpIndex::noIndex()).isNoIndex());
        REQUIRE(view.findLastNotOf(characters, CpIndex::noIndex()).isNoIndex());
    }

    void testCaseMapping() {
        using namespace el::text::literals;

        const auto mixed = U32String{std::u32string_view{U"A\u00C4\u03A3\u03C2K"}};
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::caseFolded)}.toStdU32String(), std::u32string{U"aäσσk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toLowercase)}.toStdU32String(), std::u32string{U"aäσςk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toUppercase)}.toStdU32String(), std::u32string{U"AÄΣΣK"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"aÄΣςk"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiUppercase)}.toStdU32String(), std::u32string{U"AÄΣςK"});

        const auto empty = U32String{};
        REQUIRE(empty.transformed(Char::toLowercase).isEmpty());
        REQUIRE(empty.transformed(Char::toAsciiUppercase).isEmpty());
        REQUIRE(U32StringView{}.transformed(Char::caseFolded).isEmpty());
        REQUIRE_EQUAL(mixed.transformed(nullptr), mixed);

        const auto unchanged = U32String{std::u32string_view{U"abc"}};
        REQUIRE_EQUAL(unchanged.transformed(Char::toAsciiLowercase).storageId(), unchanged.storageId());
        REQUIRE_EQUAL(unchanged.transformed(Char::caseFolded).storageId(), unchanged.storageId());

        const auto sharedView = U32StringView{unchanged};
        REQUIRE_EQUAL(
            StringConverter{sharedView.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"abc"});

        const auto literalView = U"abc"_elv;
        REQUIRE_EQUAL(
            StringConverter{literalView.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"abc"});

        const auto slicedView = U"xabcx"_elv.slice(CpRange{CpIndex{1U}, CpLength{3U}});
        const auto slicedLower = slicedView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedLower}.toStdU32String(), std::u32string{U"abc"});

        const auto invalidOnlyData = std::u32string{U'1', static_cast<char32_t>(0x110000U)};
        const auto invalidOnly = U32String{std::u32string_view{invalidOnlyData}};
        REQUIRE_EQUAL(invalidOnly.transformed(Char::toAsciiLowercase).storageId(), invalidOnly.storageId());
        REQUIRE_EQUAL(
            StringConverter{U32StringView{invalidOnly}.transformed(Char::toAsciiLowercase)}.toStdU32String(),
            std::u32string{U"1\uFFFD"});

        const auto invalidData = std::u32string{U'A', static_cast<char32_t>(0x110000U), U'B'};
        const auto invalid = U32String{std::u32string_view{invalidData}};
        REQUIRE_EQUAL(
            StringConverter{invalid.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"a\uFFFDb"});
        REQUIRE_EQUAL(
            StringConverter{U32StringView{invalid}.transformed(Char::toAsciiLowercase)}.toStdU32String(),
            std::u32string{U"a\uFFFDb"});
        REQUIRE_EQUAL(invalid.count(U"\uFFFD"_elv), ElementCount{1U});
        REQUIRE_EQUAL(U32StringView{invalid}.count(U"\uFFFD"_elv, Char::compareCaseFolded), ElementCount{1U});
    }

    void testSliceTrimFindSplitAndJoin() {
        using namespace el::text::literals;

        const auto text = U32String{std::u32string_view{U"  Alpha,Beta,Gamma  "}};
        const auto trimmed = text.trimmed();
        const auto parts = U32StringViewList::fromSplit(trimmed, CharSet{Char{U','}});
        const auto joined = parts.join(U"+"_elv);

        REQUIRE_EQUAL(StringConverter{trimmed}.toStdString(), std::string{"Alpha,Beta,Gamma"});
        REQUIRE_EQUAL(parts.count().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(StringConverter{parts[ElementIndex{1}]}.toStdString(), std::string{"Beta"});
        REQUIRE_EQUAL(U32StringViewList::fromSplit(trimmed, CharSet{Char{U','}}).count().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(StringConverter{joined}.toStdString(), std::string{"Alpha+Beta+Gamma"});
        REQUIRE_EQUAL(trimmed.find(U"Beta"_elv), CpIndex{6});
        REQUIRE_EQUAL(
            StringConverter{trimmed.slice(CpRange{CpIndex{6}, CpLength{4}})}.toStdString(), std::string{"Beta"});

        const auto unicode = U32String{std::u32string_view{U"A\u00A2\u20AC\U0001F600BC"}};
        const auto unicodeView = U32StringView{unicode};
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(CpRange{CpIndex{1}, CpLength{3}})}.toStdU32String(),
            std::u32string{U"\u00A2\u20AC\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpLength{4})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Back, CpLength{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpIndex{3})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpIndex{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE(unicodeView.slice(StringSide::Front, CpIndex::zero()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Back, CpIndex::zero())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, unicodeView.indexAt(StringSide::Back))}
                .toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicodeView.slice(StringSide::Back, unicodeView.indexAt(StringSide::Back)).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, CpIndex::noIndex())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicodeView.slice(StringSide::Back, CpIndex::noIndex()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, CpIndex{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicodeView.slice(StringSide::Back, CpIndex{99}).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Front, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(StringSide::Back, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex{2}, CpLength::zero()}).isEmpty());
        REQUIRE(unicode.slice(CpRange::noRange()).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex::noIndex(), CpLength{1}}).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex{99}, CpLength{1}}).isEmpty());

        {
            const auto [left, right] = unicode.splitAt(CpIndex{3});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), std::u32string{U"A\u00A2\u20AC"});
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), std::u32string{U"\U0001F600BC"});
        }
        {
            const auto [left, right] = unicodeView.splitAt(CpIndex::zero());
            REQUIRE(left.isEmpty());
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), StringConverter{unicode}.toStdU32String());
        }
        {
            const auto [left, right] = unicodeView.splitAt(unicodeView.indexAt(StringSide::Back));
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), StringConverter{unicode}.toStdU32String());
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = unicodeView.splitAt(CpIndex::noIndex());
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), StringConverter{unicode}.toStdU32String());
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = unicodeView.splitAt(CpIndex{99});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), StringConverter{unicode}.toStdU32String());
            REQUIRE(right.isEmpty());
        }
    }

    void testModificationComparisonAndTransform() {
        using namespace el::text::literals;

        auto text = U32String{std::u32string_view{U"Stra\u00DFe"}};
        text.replaceAll(CharSet{Char{U'S'}, Char{U's'}}, Char{U'X'});
        text.append(Char{U'!'});
        const auto lower = text.transformed(
            [](const Char character) noexcept -> Char { return character == Char{U'X'} ? Char{U'x'} : character; });
        const auto asciiFoldedCompare =
            CharCompareFn{[](const Char left, const Char right) noexcept -> std::strong_ordering {
                return left.toAsciiLowercase() <=> right.toAsciiLowercase();
            }};
        const auto reverseCompare = CharCompareFn{
            [](const Char left, const Char right) noexcept -> std::strong_ordering { return right <=> left; }};

        REQUIRE(text.startsWith(U"x"_elv, Char::compareCaseFolded));
        REQUIRE(text.contains(U"x"_elv, Char::compareCaseFolded));
        REQUIRE(text.containsOnly(CharSet::fromPattern(U"xXtra\u00DFe!"_elv)));
        REQUIRE_FALSE(text.containsOnly(CharSet::fromPattern(U"xXtra"_elv)));
        REQUIRE_EQUAL(StringConverter{lower}.toStdU32String(), std::u32string{U"xtra\u00DFe!"});
        REQUIRE_EQUAL(U"abc"_els.compare(U"ABC"_elv), std::strong_ordering::greater);
        REQUIRE_EQUAL(U"abc"_els.compare(U"ABC"_elv, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(U"ABC"_elv.compare(U"abc"_els, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(U"a"_els.compare(U"b"_elv, reverseCompare), std::strong_ordering::greater);
        REQUIRE_EQUAL(U"abc"_els.compare(U"ABC"_elv, Char::compareCaseFolded), std::strong_ordering::equal);
    }

    void testRangeInsertReplaceAndFirstModifiers() {
        using namespace el::text::literals;

        const auto source = U32String{std::u32string_view{U"A\U0001F600BC"}};

        REQUIRE_EQUAL(
            StringConverter{source.removed(CpRange{CpIndex{1U}, CpLength{1U}})}.toStdU32String(),
            std::u32string{U"ABC"});
        REQUIRE_EQUAL(
            StringConverter{source.kept(CpRange{CpIndex{1U}, CpLength{1U}})}.toStdU32String(),
            std::u32string{U"\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{source.inserted(CpIndex{2U}, U"X"_elv)}.toStdU32String(),
            std::u32string{U"A\U0001F600XBC"});
        REQUIRE_EQUAL(
            StringConverter{source.replaced(CpRange{CpIndex{1U}, CpLength{1U}}, U"X"_elv)}.toStdU32String(),
            std::u32string{U"AXBC"});

        auto text = U32String{std::u32string_view{U"abef"}};
        text.reserve(CpLength{16U});
        const auto capacity = text.capacity();

        text.insert(CpIndex{2U}, U"cd"_elv);
        text.replace(CpRange{CpIndex{2U}, CpLength{2U}}, U"XY"_elv);
        text.insert(CpIndex{99U}, U"!"_elv);
        text.insert(CpIndex::noIndex(), U"?"_elv);
        text.replace(CpRange::noRange(), U"?"_elv);

        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"abXYef!"});
        REQUIRE_EQUAL(text.capacity(), capacity);

        auto firstText = U32String{std::u32string_view{U"one one"}};
        firstText.removeFirst(U"one"_elv);
        REQUIRE_EQUAL(StringConverter{firstText}.toStdU32String(), std::u32string{U" one"});
        firstText.replaceFirst(U"ONE"_elv, U"two"_elv, Char::compareCaseFolded);
        REQUIRE_EQUAL(StringConverter{firstText}.toStdU32String(), std::u32string{U" two"});

        REQUIRE_EQUAL(StringConverter{source.removedFirst(U"\U0001F600"_elv)}.toStdU32String(), std::u32string{U"ABC"});
        REQUIRE_EQUAL(
            StringConverter{source.replacedFirst(U"\U0001F600"_elv, U"X"_elv)}.toStdU32String(),
            std::u32string{U"AXBC"});
        REQUIRE_EQUAL(
            StringConverter{source.removedFirst(U32StringView{})}.toStdU32String(),
            StringConverter{source}.toStdU32String());
    }

    void testViewAliasingAndMalformedModifiers() {
        using namespace el::text::literals;

        const auto source = U32String{std::u32string_view{U"--alpha--"}};
        const auto view = U32StringView{source}.slice(CpRange{CpIndex{2U}, CpLength{5U}});

        REQUIRE_EQUAL(
            StringConverter{view.inserted(CpIndex{2U}, U"!"_elv)}.toStdU32String(), std::u32string{U"al!pha"});
        REQUIRE_EQUAL(
            StringConverter{view.replaced(CpRange{CpIndex{2U}, CpLength{2U}}, U"X"_elv)}.toStdU32String(),
            std::u32string{U"alXa"});
        REQUIRE_EQUAL(StringConverter{view.removedFirst(U"ph"_elv)}.toStdU32String(), std::u32string{U"ala"});
        REQUIRE_EQUAL(
            StringConverter{view.replacedFirst(U"AL"_elv, U"AL"_elv, Char::compareCaseFolded)}.toStdU32String(),
            std::u32string{U"ALpha"});

        auto insertAlias = U32String{std::u32string_view{U"abcdef"}};
        insertAlias.reserve(CpLength{16U});
        const auto insertView = U32StringView{insertAlias}.slice(CpRange{CpIndex{1U}, CpLength{3U}});
        insertAlias.insert(CpIndex{3U}, insertView);
        REQUIRE_EQUAL(StringConverter{insertAlias}.toStdU32String(), std::u32string{U"abcbcddef"});

        auto replaceAlias = U32String{std::u32string_view{U"abcdef"}};
        replaceAlias.reserve(CpLength{16U});
        const auto replaceView = U32StringView{replaceAlias}.slice(CpRange{CpIndex{1U}, CpLength{4U}});
        replaceAlias.replace(CpRange{CpIndex{2U}, CpLength{2U}}, replaceView);
        REQUIRE_EQUAL(StringConverter{replaceAlias}.toStdU32String(), std::u32string{U"abbcdeef"});

        auto base = U32String{std::u32string_view{U"xxabcdefyy"}};
        auto sliced = base.slice(CpRange{CpIndex{2U}, CpLength{6U}});
        sliced.insert(CpIndex{3U}, U"!"_elv);
        REQUIRE_EQUAL(StringConverter{sliced}.toStdU32String(), std::u32string{U"abc!def"});
        REQUIRE_EQUAL(StringConverter{base}.toStdU32String(), std::u32string{U"xxabcdefyy"});

        const auto malformedData = std::u32string{U'A', static_cast<char32_t>(0x110000U), U'B'};
        auto malformed = U32String{std::u32string_view{malformedData}};
        malformed.replace(CpRange{CpIndex{1U}, CpLength{1U}}, U"?"_elv);
        malformed.insert(CpIndex{99U}, U"!"_elv);
        REQUIRE_EQUAL(StringConverter{malformed}.toStdU32String(), std::u32string{U"A?B!"});
    }

    void testPredicateChecks() {
        using namespace el::text::literals;

        const auto text = U32String{std::u32string_view{U"A\u00A2\U0001F600"}};
        const auto view = U32StringView{text};

        REQUIRE(text.containsOnly(CharSet::fromPattern(U"A\u00A2\U0001F600"_elv)));
        REQUIRE(view.containsOnly(CharSet::fromPattern(U"A\u00A2\U0001F600"_elv)));
        REQUIRE_EQUAL(text.count(U"\U0001F600"_elv), ElementCount{1U});
        REQUIRE_EQUAL(text.count(U"\u00A2\U0001F600"_elv), ElementCount{1U});
        REQUIRE_EQUAL(view.count(U"A"_elv), ElementCount{1U});
        REQUIRE_EQUAL(view.count(U32StringView{}), ElementCount::zero());
        REQUIRE_EQUAL(U"aaaa"_els.count(U"aa"_elv), ElementCount{2U});
        REQUIRE_FALSE(text.containsOnly(CharSet::fromPattern(U"A\u00A2"_elv)));
        REQUIRE_FALSE(view.containsOnly(CharSet{}));
        REQUIRE(text.containsOnly(CharSet::fromPattern(U"Aa\u00A2\U0001F600"_elv)));
        REQUIRE_EQUAL(text.count(U"a"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(view.count(U"a"_elv, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(text.count(U32StringView{}, Char::compareCaseFolded), ElementCount::zero());
    }

    void testConversionAndEncodeDecode() {
        const auto text = U32String{std::u32string_view{U"A\U0001F600"}};
        const auto expectedUtf8 = std::u8string{u8"A\U0001F600"};
        const auto expectedString =
            std::string{reinterpret_cast<const char *>(expectedUtf8.data()), expectedUtf8.size()};

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), expectedString);
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), std::u16string{u"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), std::u16string{u"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{text}.toU32String().storageId(), text.storageId());
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A\U0001F600"});
        const auto view = U32StringView{text};
        REQUIRE_EQUAL(StringConverter{view}.toStdU8String(), std::u8string{u8"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), std::u16string{u"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A\U0001F600"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"A\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{StringConverter{std::u16string_view{u"A\U0001F600"}}.toU32String()}.toStdU32String(),
            std::u32string{U"A\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{StringConverter{std::wstring{L"Wide"}}.toU32String()}.toStdString(), std::string{"Wide"});

        const auto encoded = StringEncoder{text}.encode(StringEncoding::Utf16, StringBomMode::Reject);
        const auto decoded = StringDecoder{encoded}.toU32String(StringEncoding::Utf16, StringBomMode::Reject);
        REQUIRE_EQUAL(StringConverter{decoded}.toStdU32String(), std::u32string{U"A\U0001F600"});
    }
};
