// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/impl/UnsafeU8StringEditorAccess.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/impl/U8StringData.hpp>
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
#include <string>
#include <string_view>
#include <type_traits>

using namespace el::text::literals;

using namespace el::text;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8StringEditor U8String U8StringLiteral U8StringComparisonTools BooleanFormat)
class U8StringEditorTest final : public el::UnitTest {
public:
    void testDefaultStringIsEmpty() {
        const auto text = U8StringEditor{};

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
        static_assert(std::is_constructible_v<U8StringEditor, std::string_view>);
        static_assert(!std::is_convertible_v<std::string_view, U8StringEditor>);

        const auto text = U8StringEditor{std::string_view{"Hello"}};

        REQUIRE_FALSE(text.isEmpty());
        REQUIRE_EQUAL(text, "Hello"_el);
    }

    void testStringFromStdU8String() {
        static_assert(std::is_constructible_v<U8StringEditor, std::u8string_view>);
        static_assert(!std::is_convertible_v<std::u8string_view, U8StringEditor>);

        const auto text = U8StringEditor{std::u8string_view{u8"Hello"}};

        REQUIRE_FALSE(text.isEmpty());
        REQUIRE_EQUAL(text, u8"Hello"_el);
    }

    void testStringCopiesLiteral() {

        const auto text = "Hello"_el;

        REQUIRE_FALSE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
    }

