// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/AsciiCategory.hpp>
#include <erbsland/text/impl/PlatformU16StringAccess.hpp>
#include <erbsland/text/impl/UnsafeU16StringEditorAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringDecoder.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unit/U16DataIndex.hpp>
#include <erbsland/unit/U16DataLength.hpp>
#include <erbsland/unit/U16DataRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

using namespace el::text::literals;

using namespace el::text;
using namespace el::unit;

TESTED_TARGETS(U16StringEditor U16String U16StringLiteral U16StringEncodingTools U16StringModifyTools BooleanFormat)
class U16StringTest final : public el::UnitTest {
public:
    void testDefaultStringIsEmpty() {
        const auto text = U16StringEditor{};

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{});
        REQUIRE(StringConverter{text}.toStdU16String().empty());
        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{0});
        REQUIRE_EQUAL(text.characterLength(), CpLength::zero());
        REQUIRE_EQUAL(U16String{text}.characterLength(), CpLength::zero());
        REQUIRE(text.isValidUtf16());
        REQUIRE(text.charAt(StringSide::Front).isNull());
        REQUIRE(text.charAt(StringSide::Back).isNull());
        REQUIRE(text.charAt(text.indexAt(StringSide::Back)).isEndOfData());
        REQUIRE(text.charAt(U16DataIndex::noIndex()).isNoCodePoint());
        REQUIRE(text.slice(StringSide::Front, U16DataLength{1U}).isEmpty());
        REQUIRE(text.slice(StringSide::Back, U16DataLength{1U}).isEmpty());
        REQUIRE(U16String{text}.slice(StringSide::Front, U16DataLength{1U}).isEmpty());
        REQUIRE(text.slice(StringSide::Front, CpLength{1U}).isEmpty());
        const auto [emptyCharacter, emptyRemaining] = text.slice(StringSide::Front);
        REQUIRE(emptyCharacter.isEndOfData());
        REQUIRE(emptyRemaining.isEmpty());
        const auto [emptyViewCharacter, emptyViewRemaining] = U16String{text}.slice(StringSide::Front);
        REQUIRE(emptyViewCharacter.isEndOfData());
        REQUIRE(emptyViewRemaining.isEmpty());
        const auto [emptyCodePointCharacter, emptyCodePointRemaining] = text.slice(StringSide::Front);
        REQUIRE(emptyCodePointCharacter.isEndOfData());
        REQUIRE(emptyCodePointRemaining.isEmpty());
    }

    void testReadOnlyConstructionFactoriesAndSwap() {
        static_assert(std::is_constructible_v<U16String, std::u16string_view>);
        static_assert(!std::is_convertible_v<std::u16string_view, U16String>);

        REQUIRE_EQUAL(U16String{std::u16string_view{u"utf-16"}}, u"utf-16"_el);
        REQUIRE_EQUAL(U16String::fromCharacter(Char{U'\U0001F600'}, CpLength{2U}), u"😀😀"_el);
        REQUIRE_EQUAL(U16String::fromJoined({u"one"_el, u"-"_el, u"two"_el}), u"one-two"_el);
        REQUIRE_EQUAL(U16String::fromInteger(-42), u"-42"_el);
        REQUIRE_EQUAL(U16String::fromFloat(1.25), U16String{U16StringEditor::fromFloat(1.25)});
        REQUIRE_EQUAL(U16String::fromBoolean(true), u"true"_el);

        const auto bytes = el::mem::ByteBlock::fromVector(std::vector<std::uint8_t>{0x12U, 0x34U});
        REQUIRE_EQUAL(U16String::fromByteBlock(bytes), u"1234"_el);

        auto first = U16String{std::u16string_view{u"first"}};
        auto second = U16String{std::u16string_view{u"second"}};
        swap(first, second);
        REQUIRE_EQUAL(first, u"second"_el);
        REQUIRE_EQUAL(second, u"first"_el);
    }

    void testRepeatedAppend() {

        const auto source = U16StringEditor{std::u16string_view{u"xA\U0001F600"}};
        const auto view = U16String{source}.slice(U16DataRange{U16DataIndex{1U}, U16DataLength{3U}});

        auto repeatedText = U16StringEditor{};
        repeatedText.append(view, ItemCount{2U});
        REQUIRE_EQUAL(StringConverter{repeatedText}.toStdU16String(), std::u16string{u"A\U0001F600A\U0001F600"});
        REQUIRE_EQUAL(repeatedText.length(), U16DataLength{6U});
        REQUIRE_EQUAL(repeatedText.characterLength(), CpLength{4U});

        auto repeatedCharacter = U16StringEditor{};
        repeatedCharacter.append(Char{U'\U0001F600'}, CpLength{3U});
        REQUIRE_EQUAL(
            StringConverter{repeatedCharacter}.toStdU16String(), std::u16string{u"\U0001F600\U0001F600\U0001F600"});
        REQUIRE_EQUAL(repeatedCharacter.length(), U16DataLength{6U});
        REQUIRE_EQUAL(repeatedCharacter.characterLength(), CpLength{3U});
        REQUIRE_EQUAL(U16StringEditor::fromCharacter(Char{U'\U0001F600'}, CpLength{3U}), repeatedCharacter);
        REQUIRE_EQUAL(U16StringEditor::fromCharacter(Char{U'A'}), u"A"_el);

        auto appended = U16StringEditor{std::u16string_view{u">"}};
        appended.append(view, ItemCount{2U}).append(Char{U'!'}, CpLength{2U});
        REQUIRE_EQUAL(StringConverter{appended}.toStdU16String(), std::u16string{u">A\U0001F600A\U0001F600!!"});
        REQUIRE_EQUAL(appended.characterLength(), CpLength{7U});

        REQUIRE(U16StringEditor{}.append(view, ItemCount::zero()).isEmpty());
        REQUIRE(U16StringEditor{}.append(U16String{}, ItemCount{5U}).isEmpty());
        REQUIRE(U16StringEditor{}.append(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE(U16StringEditor::fromCharacter(Char{U'A'}, CpLength::zero()).isEmpty());
        REQUIRE(U16StringEditor::fromCharacter(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE(U16StringEditor::fromJoined({}).isEmpty());
        REQUIRE_EQUAL(U16StringEditor::fromJoined({u"solo"_el}), u"solo"_el);
        REQUIRE_EQUAL(
            StringConverter{U16StringEditor::fromJoined({u"prefix-"_el, u""_el, view, u"-suffix"_el})}.toStdU32String(),
            std::u32string{U"prefix-A\U0001F600-suffix"});
        REQUIRE_THROWS(U16StringEditor{}.append(view, ItemCount::infinite()));
        REQUIRE_THROWS(U16StringEditor::fromCharacter(Char{U'A'}, CpLength::infinite()));
    }

    void testBooleanConversion() {

        REQUIRE_EQUAL(U16StringEditor::fromBoolean(true), u"true"_el);
        REQUIRE_EQUAL(
            U16StringEditor::fromBoolean(
                false, BooleanFormat::trueFalse().setCapitalization(Capitalization::Uppercase)),
            u"FALSE"_el);
        REQUIRE_EQUAL(U16StringEditor::fromBoolean(true, BooleanFormat::yesNo()), u"yes"_el);
        REQUIRE_EQUAL(
            U16StringEditor::fromBoolean(true, BooleanFormat::onOff().setCapitalization(Capitalization::Titlecase)),
            u"On"_el);
    }

    void testStringFromStdU16StringViewAndStringCopy() {
        static_assert(std::is_constructible_v<U16StringEditor, std::u16string_view>);
        static_assert(!std::is_convertible_v<std::u16string_view, U16StringEditor>);

        const auto text = U16StringEditor{std::u16string_view{u"xA\U0001F600x"}};
        const auto view = U16String{text}.slice(U16DataRange{U16DataIndex{1U}, U16DataLength{3U}});
        const auto copy = view.copy();

        REQUIRE_EQUAL(copy, u"A\U0001F600"_el);
        REQUIRE_NOT_EQUAL(copy.storageId(), view.storageId());
    }

    void testUtf16CodeUnitIndexedRead() {
        const auto text = U16StringEditor{std::u16string_view{u"A\u00A2\U0001F600"}};
        const auto view = U16String{text};

        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{4});
        REQUIRE_EQUAL(text.characterLength(), CpLength{3});
        REQUIRE(text.isValidUtf16());
        REQUIRE_EQUAL(text.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(StringSide::Back).toRawValue(), U'\U0001F600');
        {
            const auto [character, remaining] = text.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU16String(), std::u16string{u"\u00A2\U0001F600"});
        }
        {
            const auto [character, remaining] = text.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU16String(), std::u16string{u"A\u00A2"});
        }
        {
            REQUIRE_EQUAL(view.characterLength(), CpLength{3});
            const auto [character, remaining] = view.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU16String(), std::u16string{u"\u00A2\U0001F600"});
        }
        {
            const auto single = U16StringEditor{std::u16string_view{u"\U0001F600"}};
            const auto singleView = U16String{single};
            const auto [character, remaining] = singleView.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE(remaining.isEmpty());
        }
        REQUIRE_EQUAL(text.charAt(U16DataIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(U16DataIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(text.charAt(U16DataIndex{2}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(text[U16DataIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(text[U16DataIndex{2}].toRawValue(), U'\U0001F600');
        REQUIRE(text.charAt(U16DataIndex{3}).isReplacement());
        REQUIRE(text.charAt(U16DataIndex{4}).isEndOfData());
        REQUIRE(text.charAt(U16DataIndex{5}).isNoCodePoint());
        REQUIRE(text.charAt(U16DataIndex{3}).isReplacement());
        REQUIRE_EQUAL(text.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(text.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(text.charAt(CpIndex{2}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(text[CpIndex{1}].toRawValue(), U'\u00A2');
        REQUIRE(text.charAt(CpIndex{3}).isEndOfData());
        REQUIRE(text.charAt(CpIndex{4}).isNoCodePoint());
        REQUIRE(text.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(text[CpIndex{3}].isEndOfData());
        REQUIRE(text[CpIndex::noIndex()].isNoCodePoint());

        REQUIRE_EQUAL(view[U16DataIndex{0}].toRawValue(), U'A');
        REQUIRE_EQUAL(view[U16DataIndex{2}].toRawValue(), U'\U0001F600');
        REQUIRE(view[U16DataIndex{4}].isEndOfData());
        REQUIRE(view[U16DataIndex::noIndex()].isNoCodePoint());
        REQUIRE_EQUAL(view.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(view.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(view.charAt(CpIndex{2}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(view[CpIndex{1}].toRawValue(), U'\u00A2');
        REQUIRE(view.charAt(CpIndex{3}).isEndOfData());
        REQUIRE(view.charAt(CpIndex{4}).isNoCodePoint());
        REQUIRE(view.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE(view[CpIndex{3}].isEndOfData());
        REQUIRE(view[CpIndex::noIndex()].isNoCodePoint());
    }

    void testAdvanceRetreatAndIndexConversion() {
        const auto text = U16StringEditor{std::u16string_view{u"A\u00A2\U0001F600"}};
        auto index = U16DataIndex::zero();

        REQUIRE(text.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE(text.advance(index, CpLength{2}));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});
        REQUIRE_FALSE(text.advance(index));

        REQUIRE(text.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE(text.retreat(index, CpLength::infinite()));
        REQUIRE(index.isZero());

        REQUIRE_EQUAL(text.indexAt(CpIndex{0}), U16DataIndex{0});
        REQUIRE_EQUAL(text.indexAt(CpIndex{1}), U16DataIndex{1});
        REQUIRE_EQUAL(text.indexAt(CpIndex{2}), U16DataIndex{2});
        REQUIRE_EQUAL(text.indexAt(CpIndex{3}), text.indexAt(StringSide::Back));
        REQUIRE_EQUAL(text.toCharIndex(U16DataIndex{3}), CpIndex{2});
        REQUIRE_EQUAL(text.toCharIndex(text.indexAt(StringSide::Back)), CpIndex{3});
    }

    void testIndexedSequentialRead() {
        const auto text = U16StringEditor{std::u16string_view{u"A\u00A2\U0001F600"}};
        auto index = U16DataIndex::zero();

        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});
        REQUIRE(text.readCharAndAdvance(index).isEndOfData());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{4});

        index = U16DataIndex{5U};
        REQUIRE(text.readCharAndAdvance(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        index = text.indexAt(StringSide::Back);
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'A');
        REQUIRE(index.isZero());
        REQUIRE(text.readCharAndRetreat(index).isEndOfData());
        REQUIRE(index.isZero());

        index = U16DataIndex{5U};
        REQUIRE(text.readCharAndRetreat(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{5});

        const auto view = U16String{text};
        index = U16DataIndex::zero();
        REQUIRE_EQUAL(view.readCharAndAdvance(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        index = view.indexAt(StringSide::Back);
        REQUIRE_EQUAL(view.readCharAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});

        const auto invalidData = std::u16string{u'A', static_cast<char16_t>(0xD800U), u'B'};
        const auto invalid = U16StringEditor{std::u16string_view{invalidData}};
        index = U16DataIndex{1U};
        REQUIRE(invalid.readCharAndAdvance(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE(invalid.readCharAndRetreat(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
    }

    void testMalformedSurrogatesAreTolerated() {

        const auto invalid = std::u16string{u'A', static_cast<char16_t>(0xD800U), u'B'};
        const auto text = U16StringEditor{std::u16string_view{invalid}};

        REQUIRE_FALSE(text.isValidUtf16());
        REQUIRE_EQUAL(text.length().toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(text.characterLength(), CpLength{3});
        REQUIRE_EQUAL(U16String{text}.characterLength(), CpLength{3});
        REQUIRE_EQUAL(text.count(u"\uFFFD"_el), ItemCount{1U});
        REQUIRE_EQUAL(U16String{text}.count(u"\uFFFD"_el, Char::compareCaseFolded), ItemCount{1U});
        REQUIRE_EQUAL(text.charAt(U16DataIndex{0}).toRawValue(), U'A');
        REQUIRE(text.charAt(U16DataIndex{1}).isReplacement());
        REQUIRE_EQUAL(text.charAt(U16DataIndex{2}).toRawValue(), U'B');
        REQUIRE(text.charAt(U16DataIndex{1}).isReplacement());
        REQUIRE_EQUAL(text.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(text.charAt(CpIndex{1}).isReplacement());
        REQUIRE(text[CpIndex{1}].isReplacement());
        REQUIRE_EQUAL(text.charAt(CpIndex{2}).toRawValue(), U'B');
        REQUIRE_EQUAL(U16String{text}.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE(U16String{text}.charAt(CpIndex{1}).isReplacement());
        REQUIRE(U16String{text}[CpIndex{1}].isReplacement());
        REQUIRE_EQUAL(U16String{text}.charAt(CpIndex{2}).toRawValue(), U'B');
    }

    void testNoIndexFindPositions() {

        const auto text = U16StringEditor{std::u16string_view{u"A\u00A2\u20AC\U0001F600"}};
        const auto view = U16String{text};
        const auto characters = CharSet{Char{U'A'}, Char{0x00A2U}};

        REQUIRE(text.find(u"A"_el, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(text.findFirstOf(characters, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(text.findFirstNotOf(characters, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(text.findLastOf(characters, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(text.findLastNotOf(characters, U16DataIndex::noIndex()).isNoIndex());

        REQUIRE(view.find(u"A"_el, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(view.findFirstOf(characters, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(view.findFirstNotOf(characters, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(view.findLastOf(characters, U16DataIndex::noIndex()).isNoIndex());
        REQUIRE(view.findLastNotOf(characters, U16DataIndex::noIndex()).isNoIndex());
    }

    void testLinearDecodedSearchForLongRepeatedPrefixes() {
        auto needleData = std::u16string(64U, u'a');
        needleData.back() = u'b';
        auto textData = std::u16string(4096U, u'a');
        textData.push_back(u'b');
        const auto needle = U16String{std::u16string_view{needleData}};
        const auto text = U16String{std::u16string_view{textData}};

        REQUIRE_EQUAL(text.find(needle), U16DataIndex{4033U});
        REQUIRE_EQUAL(text.count(needle), ItemCount{1U});

        needleData.back() = u'c';
        REQUIRE(text.find(U16String{std::u16string_view{needleData}}).isNoIndex());

        auto uppercaseNeedleData = std::u16string(64U, u'A');
        uppercaseNeedleData.back() = u'B';
        REQUIRE_EQUAL(
            text.find(U16String{std::u16string_view{uppercaseNeedleData}}, Char::compareAsciiFolded),
            U16DataIndex{4033U});
    }

    void testCaseMapping() {

        const auto mixed = U16StringEditor{std::u16string_view{u"A\u00C4\u03A3\u03C2K"}};
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::caseFolded)}.toStdU32String(), std::u32string{U"aäσσk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toLowercase)}.toStdU32String(), std::u32string{U"aäσςk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toUppercase)}.toStdU32String(), std::u32string{U"AÄΣΣK"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"aÄΣςk"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiUppercase)}.toStdU32String(), std::u32string{U"AÄΣςK"});

        const auto empty = U16StringEditor{};
        REQUIRE(empty.transformed(Char::toLowercase).isEmpty());
        REQUIRE(empty.transformed(Char::toAsciiUppercase).isEmpty());
        REQUIRE(U16String{}.transformed(Char::caseFolded).isEmpty());
        REQUIRE_EQUAL(mixed.transformed(nullptr), mixed);

        const auto unchanged = U16StringEditor{std::u16string_view{u"abc"}};
        REQUIRE_EQUAL(unchanged.transformed(Char::toAsciiLowercase).storageId(), unchanged.storageId());
        REQUIRE_EQUAL(unchanged.transformed(Char::caseFolded).storageId(), unchanged.storageId());

        const auto sharedView = U16String{unchanged};
        REQUIRE_EQUAL(sharedView.transformed(Char::toAsciiLowercase).storageId(), sharedView.storageId());
        REQUIRE_EQUAL(
            StringConverter{sharedView.transformed(Char::toAsciiLowercase)}.toStdU16String(), std::u16string{u"abc"});

        const auto literalView = U16String{u"abc"_el};
        REQUIRE_EQUAL(literalView.transformed(Char::toAsciiLowercase).storageId(), literalView.storageId());
        REQUIRE_EQUAL(
            StringConverter{literalView.transformed(Char::toAsciiLowercase)}.toStdU16String(), std::u16string{u"abc"});

        const auto slicedView = U16String{u"xabcx"_el}.slice(U16DataRange{U16DataIndex{1U}, U16DataLength{3U}});
        const auto slicedLower = slicedView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedLower}.toStdU16String(), std::u16string{u"abc"});

        const auto fullCodePointLower = literalView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{fullCodePointLower}.toStdU16String(), std::u16string{u"abc"});
        const auto slicedCodePointLower = slicedView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedCodePointLower}.toStdU16String(), std::u16string{u"abc"});

        const auto invalidOnlyData = std::u16string{u'1', static_cast<char16_t>(0xD800U)};
        const auto invalidOnly = U16StringEditor{std::u16string_view{invalidOnlyData}};
        REQUIRE_EQUAL(invalidOnly.transformed(Char::toAsciiLowercase).storageId(), invalidOnly.storageId());
        REQUIRE_EQUAL(
            StringConverter{U16String{invalidOnly}.transformed(Char::toAsciiLowercase)}.toStdU32String(),
            std::u32string{U"1\uFFFD"});

        const auto invalidData = std::u16string{u'A', static_cast<char16_t>(0xD800U), u'B'};
        const auto invalid = U16StringEditor{std::u16string_view{invalidData}};
        REQUIRE_EQUAL(
            StringConverter{invalid.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"a\uFFFDb"});
        REQUIRE_EQUAL(
            StringConverter{U16String{invalid}.transformed(Char::toAsciiLowercase)}.toStdU32String(),
            std::u32string{U"a\uFFFDb"});
    }

    void testSliceAndCodePointOperations() {
        const auto text = U16StringEditor{std::u16string_view{u"A\U0001F600BC"}};
        const auto face = text.slice(U16DataRange{U16DataIndex{1}, U16DataLength{2}});
        const auto chars = text;

        REQUIRE_EQUAL(StringConverter{face}.toStdU32String(), std::u32string{U"\U0001F600"});
        REQUIRE_EQUAL(chars.characterLength().toSizeT(), std::size_t{4});
        REQUIRE_EQUAL(chars.charAt(StringSide::Front).toRawValue(), U'A');
        REQUIRE_EQUAL(chars.charAt(StringSide::Back).toRawValue(), U'C');
        {
            const auto [character, remaining] = chars.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU32String(), std::u32string{U"\U0001F600BC"});
        }
        {
            const auto [character, remaining] = chars.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'C');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU32String(), std::u32string{U"A\U0001F600B"});
        }
        REQUIRE_EQUAL(chars.charAt(CpIndex{1}).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(
            StringConverter{chars.slice(CpRange{CpIndex{1}, CpLength{2}})}.toStdU32String(),
            std::u32string{U"\U0001F600B"});

        const auto unicode = U16StringEditor{std::u16string_view{u"A\u00A2\u20AC\U0001F600BC"}};
        const auto unicodeView = U16String{unicode};
        const auto unicodeChars = unicode;
        const auto range = CpRange{CpIndex{1}, CpLength{3}};

        REQUIRE_EQUAL(
            StringConverter{unicode.slice(range)}.toStdU32String(), std::u32string{U"\u00A2\u20AC\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(range)}.toStdU32String(),
            StringConverter{unicodeChars.slice(range)}.toStdU32String());
        REQUIRE(unicode.slice(U16DataRange{U16DataIndex{2}, U16DataLength::zero()}).isEmpty());
        REQUIRE(unicode.slice(U16DataRange{U16DataIndex::noIndex(), U16DataLength{1}}).isEmpty());
        REQUIRE(unicode.slice(U16DataRange::noRange()).isEmpty());
        REQUIRE(unicode.slice(U16DataRange{U16DataIndex{99}, U16DataLength{1}}).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(U16DataRange{U16DataIndex{2}, U16DataLength::infinite()})}.toStdU32String(),
            std::u32string{U"\u20AC\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(U16DataRange{U16DataIndex{1}, U16DataLength{2}})}.toStdU32String(),
            std::u32string{U"\u00A2\u20AC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, U16DataLength{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, U16DataLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Front, U16DataLength::zero()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, U16DataLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Back, U16DataLength::zero()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, U16DataIndex{3})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, U16DataIndex{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, U16DataIndex{5})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, U16DataIndex{5})}.toStdU32String(), std::u32string{U"BC"});
        REQUIRE(unicodeView.slice(StringSide::Front, U16DataIndex::zero()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Back, U16DataIndex::zero())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, unicodeView.indexAt(StringSide::Back))}
                .toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicodeView.slice(StringSide::Back, unicodeView.indexAt(StringSide::Back)).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, U16DataIndex::noIndex())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicodeView.slice(StringSide::Back, U16DataIndex::noIndex()).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Front, U16DataIndex{99})}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicodeView.slice(StringSide::Back, U16DataIndex{99}).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpLength{4})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpLength{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Front, CpIndex{4})}.toStdU32String(),
            std::u32string{U"A\u00A2\u20AC\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{unicode.slice(StringSide::Back, CpIndex{4})}.toStdU32String(), std::u32string{U"BC"});
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
            StringConverter{unicodeChars.slice(StringSide::Front, CpLength::infinite())}.toStdU32String(),
            StringConverter{unicode}.toStdU32String());
        REQUIRE(unicode.slice(StringSide::Front, CpLength::zero()).isEmpty());
        REQUIRE(unicodeChars.slice(StringSide::Front, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(StringSide::Back, CpLength::zero()).isEmpty());
        REQUIRE(unicode.slice(CpRange::noRange()).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex::noIndex(), CpLength{1}}).isEmpty());
        REQUIRE(unicode.slice(CpRange{CpIndex{99}, CpLength{1}}).isEmpty());
        REQUIRE_EQUAL(
            StringConverter{unicodeView.slice(StringSide::Back, CpLength{3})}.toStdU32String(),
            std::u32string{U"\U0001F600BC"});

        const auto source = U16StringEditor{std::u16string_view{u"xxA\u00A2\u20AC\U0001F600BCyy"}};
        const auto nestedView = U16String{source}.slice(U16DataRange{U16DataIndex{2}, U16DataLength{7}});
        REQUIRE_EQUAL(
            StringConverter{nestedView.slice(CpRange{CpIndex{1}, CpLength{4}}).slice(StringSide::Back, CpLength{2})}
                .toStdU32String(),
            std::u32string{U"\U0001F600B"});

        {
            const auto [left, right] = unicode.splitAt(U16DataIndex{3});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), std::u32string{U"A\u00A2\u20AC"});
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), std::u32string{U"\U0001F600BC"});
        }
        {
            const auto [left, right] = unicode.splitAt(U16DataIndex{5});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), std::u32string{U"A\u00A2\u20AC\U0001F600"});
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), std::u32string{U"BC"});
        }
        {
            const auto [left, right] = unicode.splitAt(CpIndex{4});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), std::u32string{U"A\u00A2\u20AC\U0001F600"});
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), std::u32string{U"BC"});
        }
        {
            const auto [left, right] = unicodeView.splitAt(U16DataIndex::zero());
            REQUIRE(left.isEmpty());
            REQUIRE_EQUAL(StringConverter{right}.toStdU32String(), StringConverter{unicode}.toStdU32String());
        }
        {
            const auto [left, right] = unicodeView.splitAt(unicodeView.indexAt(StringSide::Back));
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), StringConverter{unicode}.toStdU32String());
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = unicodeView.splitAt(U16DataIndex::noIndex());
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), StringConverter{unicode}.toStdU32String());
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = unicodeView.splitAt(U16DataIndex{99});
            REQUIRE_EQUAL(StringConverter{left}.toStdU32String(), StringConverter{unicode}.toStdU32String());
            REQUIRE(right.isEmpty());
        }
    }

    void testPredicateChecks() {

        const auto text = U16StringEditor{std::u16string_view{u"A\u00A2\U0001F600"}};
        const auto view = U16String{text};
        const auto chars = view;

        REQUIRE(text.containsOnly(CharSet::fromPattern(u"A\u00A2\U0001F600"_el)));
        REQUIRE(view.containsOnly(CharSet::fromPattern(u"A\u00A2\U0001F600"_el)));
        REQUIRE(chars.containsOnly(CharSet::fromPattern(u"A\u00A2\U0001F600"_el)));
        REQUIRE_EQUAL(text.count(u"\U0001F600"_el), ItemCount{1U});
        REQUIRE_EQUAL(text.count(u"\u00A2\U0001F600"_el), ItemCount{1U});
        REQUIRE_EQUAL(view.count(u"A"_el), ItemCount{1U});
        REQUIRE_EQUAL(view.count(U16String{}), ItemCount::zero());
        REQUIRE_EQUAL(U16String{u"aaaa"_el}.count(u"aa"_el), ItemCount{2U});
        REQUIRE_FALSE(text.containsOnly(CharSet::fromPattern(u"A\u00A2"_el)));
        REQUIRE_FALSE(chars.containsOnly(CharSet{}));
        REQUIRE(U16StringEditor{std::u16string_view{u"Alpha_09"}}.containsOnly(AsciiCategory::Word));
        REQUIRE(U16String{u"Alpha-09"_el}.containsOnly(AsciiCategory::WordWithHyphen));
        REQUIRE(U16String{}.containsOnly(AsciiCategory::Word));
        REQUIRE_FALSE(U16String{u"Alpha.09"_el}.containsOnly(AsciiCategory::WordWithHyphen));
        REQUIRE_FALSE(U16String{u"Alpha\u00E4"_el}.containsOnly(AsciiCategory::Word));
        const auto malformedCategoryData = std::u16string{u'A', static_cast<char16_t>(0xD800U), u'B'};
        const auto malformedCategoryText = U16StringEditor{std::u16string_view{malformedCategoryData}};
        REQUIRE_FALSE(malformedCategoryText.containsOnly(AsciiCategory::Word));
        REQUIRE_FALSE(U16String{malformedCategoryText}.containsOnly(AsciiCategory::Word));

        const auto mixed = U16StringEditor{std::u16string_view{u"\u00C4xK"}};
        const auto mixedView = U16String{mixed};
        REQUIRE(mixed.containsOnly(CharSet::fromPattern(u"\u00C4\u00E4xXkK"_el)));
        REQUIRE(mixedView.containsOnly(CharSet::fromPattern(u"\u00C4\u00E4xXkK"_el)));
        REQUIRE_EQUAL(mixed.count(u"\u00E4"_el, Char::compareCaseFolded), ItemCount{1U});
        REQUIRE_EQUAL(mixed.count(u"k"_el, Char::compareCaseFolded), ItemCount{1U});
        REQUIRE_EQUAL(mixed.count(U16String{}, Char::compareCaseFolded), ItemCount::zero());
        REQUIRE_EQUAL(mixedView.count(u"X"_el, Char::compareCaseFolded), ItemCount{1U});
        REQUIRE(mixed.containsOnly(CharSet::fromPattern(u"\u00C4\u00E4xXkK"_el)));
        REQUIRE_FALSE(mixed.containsOnly(CharSet::fromPattern(u"\u00C4\u00E4xX"_el)));
    }

    void testCustomComparisonChecks() {

        const auto asciiFoldedCompare =
            CharCompareFn{[](const Char left, const Char right) noexcept -> std::strong_ordering {
                return left.toAsciiLowercase() <=> right.toAsciiLowercase();
            }};
        const auto reverseCompare = CharCompareFn{
            [](const Char left, const Char right) noexcept -> std::strong_ordering { return right <=> left; }};

        const auto text = U16String{u"Alpha"_el};
        const auto view = U16String{u"ALPHA"_el};
        REQUIRE_EQUAL(text.compare(view, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(view.compare(text, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(U16String{u"a"_el}.compare(u"b"_el, reverseCompare), std::strong_ordering::greater);
    }

    void testModificationAndConversion() {

        auto text = U16StringEditor{std::u16string_view{u"Hello"}};
        text.append(Char{U'!'});
        text.replaceAll(u"ll"_el, u"yy"_el);
        text.remove(CpRange{CpIndex{0}, CpLength{1}});

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"eyyo!"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8"eyyo!"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), std::u16string{u"eyyo!"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8"eyyo!"});
        REQUIRE_EQUAL(StringConverter{text}.toU16String().storageId(), text.storageId());
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), std::u16string{u"eyyo!"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"eyyo!"});

        const auto view = U16String{text};
        REQUIRE_EQUAL(StringConverter{view}.toStdU8String(), std::u8string{u8"eyyo!"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), std::u16string{u"eyyo!"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU16String(), std::u16string{u"eyyo!"});
        REQUIRE_EQUAL(StringConverter{view}.toStdU32String(), std::u32string{U"eyyo!"});
    }

    void testRangeInsertReplaceAndFirstModifiers() {

        const auto source = U16StringEditor{std::u16string_view{u"A\U0001F600BC"}};

        REQUIRE_EQUAL(
            StringConverter{source.removed(U16DataRange{U16DataIndex{1U}, U16DataLength{2U}})}.toStdU16String(),
            std::u16string{u"ABC"});
        REQUIRE_EQUAL(
            StringConverter{source.kept(U16DataRange{U16DataIndex{1U}, U16DataLength{2U}})}.toStdU16String(),
            std::u16string{u"\U0001F600"});
        REQUIRE_EQUAL(
            StringConverter{source.inserted(U16DataIndex{1U}, u"X"_el)}.toStdU16String(),
            std::u16string{u"AX\U0001F600BC"});
        REQUIRE_EQUAL(
            StringConverter{source.inserted(CpIndex{2U}, u"X"_el)}.toStdU16String(), std::u16string{u"A\U0001F600XBC"});
        REQUIRE_EQUAL(
            StringConverter{source.replaced(U16DataRange{U16DataIndex{1U}, U16DataLength{2U}}, u"X"_el)}
                .toStdU16String(),
            std::u16string{u"AXBC"});
        REQUIRE_EQUAL(
            StringConverter{source.replaced(CpRange{CpIndex{1U}, CpLength{1U}}, u"X"_el)}.toStdU16String(),
            std::u16string{u"AXBC"});

        auto text = U16StringEditor{std::u16string_view{u"abef"}};
        text.reserve(U16DataLength{16U});
        const auto *data = el::text::impl::UnsafeU16StringEditorAccess{text}.dataSpan().data();

        text.insert(U16DataIndex{2U}, u"cd"_el);
        text.replace(U16DataRange{U16DataIndex{2U}, U16DataLength{2U}}, u"XY"_el);
        text.insert(U16DataIndex{99U}, u"!"_el);
        text.insert(U16DataIndex::noIndex(), u"?"_el);
        text.replace(U16DataRange::noRange(), u"?"_el);

        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), std::u16string{u"abXYef!"});
        REQUIRE_EQUAL(el::text::impl::UnsafeU16StringEditorAccess{text}.dataSpan().data(), data);
        REQUIRE_EQUAL(
            el::text::impl::PlatformU16StringAccess{text}.nullTerminatedCharPtr()[text.length().toSizeT()], u'\0');

        auto firstText = U16StringEditor{std::u16string_view{u"one one"}};
        firstText.removeFirst(u"one"_el);
        REQUIRE_EQUAL(StringConverter{firstText}.toStdU16String(), std::u16string{u" one"});
        firstText.replaceFirst(u"ONE"_el, u"two"_el, Char::compareCaseFolded);
        REQUIRE_EQUAL(StringConverter{firstText}.toStdU16String(), std::u16string{u" two"});

        REQUIRE_EQUAL(StringConverter{source.removedFirst(u"\U0001F600"_el)}.toStdU16String(), std::u16string{u"ABC"});
        REQUIRE_EQUAL(
            StringConverter{source.replacedFirst(u"\U0001F600"_el, u"X"_el)}.toStdU16String(), std::u16string{u"AXBC"});
        REQUIRE_EQUAL(
            StringConverter{source.removedFirst(U16String{})}.toStdU16String(),
            StringConverter{source}.toStdU16String());
    }

    void testViewCodePointAliasingAndMalformedModifiers() {

        const auto source = U16StringEditor{std::u16string_view{u"--alpha--"}};
        const auto view = U16String{source}.slice(U16DataRange{U16DataIndex{2U}, U16DataLength{5U}});
        const auto charView = view;

        REQUIRE_EQUAL(
            StringConverter{view.inserted(U16DataIndex{2U}, u"!"_el)}.toStdU16String(), std::u16string{u"al!pha"});
        REQUIRE_EQUAL(
            StringConverter{view.replaced(U16DataRange{U16DataIndex{2U}, U16DataLength{2U}}, u"X"_el)}.toStdU16String(),
            std::u16string{u"alXa"});
        REQUIRE_EQUAL(StringConverter{view.removedFirst(u"ph"_el)}.toStdU16String(), std::u16string{u"ala"});
        REQUIRE_EQUAL(
            StringConverter{view.replacedFirst(u"AL"_el, u"AL"_el, Char::compareCaseFolded)}.toStdU16String(),
            std::u16string{u"ALpha"});

        REQUIRE_EQUAL(
            StringConverter{charView.inserted(CpIndex{2U}, u"!"_el)}.toStdU16String(), std::u16string{u"al!pha"});
        REQUIRE_EQUAL(
            StringConverter{charView.replaced(CpRange{CpIndex{2U}, CpLength{2U}}, u"X"_el)}.toStdU16String(),
            std::u16string{u"alXa"});
        REQUIRE_EQUAL(StringConverter{charView.removedFirst(u"ph"_el)}.toStdU16String(), std::u16string{u"ala"});
        REQUIRE_EQUAL(
            StringConverter{charView.replacedFirst(u"AL"_el, u"AL"_el, Char::compareCaseFolded)}.toStdU16String(),
            std::u16string{u"ALpha"});

        auto insertAlias = U16StringEditor{std::u16string_view{u"abcdef"}};
        insertAlias.reserve(U16DataLength{16U});
        const auto insertView = U16String{insertAlias}.slice(U16DataRange{U16DataIndex{1U}, U16DataLength{3U}});
        insertAlias.insert(U16DataIndex{3U}, insertView);
        REQUIRE_EQUAL(StringConverter{insertAlias}.toStdU16String(), std::u16string{u"abcbcddef"});

        auto replaceAlias = U16StringEditor{std::u16string_view{u"abcdef"}};
        replaceAlias.reserve(U16DataLength{16U});
        const auto replaceView = U16String{replaceAlias}.slice(U16DataRange{U16DataIndex{1U}, U16DataLength{4U}});
        replaceAlias.replace(U16DataRange{U16DataIndex{2U}, U16DataLength{2U}}, replaceView);
        REQUIRE_EQUAL(StringConverter{replaceAlias}.toStdU16String(), std::u16string{u"abbcdeef"});

        auto base = U16StringEditor{std::u16string_view{u"xxabcdefyy"}};
        auto sliced = base.slice(U16DataRange{U16DataIndex{2U}, U16DataLength{6U}});
        sliced.insert(U16DataIndex{3U}, u"!"_el);
        REQUIRE_EQUAL(StringConverter{sliced}.toStdU16String(), std::u16string{u"abc!def"});
        REQUIRE_EQUAL(StringConverter{base}.toStdU16String(), std::u16string{u"xxabcdefyy"});

        const auto malformedData = std::u16string{u'A', static_cast<char16_t>(0xD800U), u'B'};
        auto malformed = U16StringEditor{std::u16string_view{malformedData}};
        malformed.replace(U16DataRange{U16DataIndex{1U}, U16DataLength{1U}}, u"?"_el);
        malformed.insert(U16DataIndex{99U}, u"!"_el);
        REQUIRE_EQUAL(StringConverter{malformed}.toStdU16String(), std::u16string{u"A?B!"});
    }

    void testEncodeDecode() {
        const auto text = U16StringEditor{std::u16string_view{u"A\U0001F600"}};

        const auto encoded = StringEncoder{text}.encode(StringEncoding::Utf8, StringBomMode::Reject);
        const auto decoded = StringDecoder{encoded}.toU16String(StringEncoding::Utf8, StringBomMode::Reject);

        REQUIRE_EQUAL(StringConverter{decoded}.toStdU16String(), std::u16string{u"A\U0001F600"});
    }
};