    void testRepeatedAppend() {

        const auto source = U8StringEditor{std::u8string_view{u8"xA¢"}};
        const auto view = U8String{source}.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}});

        auto repeatedText = U8StringEditor{};
        repeatedText.append(view, ElementCount{3U});
        REQUIRE_EQUAL(StringConverter{repeatedText}.toStdU8String(), std::u8string{u8"A¢A¢A¢"});
        REQUIRE_EQUAL(repeatedText.length(), ByteLength{9U});
        REQUIRE_EQUAL(repeatedText.characterLength(), CpLength{6U});

        auto repeatedCharacter = U8StringEditor{};
        repeatedCharacter.append(Char{U'\u20AC'}, CpLength{3U});
        REQUIRE_EQUAL(StringConverter{repeatedCharacter}.toStdU8String(), std::u8string{u8"€€€"});
        REQUIRE_EQUAL(repeatedCharacter.length(), ByteLength{9U});
        REQUIRE_EQUAL(repeatedCharacter.characterLength(), CpLength{3U});
        REQUIRE_EQUAL(U8StringEditor::fromCharacter(Char{U'\u20AC'}, CpLength{3U}), repeatedCharacter);
        REQUIRE_EQUAL(U8StringEditor::fromCharacter(Char{U'A'}), "A"_el);

        REQUIRE(U8StringEditor{}.append(view, ElementCount::zero()).isEmpty());
        REQUIRE(U8StringEditor{}.append(""_el, ElementCount{5U}).isEmpty());
        REQUIRE(U8StringEditor{}.append(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE(U8StringEditor::fromCharacter(Char{U'A'}, CpLength::zero()).isEmpty());
        REQUIRE(U8StringEditor::fromCharacter(Char::noCodePoint(), CpLength{5U}).isEmpty());
        REQUIRE(U8StringEditor::fromJoined({}).isEmpty());
        REQUIRE_EQUAL(U8StringEditor::fromJoined({"solo"_el}), "solo"_el);
        REQUIRE_EQUAL(U8StringEditor::fromJoined({"prefix-"_el, ""_el, view, "-suffix"_el}), u8"prefix-A¢-suffix"_el);
        REQUIRE_THROWS(U8StringEditor{}.append("x"_el, ElementCount::infinite()));
        REQUIRE_THROWS(U8StringEditor::fromCharacter(Char{U'A'}, CpLength::infinite()));
    }

    void testBooleanConversion() {

        REQUIRE_EQUAL(U8StringEditor::fromBoolean(true), "true"_el);
        REQUIRE_EQUAL(
            U8StringEditor::fromBoolean(false, BooleanFormat::yesNo().setCapitalization(Capitalization::Uppercase)),
            "NO"_el);
        REQUIRE_EQUAL(
            U8StringEditor::fromBoolean(
                true, BooleanFormat::enabledDisabled().setCapitalization(Capitalization::Titlecase)),
            "Enabled"_el);
    }

    void testClear() {
        auto text = U8StringEditor{std::string_view{"Hello"}};
        text.reserve(ByteLength{8U});

        text.clear();

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{});
        REQUIRE_EQUAL(text.capacity(), ByteLength{8U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(8U));
    }

    void testResetReleasesReservedStorage() {
        auto text = U8StringEditor{std::string_view{"Hello"}};
        text.reserve(ByteLength{8U});

        text.reset();

        REQUIRE(text.isEmpty());
        REQUIRE(text.capacity().isZero());
        REQUIRE(text.memoryUsage().isZero());
    }

    void testByteIndexedRead() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};

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
            const auto view = U8String{text};
            const auto [character, remaining] = view.slice(StringSide::Front);
            REQUIRE_EQUAL(character.toRawValue(), U'A');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"¢€😀"});
        }
        {
            const auto charView = U8String{text};
            const auto [character, remaining] = charView.slice(StringSide::Back);
            REQUIRE_EQUAL(character.toRawValue(), U'\U0001F600');
            REQUIRE_EQUAL(StringConverter{remaining}.toStdU8String(), std::u8string{u8"A¢€"});
        }
        {
            const auto single = U8StringEditor{std::u8string_view{u8"€"}};
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

    void testIndexedSequentialRead() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        auto index = ByteIndex::zero();

        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});
        REQUIRE_EQUAL(text.readCharAndAdvance(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});
        REQUIRE(text.readCharAndAdvance(index).isEndOfData());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{10});

        index = ByteIndex{11U};
        REQUIRE(text.readCharAndAdvance(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{11});

        index = ByteIndex::noIndex();
        REQUIRE(text.readCharAndAdvance(index).isNoCodePoint());
        REQUIRE(index.isNoIndex());

        index = text.indexAt(StringSide::Back);
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{3});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(text.readCharAndRetreat(index).toRawValue(), U'A');
        REQUIRE(index.isZero());
        REQUIRE(text.readCharAndRetreat(index).isEndOfData());
        REQUIRE(index.isZero());

        index = ByteIndex{11U};
        REQUIRE(text.readCharAndRetreat(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{11});

        const auto view = U8String{text};
        index = ByteIndex::zero();
        REQUIRE_EQUAL(view.readCharAndAdvance(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
        index = view.indexAt(StringSide::Back);
        REQUIRE_EQUAL(view.readCharAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{6});

        const auto invalid = U8StringEditor{std::string_view{invalidUtf8Data()}};
        index = ByteIndex{1U};
        REQUIRE(invalid.readCharAndAdvance(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{2});
        REQUIRE(invalid.readCharAndRetreat(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), std::size_t{1});
    }

    void testCaseMapping() {

        const auto mixed = U8StringEditor{std::u8string_view{u8"AÄΣςK"}};
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::caseFolded)}.toStdU32String(), std::u32string{U"aäσσk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toLowercase)}.toStdU32String(), std::u32string{U"aäσςk"});
        REQUIRE_EQUAL(StringConverter{mixed.transformed(Char::toUppercase)}.toStdU32String(), std::u32string{U"AÄΣΣK"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiLowercase)}.toStdU32String(), std::u32string{U"aÄΣςk"});
        REQUIRE_EQUAL(
            StringConverter{mixed.transformed(Char::toAsciiUppercase)}.toStdU32String(), std::u32string{U"AÄΣςK"});

        const auto empty = U8StringEditor{};
        REQUIRE(empty.transformed(Char::toLowercase).isEmpty());
        REQUIRE(empty.transformed(Char::toAsciiUppercase).isEmpty());
        REQUIRE(U8String{}.transformed(Char::caseFolded).isEmpty());
        REQUIRE_EQUAL(mixed.transformed(nullptr), mixed);

        const auto unchanged = U8StringEditor{std::string_view{"abc"}};
        REQUIRE_EQUAL(unchanged.transformed(Char::toAsciiLowercase).storageId(), unchanged.storageId());
        REQUIRE_EQUAL(unchanged.transformed(Char::caseFolded).storageId(), unchanged.storageId());

        const auto sharedView = U8String{unchanged};
        REQUIRE_EQUAL(
            StringConverter{sharedView.transformed(Char::toAsciiLowercase)}.toStdString(), std::string{"abc"});

        const auto literalView = U8String{"abc"_el};
        REQUIRE_EQUAL(
            StringConverter{literalView.transformed(Char::toAsciiLowercase)}.toStdString(), std::string{"abc"});

        const auto slicedView = String{"xabcx"_el}.slice(ByteRange{ByteIndex{1U}, ByteLength{3U}});
        const auto slicedLower = slicedView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedLower}.toStdString(), std::string{"abc"});

        const auto fullCodePointLower = literalView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{fullCodePointLower}.toStdString(), std::string{"abc"});
        const auto slicedCodePointLower = slicedView.transformed(Char::toAsciiLowercase);
        REQUIRE_EQUAL(StringConverter{slicedCodePointLower}.toStdString(), std::string{"abc"});

        auto invalidOnlyBytes = std::string{"1"};
        invalidOnlyBytes.push_back(static_cast<char>(0xC0U));
        const auto invalidOnly = U8StringEditor{std::string_view{invalidOnlyBytes}};
        REQUIRE_EQUAL(invalidOnly.transformed(Char::toAsciiLowercase).storageId(), invalidOnly.storageId());
        REQUIRE_EQUAL(
            StringConverter{U8String{invalidOnly}.transformed(Char::toAsciiLowercase)}.toStdString(),
            th::stdStringFromHex("31 EF BF BD"));

        auto invalidBytes = std::string{"A"};
        invalidBytes.push_back(static_cast<char>(0xC0U));
        invalidBytes.push_back('B');
        const auto invalid = U8StringEditor{std::string_view{invalidBytes}};
        REQUIRE_EQUAL(
            StringConverter{invalid.transformed(Char::toAsciiLowercase)}.toStdString(),
            th::stdStringFromHex("61 EF BF BD 62"));
        REQUIRE_EQUAL(
            StringConverter{U8String{invalid}.transformed(Char::toAsciiLowercase)}.toStdString(),
            th::stdStringFromHex("61 EF BF BD 62"));
    }

    void testAdvance() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
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
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
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
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};

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
        const auto text = U8StringEditor{std::string_view{data}};

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
        const auto text = U8StringEditor{std::string_view{"abcdef"}};

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
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Front, ByteLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Front, ByteLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(text.slice(StringSide::Front, ByteLength{0}).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteLength{99})}.toStdString(), "abcdef");
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteLength::infinite())}.toStdString(), "abcdef");
        REQUIRE(text.slice(StringSide::Back, ByteLength{0}).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Front, ByteIndex{2})}.toStdString(), "ab");
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteIndex{2})}.toStdString(), "cdef");
        REQUIRE(text.slice(StringSide::Front, ByteIndex::zero()).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Back, ByteIndex::zero())}.toStdString(), "abcdef");
        REQUIRE_EQUAL(
            StringConverter{text.slice(StringSide::Front, text.indexAt(StringSide::Back))}.toStdString(), "abcdef");
        REQUIRE(text.slice(StringSide::Back, text.indexAt(StringSide::Back)).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Front, ByteIndex::noIndex())}.toStdString(), "abcdef");
        REQUIRE(text.slice(StringSide::Back, ByteIndex::noIndex()).isEmpty());
        REQUIRE_EQUAL(StringConverter{text.slice(StringSide::Front, ByteIndex{99})}.toStdString(), "abcdef");
        REQUIRE(text.slice(StringSide::Back, ByteIndex{99}).isEmpty());

        const auto unicode = U8StringEditor{std::u8string_view{u8"A¢€😀BC"}};
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
            const auto [left, right] = text.splitAt(ByteIndex{2});
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "ab");
            REQUIRE_EQUAL(StringConverter{right}.toStdString(), "cdef");
        }
        {
            const auto [left, right] = text.splitAt(ByteIndex::zero());
            REQUIRE(left.isEmpty());
            REQUIRE_EQUAL(StringConverter{right}.toStdString(), "abcdef");
        }
        {
            const auto [left, right] = text.splitAt(text.indexAt(StringSide::Back));
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "abcdef");
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = text.splitAt(ByteIndex::noIndex());
            REQUIRE_EQUAL(StringConverter{left}.toStdString(), "abcdef");
            REQUIRE(right.isEmpty());
        }
        {
            const auto [left, right] = text.splitAt(ByteIndex{99});
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
        const auto first = text.slice(ByteRange{ByteIndex{1}, ByteLength{4}});
        const auto second = first.slice(ByteRange{ByteIndex{1}, ByteLength{2}});

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), "bcde");
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), "cd");
    }

    void testSliceThroughUtf8Sequence() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€"}};
        const auto slice = text.slice(ByteRange{ByteIndex{2}, ByteLength{2}});

        REQUIRE_EQUAL(th::toStdU32String(StringConverter{slice}.toStdString()), std::u32string{U"\uFFFD\uFFFD"});
        REQUIRE_FALSE(slice.isValidUtf8());
        REQUIRE(slice.charAt(ByteIndex::zero()).isReplacement());
    }

    void testPredicateChecks() {

        const auto text =
            U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE(text.startsWith(u8"A¢"_el));
        REQUIRE(text.startsWith(u8"A"_el));
        REQUIRE(text.startsWith(U8String{}));
        REQUIRE_FALSE(text.startsWith(u8"¢"_el));
        REQUIRE(text.endsWith(u8"😀"_el));
        REQUIRE(text.endsWith(U8String{}));
        REQUIRE_FALSE(text.endsWith(u8"€"_el));
        REQUIRE(text.contains(u8"¢€"_el));
        REQUIRE(text.contains(u8"€"_el));
        REQUIRE(text.contains(U8String{}));
        REQUIRE_FALSE(text.contains(u8"€¢"_el));
        REQUIRE_EQUAL(text.count(u8"¢€"_el), ElementCount{1U});
        REQUIRE_EQUAL(text.count(u8"€"_el), ElementCount{1U});
        REQUIRE_EQUAL(text.count(U8String{}), ElementCount::zero());
        REQUIRE_EQUAL(U8String{u8"¢€¢€"_el}.count(u8"¢€"_el), ElementCount{2U});
        REQUIRE_EQUAL(String{"aaaa"_el}.count("aa"_el), ElementCount{2U});
        REQUIRE(text.containsOneOf(CharSet{u8"z😀"_el}));
        REQUIRE(text.containsOneOf(CharSet{Char{0x20ACU}}));
        REQUIRE_FALSE(text.containsOneOf(CharSet{"xyz"_el}));
        REQUIRE_FALSE(text.containsOneOf(CharSet{}));
        REQUIRE(text.containsOnly(CharSet{u8"A¢€😀"_el}));
        REQUIRE_FALSE(text.containsOnly(CharSet{u8"A¢€"_el}));
        REQUIRE_FALSE(text.containsOnly(CharSet{}));
    }

    void testComparisonChecks() {

        const auto first = U8StringEditor{std::u8string_view{u8"A¢"}};
        const auto same = U8StringEditor{std::u8string_view{u8"A¢"}};
        const auto greater = U8StringEditor{std::u8string_view{u8"A€"}};
        const auto view = U8String{same};
        const auto charView = view;
        const auto asciiFoldedCompare =
            CharCompareFn{[](const Char left, const Char right) noexcept -> std::strong_ordering {
                return left.toAsciiLowercase() <=> right.toAsciiLowercase();
            }};
        const auto reverseCompare = CharCompareFn{
            [](const Char left, const Char right) noexcept -> std::strong_ordering { return right <=> left; }};

        REQUIRE_EQUAL(first.compare(same), std::strong_ordering::equal);
        REQUIRE_EQUAL(first.compare(view), std::strong_ordering::equal);
        REQUIRE_EQUAL(first.compare(u8"a¢"_el, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(view.compare(u8"a¢"_el, asciiFoldedCompare), std::strong_ordering::equal);
        REQUIRE_EQUAL(U8String{u8"a"_el}.compare(u8"b"_el, reverseCompare), std::strong_ordering::greater);
        REQUIRE(first.compare(greater) < 0);
        REQUIRE(view.compare(first) == std::strong_ordering::equal);
        REQUIRE(charView.startsWith(u8"A"_el));
        REQUIRE(charView.endsWith(u8"¢"_el));
        REQUIRE(charView.contains(u8"A¢"_el));

        const auto invalid = U8StringEditor{std::string_view{th::stdStringFromHex("41 C0 42")}};
        const auto replacement = U8StringEditor{std::u8string_view{u8"A\uFFFDB"}};
        REQUIRE_EQUAL(invalid.compare(replacement), std::strong_ordering::equal);
    }

    void testCaseInsensitiveComparisonChecks() {

        const auto text = U8StringEditor{std::u8string_view{u8"ÄbcK"}};
        const auto view = U8String{text};
        const auto charView = view;

        REQUIRE_EQUAL(text.compare(u8"äbcK"_el, Char::compareCaseFolded), std::strong_ordering::equal);
        REQUIRE_EQUAL(view.compare(u8"äbcK"_el, Char::compareCaseFolded), std::strong_ordering::equal);
        REQUIRE(text.startsWith(u8"äB"_el, Char::compareCaseFolded));
        REQUIRE(text.startsWith(u8"ä"_el, Char::compareCaseFolded));
        REQUIRE(text.endsWith(u8"k"_el, Char::compareCaseFolded));
        REQUIRE(text.contains(u8"BC"_el, Char::compareCaseFolded));
        REQUIRE_EQUAL(text.count(u8"BC"_el, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(text.count(u8"k"_el, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(text.count(U8String{}, Char::compareCaseFolded), ElementCount::zero());
        REQUIRE_EQUAL(view.count(u8"ä"_el, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE_EQUAL(view.count(u8"X"_el, Char::compareCaseFolded), ElementCount::zero());
        REQUIRE(charView.startsWith(u8"äB"_el, Char::compareCaseFolded));
        REQUIRE(charView.endsWith(u8"k"_el, Char::compareCaseFolded));
        REQUIRE(charView.contains(u8"BC"_el, Char::compareCaseFolded));
        REQUIRE_FALSE(text.contains(u8"bd"_el, Char::compareCaseFolded));
    }

    void testInvalidUtf8PredicateChecks() {

        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};

        REQUIRE_FALSE(text.isValidUtf8());
        REQUIRE(text.contains(u8"\uFFFD"_el));
        REQUIRE_EQUAL(text.count(u8"\uFFFD"_el), ElementCount{1U});
        REQUIRE_EQUAL(text.count(u8"\uFFFD"_el, Char::compareCaseFolded), ElementCount{1U});
        REQUIRE(text.containsOneOf(CharSet{Char::replacement()}));
        REQUIRE_FALSE(text.startsWith(u8"\uFFFD"_el));
        REQUIRE_FALSE(text.endsWith(u8"\uFFFD"_el));
    }

    void testForwardFind() {

        const auto text =
            U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(text.find(u8"¢€"_el), ByteIndex{1U});
        REQUIRE_EQUAL(text.find(U8String{}, ByteIndex{3U}), ByteIndex{3U});
        REQUIRE(text.find(u8"A"_el, ByteIndex::noIndex()).isNoIndex());
        REQUIRE(text.find(u8"€¢"_el).isNoIndex());
        REQUIRE_EQUAL(text.findFirstOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findFirstOf(CharSet{Char{0x20ACU}}, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findFirstOf(CharSet{u8"z😀"_el}), ByteIndex{6U});
        REQUIRE(text.findFirstOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{u8"A¢"_el}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{}), ByteIndex{0U});
        REQUIRE(text.findFirstOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
        REQUIRE(text.findFirstNotOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testReverseFind() {

        const auto text =
            U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

        REQUIRE_EQUAL(text.findLastOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findLastOf(CharSet{Char{0x20ACU}}, ByteIndex{6U}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findLastOf(CharSet{u8"z😀"_el}), ByteIndex{6U});
        REQUIRE(text.findLastOf(CharSet{}).isNoIndex());
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{Char{0x1F600U}}), ByteIndex{3U});
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{u8"€😀"_el}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{}), ByteIndex{6U});
        REQUIRE(text.findLastOf(CharSet{Char{0x41U}}, ByteIndex::zero()).isNoIndex());
        REQUIRE(text.findLastOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
        REQUIRE(text.findLastNotOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testInvalidUtf8ForwardFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};

        REQUIRE_EQUAL(text.findFirstOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
    }

    void testInvalidUtf8ReverseFind() {
        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};

        REQUIRE_EQUAL(text.findLastOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(text.findLastNotOf(CharSet{Char{0x42U}}), ByteIndex{1U});
    }

    void testDetach() {
        auto first = U8StringEditor{std::string_view{"Hello"}};
        const auto second = first;
        const auto *firstData = el::text::impl::UnsafeU8StringEditorAccess{first}.data();
        const auto *secondData = el::text::impl::UnsafeU8StringEditorAccess{second}.data();

        REQUIRE_EQUAL(firstData, secondData);

        first.detach();

        REQUIRE_NOT_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{first}.data(), firstData);
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{second}.data(), secondData);
        REQUIRE_EQUAL(StringConverter{first}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(StringConverter{second}.toStdString(), std::string{"Hello"});
    }

    void testReserveOnEmptyStringCreatesCapacity() {
        auto text = U8StringEditor{};

        text.reserve(ByteLength{7U});

        REQUIRE(text.isEmpty());
        REQUIRE_EQUAL(text.capacity(), ByteLength{7U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(7U));
    }

    void testReserveOnNonEmptyStringGrowsCapacityWithoutChangingText() {
        auto text = U8StringEditor{std::string_view{"Hello"}};
        const auto *originalData = el::text::impl::UnsafeU8StringEditorAccess{text}.data();

        text.reserve(ByteLength{9U});

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{9U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(9U));
        REQUIRE_NOT_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.data(), originalData);

        const auto *reservedData = el::text::impl::UnsafeU8StringEditorAccess{text}.data();
        text.reserve(ByteLength{3U});
        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringEditorAccess{text}.data(), reservedData);
    }

    void testReserveOnSliceMaterializesStandaloneStorage() {
        auto text = U8StringEditor{std::string_view{"abcdef"}}.slice(ByteRange{ByteIndex{2U}, ByteLength{2U}});

        text.reserve(ByteLength{6U});

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"cd"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{6U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(6U));
    }

    void testShrinkToFitReleasesUnusedCapacity() {
        auto text = U8StringEditor{std::string_view{"Hello"}};
        text.reserve(ByteLength{9U});

        text.shrinkToFit();

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"Hello"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{5U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(5U));
    }

    void testShrinkToFitMaterializesSliceToExactSize() {
        auto text = U8StringEditor{std::string_view{"abcdef"}}.slice(ByteRange{ByteIndex{2U}, ByteLength{2U}});

        text.shrinkToFit();

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), std::string{"cd"});
        REQUIRE_EQUAL(text.capacity(), ByteLength{2U});
        REQUIRE_EQUAL(text.memoryUsage(), expectedMemoryUsage(2U));
    }

    void testStdConversions() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};
        const auto slice =
            U8StringEditor{std::u8string_view{u8"xxA¢€😀yy"}}.slice(ByteRange{ByteIndex{2}, ByteLength{10}});

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
        const auto text = U8StringEditor{std::string_view{invalidUtf8Data()}};

        REQUIRE_EQUAL(StringConverter{text}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU8String(), std::u8string{u8"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(th::toStdU32String(StringConverter{text}.toStdWString()), std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(StringConverter{text}.toStdString(), th::stdStringFromHex("41 EF BF BD 42"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU16String(), th::stdU16StringFromHex("0041 FFFD 0042"));
        REQUIRE_EQUAL(StringConverter{text}.toStdU32String(), std::u32string{U"A\uFFFDB"});
    }

    void testFromUtf8EncodingModes() {
        const auto valid = StringConverter{std::string_view{"Hello"}}.toU8String(EncodingMode::Strict);

        REQUIRE_EQUAL(StringConverter{valid}.toStdString(), std::string{"Hello"});
        REQUIRE(
            StringConverter{StringConverter{std::string_view{}}.toU8String(EncodingMode::Strict)}
                .toStdString()
                .empty());
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireFromUtf8Modes(error));
        }
    }

    void testFromUtf16EncodingModes() {
        const auto valid =
            StringConverter{th::stdU16StringFromHex("0041 00A2 20AC D83D DE00")}.toU8String(EncodingMode::Strict);

        REQUIRE_EQUAL(StringConverter{valid}.toStdU32String(), std::u32string{U"A¢€😀"});

        const auto invalid = th::stdU16StringFromHex("D800 0041 DC00 D800 D83D DE00");
        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU8String(EncodingMode::Tolerant)}.toStdU32String(),
            std::u32string{U"\uFFFDA\uFFFD\uFFFD😀"});
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingMode::Strict));
    }

    void testFromUtf32EncodingModes() {
        const auto valid = StringConverter{std::u32string_view{U"A¢€😀"}}.toU8String(EncodingMode::Strict);

        REQUIRE_EQUAL(StringConverter{valid}.toStdU32String(), std::u32string{U"A¢€😀"});

        const auto invalid = std::u32string{U'A', char32_t{0xD800U}, char32_t{0x110000U}, U'B'};

        REQUIRE_EQUAL(
            StringConverter{StringConverter{invalid}.toU8String(EncodingMode::Tolerant)}.toStdU32String(),
            std::u32string{U"A\uFFFD\uFFFDB"});
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingMode::Strict));
    }

    void testFromWideEncodingModes() {
#ifdef ERBSLAND_WCHAR_16BIT
        const auto invalid = th::stdWStringFromHex("D800 0041 DC00 D800 D83D DE00");
        const auto valid = th::stdWStringFromHex("0041 00A2 20AC D83D DE00");
#else
        const auto invalid = th::stdWStringFromHex("00000041 0000D800 00110000 00000042");
        const auto valid = th::stdWStringFromHex("00000041 000000A2 000020AC 0001F600");
#endif

        REQUIRE_EQUAL(
            StringConverter{StringConverter{valid}.toU8String(EncodingMode::Strict)}.toStdU32String(),
            std::u32string{U"A¢€😀"});
        REQUIRE_FALSE(
            StringConverter{StringConverter{invalid}.toU8String(EncodingMode::Tolerant)}.toStdU32String().empty());
        REQUIRE_THROWS(StringConverter{invalid}.toU8String(EncodingMode::Strict));
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
            StringConverter{StringConverter{malformed}.toU8String(EncodingMode::Tolerant)}.toStdU32String(),
            expectedReplaceDecodedText(error));
        REQUIRE_THROWS(StringConverter{malformed}.toU8String(EncodingMode::Strict));
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
};
