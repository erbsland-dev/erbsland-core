// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u8/impl/U8StringComparisonTools.hpp>
#include <erbsland/text/u8/impl/U8StringReadTools.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <span>
#include <string>
#include <string_view>

using el::text::Char;
using el::text::CharSet;
using el::text::EncodingMode;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::ByteRange;
using el::unit::CpLength;
using el::unit::ElementCount;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8StringReadTools U8StringComparisonTools)
class U8StringReadToolsTest final : public el::UnitTest {
    static constexpr auto cText = std::array<char, 6>{'a', 'b', 'c', 'd', 'e', 'f'};
    static constexpr auto cUtf8Text = std::array<char, 10>{
        'A',
        static_cast<char>(0xC2U),
        static_cast<char>(0xA2U),
        static_cast<char>(0xE2U),
        static_cast<char>(0x82U),
        static_cast<char>(0xACU),
        static_cast<char>(0xF0U),
        static_cast<char>(0x9FU),
        static_cast<char>(0x98U),
        static_cast<char>(0x80U)};
    static constexpr auto cInvalidUtf8Text = std::array<char, 3>{'A', static_cast<char>(0xC0U), 'B'};

public:
    void testEmptyDataView() {
        const auto tools = el::text::impl::U8StringReadTools{el::text::impl::U8StringDataView{}};

        REQUIRE(tools.isEmpty());
        REQUIRE(tools.isValidUtf8());
        REQUIRE(tools.data().empty());
        REQUIRE(tools.range().isEmpty());
        REQUIRE_EQUAL(tools.byteLength().toSizeT(), 0U);
        REQUIRE(tools.charAt(ByteIndex::zero()).isEndOfData());
        REQUIRE_EQUAL(tools.toStdString(), std::string{});
        REQUIRE(tools.toStdU8String().empty());
    }

    void testFullRange() {
        const auto tools = makeTools(ByteRange::fromSizeT(cText.size()));

        REQUIRE_FALSE(tools.isEmpty());
        REQUIRE_EQUAL(tools.data().size(), cText.size());
        REQUIRE_EQUAL(tools.byteLength().toSizeT(), cText.size());
        REQUIRE_EQUAL(tools.toStdString(), std::string{"abcdef"});
    }

    void testSlicedRange() {
        const auto tools = makeTools(ByteRange{ByteIndex{2U}, ByteLength{3U}});

        REQUIRE_FALSE(tools.isEmpty());
        REQUIRE_EQUAL(tools.toStdString(), std::string{"cde"});
        REQUIRE_EQUAL(tools.toStdU8String(), std::u8string{u8"cde"});
    }

    void testRangeLengthIsClampedToAvailableData() {
        const auto tools = makeTools(ByteRange{ByteIndex{4U}, ByteLength{99U}});

        REQUIRE_EQUAL(tools.toStdString(), std::string{"ef"});
    }

    void testInvalidRangeReturnsEmptyString() {
        const auto tools = makeTools(ByteRange{ByteIndex::noIndex(), ByteLength{1U}});

        REQUIRE_FALSE(tools.isEmpty());
        REQUIRE_EQUAL(tools.toStdString(), std::string{});
        REQUIRE(tools.toStdU8String().empty());
    }

    void testRangeStartingAfterDataReturnsEmptyString() {
        const auto tools = makeTools(ByteRange{ByteIndex{99U}, ByteLength{1U}});

        REQUIRE_EQUAL(tools.toStdString(), std::string{});
    }

    void testEmptyRangeWithDataIsEmpty() {
        const auto tools = makeTools(ByteRange::empty());

        REQUIRE(tools.isEmpty());
        REQUIRE_EQUAL(tools.toStdString(), std::string{});
    }

    void testUtf8Read() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));

        REQUIRE(tools.isValidUtf8());
        REQUIRE_EQUAL(tools.byteLength().toSizeT(), cUtf8Text.size());
        REQUIRE_EQUAL(tools.charAt(ByteIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(tools.charAt(ByteIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(tools.charAt(ByteIndex{3}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(tools.charAt(ByteIndex{6}).toRawValue(), U'\U0001F600');
        REQUIRE(tools.charAt(ByteIndex{10}).isEndOfData());
        REQUIRE(tools.charAt(ByteIndex{11}).isNoCodePoint());
        REQUIRE(tools.charAt(ByteIndex::noIndex()).isNoCodePoint());
    }

    void testInvalidUtf8Read() {
        const auto tools = makeInvalidUtf8Tools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));

        REQUIRE_FALSE(tools.isValidUtf8());
        REQUIRE(tools.charAt(ByteIndex{1}).isReplacement());
    }

    void testReadAndAdvance() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));
        auto index = ByteIndex::zero();

        REQUIRE_EQUAL(tools.read(index).toRawValue(), U'A');
        REQUIRE_EQUAL(index.toSizeT(), 1U);
        REQUIRE_EQUAL(tools.read(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), 3U);
        REQUIRE_EQUAL(tools.read(index).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(index.toSizeT(), 6U);
        REQUIRE_EQUAL(tools.read(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), 10U);

        REQUIRE(tools.read(index).isEndOfData());
        REQUIRE_EQUAL(index.toSizeT(), 10U);

        index = ByteIndex{11U};
        REQUIRE(tools.read(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), 11U);

        index = ByteIndex::noIndex();
        REQUIRE(tools.read(index).isNoCodePoint());
        REQUIRE(index.isNoIndex());
    }

    void testReadAndRetreat() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));
        auto index = ByteIndex::end(tools.byteLength());

        REQUIRE_EQUAL(tools.readAndRetreat(index).toRawValue(), U'\U0001F600');
        REQUIRE_EQUAL(index.toSizeT(), 6U);
        REQUIRE_EQUAL(tools.readAndRetreat(index).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(index.toSizeT(), 3U);
        REQUIRE_EQUAL(tools.readAndRetreat(index).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(index.toSizeT(), 1U);
        REQUIRE_EQUAL(tools.readAndRetreat(index).toRawValue(), U'A');
        REQUIRE(index.isZero());

        REQUIRE(tools.readAndRetreat(index).isEndOfData());
        REQUIRE(index.isZero());

        index = ByteIndex{11U};
        REQUIRE(tools.readAndRetreat(index).isNoCodePoint());
        REQUIRE_EQUAL(index.toSizeT(), 11U);

        index = ByteIndex::noIndex();
        REQUIRE(tools.readAndRetreat(index).isNoCodePoint());
        REQUIRE(index.isNoIndex());
    }

    void testIndexedReadWithInvalidUtf8() {
        const auto tools = makeInvalidUtf8Tools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));
        auto index = ByteIndex{1U};

        REQUIRE(tools.read(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), 2U);

        REQUIRE(tools.readAndRetreat(index).isReplacement());
        REQUIRE_EQUAL(index.toSizeT(), 1U);
    }

    void testAdvanceEdgeCases() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));
        auto index = ByteIndex{1};

        REQUIRE_FALSE(tools.advance(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), 1U);

        REQUIRE(tools.advance(index, CpLength{2}));
        REQUIRE_EQUAL(index.toSizeT(), 6U);

        REQUIRE(tools.advance(index, CpLength{3}));
        REQUIRE_EQUAL(index.toSizeT(), 10U);

        index = ByteIndex::zero();
        REQUIRE(tools.advance(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), 10U);

        REQUIRE_FALSE(tools.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), 10U);
    }

    void testAdvanceOverAllUtf8SequenceKinds() {
        WITH_CONTEXT(requireAdvanceForSingleValidSequence("A"));
        WITH_CONTEXT(requireAdvanceForSingleValidSequence(th::stdStringFromHex("C2 A2")));
        WITH_CONTEXT(requireAdvanceForSingleValidSequence(th::stdStringFromHex("E2 82 AC")));
        WITH_CONTEXT(requireAdvanceForSingleValidSequence(th::stdStringFromHex("F0 9F 98 80")));

        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireAdvanceForMalformedSequence(error));
        }
    }

    void testRetreatEdgeCases() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));
        auto index = ByteIndex{5};

        // Zero count is a documented no-op, even for an index inside a multibyte sequence.
        REQUIRE_FALSE(tools.retreat(index, CpLength::zero()));
        REQUIRE_EQUAL(index.toSizeT(), 5U);

        // From the middle of the 3-byte euro sign, retreat is documented to move back by one raw byte.
        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 4U);

        // Retreating two characters from the end should land on the euro sign start at byte 3.
        index = ByteIndex{10};
        REQUIRE(tools.retreat(index, CpLength{2}));
        REQUIRE_EQUAL(index.toSizeT(), 3U);

        // An out-of-bounds index is considered as being at the end position.
        index = ByteIndex{99};
        // After clamping, a single retreat from the end should reach the start of the final 4-byte character.
        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 6U);

        // Infinite retreat moves all the way to the beginning.
        index = ByteIndex{6};
        REQUIRE(tools.retreat(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), 0U);

        // Once we are at the beginning, another retreat must stay there and report no movement.
        REQUIRE_FALSE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 0U);
        REQUIRE_FALSE(tools.retreat(index, CpLength::infinite()));
        REQUIRE_EQUAL(index.toSizeT(), 0U);

        // The special no-index value is never modified by retreat.
        index = ByteIndex::noIndex();
        REQUIRE_FALSE(tools.retreat(index));
        REQUIRE(index.isNoIndex());
    }

    void testRetreatOverAllUtf8SequenceKinds() {
        WITH_CONTEXT(requireRetreatForSingleValidSequence("A"));
        WITH_CONTEXT(requireRetreatForSingleValidSequence(th::stdStringFromHex("C2 A2")));
        WITH_CONTEXT(requireRetreatForSingleValidSequence(th::stdStringFromHex("E2 82 AC")));
        WITH_CONTEXT(requireRetreatForSingleValidSequence(th::stdStringFromHex("F0 9F 98 80")));

        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireRetreatForMalformedSequence(error));
        }
    }

    void testRetreatWithInvalidUtf8() {
        const auto tools = makeInvalidUtf8Tools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));
        auto index = ByteIndex{3};

        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 2U);

        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 1U);

        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 0U);
    }

    void testSliceRange() {
        const auto tools = makeTools(ByteRange{ByteIndex{1U}, ByteLength{4U}});

        REQUIRE_EQUAL(
            tools.sliceRange(ByteRange{ByteIndex{1U}, ByteLength{2U}}), (ByteRange{ByteIndex{2U}, ByteLength{2U}}));
        REQUIRE_EQUAL(
            tools.sliceRange(ByteRange{ByteIndex{2U}, ByteLength::infinite()}),
            (ByteRange{ByteIndex{3U}, ByteLength{2U}}));
        REQUIRE(tools.sliceRange(ByteRange{ByteIndex{4U}, ByteLength{1U}}).isEmpty());
        REQUIRE(tools.sliceRange(ByteRange{ByteIndex::noIndex(), ByteLength{1U}}).isEmpty());
        REQUIRE(tools.sliceRange(ByteRange::noRange()).isEmpty());
    }

    void testByteSequencePredicates() {
        const auto tools = makeComparisonTools("abcdef");

        REQUIRE(tools.startsWith(makeDataView("abc")));
        REQUIRE(tools.startsWith(makeDataView("")));
        REQUIRE_FALSE(tools.startsWith(makeDataView("bc")));
        REQUIRE_FALSE(tools.startsWith(makeDataView("abcdefg")));
        REQUIRE(tools.endsWith(makeDataView("def")));
        REQUIRE(tools.endsWith(makeDataView("")));
        REQUIRE_FALSE(tools.endsWith(makeDataView("de")));
        REQUIRE_FALSE(tools.endsWith(makeDataView("zabcdef")));
        REQUIRE(tools.contains(makeDataView("bcd")));
        REQUIRE(tools.contains(makeDataView("")));
        REQUIRE_FALSE(tools.contains(makeDataView("bd")));
    }

    void testUtf8PredicateChecks() {
        const auto tools = makeUtf8ComparisonTools(ByteRange::fromSizeT(cUtf8Text.size()));

        REQUIRE(tools.startsWith(Char{0x41U}));
        REQUIRE_FALSE(tools.startsWith(Char{0x00A2U}));
        REQUIRE(tools.endsWith(Char{0x1F600U}));
        REQUIRE_FALSE(tools.endsWith(Char{0x20ACU}));
        REQUIRE(tools.contains(Char{0x20ACU}));
        REQUIRE_FALSE(tools.contains(Char{0x5AU}));
        REQUIRE(tools.containsOneOf(CharSet{Char{0x20ACU}, Char{U'Z'}}));
        REQUIRE(tools.containsOneOf(CharSet{Char{U'z'}, Char{0x1F600U}}));
        REQUIRE_FALSE(tools.containsOneOf(CharSet{Char{U'x'}, Char{U'y'}}));
        REQUIRE_FALSE(tools.containsOneOf(CharSet{}));
    }

    void testInvalidUtf8PredicateChecks() {
        const auto readTools = makeInvalidUtf8Tools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));
        const auto tools = makeInvalidUtf8ComparisonTools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));

        REQUIRE_FALSE(readTools.isValidUtf8());
        REQUIRE(tools.contains(Char::replacement()));
        REQUIRE(tools.containsOneOf(CharSet{Char::replacement()}));
        REQUIRE_FALSE(tools.startsWith(Char::replacement()));
        REQUIRE_FALSE(tools.endsWith(Char::replacement()));
    }

    void testForEachDecodedCharacter() {
        const auto text = th::stdStringFromHex("41 C2 A2 E2 82 AC");
        auto characters = std::u32string{};

        const auto completed = el::text::impl::utf8::forEachDecodedCharacter(
            makeSpan(text), EncodingMode::Tolerant, [&](const Char character) -> bool {
                characters.push_back(character.toRawValue());
                return true;
            });

        REQUIRE(completed);
        REQUIRE_EQUAL(characters.size(), 3U);
        REQUIRE_EQUAL(characters.at(0), U'A');
        REQUIRE_EQUAL(characters.at(1), U'\u00A2');
        REQUIRE_EQUAL(characters.at(2), U'\u20AC');
    }

    void testForEachDecodedCharacterWithInvalidUtf8() {
        const auto text = invalidUtf8Text();
        auto characters = std::u32string{};

        const auto completed = el::text::impl::utf8::forEachDecodedCharacter(
            makeSpan(text), EncodingMode::Tolerant, [&](const Char character) -> bool {
                characters.push_back(character.toRawValue());
                return true;
            });

        REQUIRE(completed);
        REQUIRE_EQUAL(characters.size(), 3U);
        REQUIRE_EQUAL(characters.at(0), U'A');
        REQUIRE_EQUAL(characters.at(1), Char::replacement().toRawValue());
        REQUIRE_EQUAL(characters.at(2), U'B');
    }

    void testForEachDecodedCharacterCanStopEarly() {
        const auto text = std::string_view{"abc"};
        auto count = std::size_t{0};

        const auto completed = el::text::impl::utf8::forEachDecodedCharacter(
            makeSpan(text), EncodingMode::Tolerant, [&](const Char) -> bool {
                ++count;
                return count < 2U;
            });

        REQUIRE_FALSE(completed);
        REQUIRE_EQUAL(count, 2U);
    }

    void testContainsOneDecodedCharacter() {
        auto set = el::text::impl::U8StringComparisonTools::CharacterSet{};
        set.add(Char{U'\u20AC'});
        set.add(Char{U'z'});
        const auto text = th::stdStringFromHex("41 E2 82 AC");

        REQUIRE(el::text::impl::U8StringComparisonTools::containsOneDecodedCharacter(makeSpan(text), set));
        REQUIRE_FALSE(el::text::impl::U8StringComparisonTools::containsOneDecodedCharacter(makeSpan("abc"), set));
        REQUIRE_FALSE(
            el::text::impl::U8StringComparisonTools::containsOneDecodedCharacter(
                makeSpan("abc"), el::text::impl::U8StringComparisonTools::CharacterSet{}));
    }

    void testFindDecodedText() {
        const auto tools = makeComparisonTools("abcabc");

        REQUIRE_EQUAL(tools.find(makeDataView("bc")), ByteIndex{1U});
        REQUIRE_EQUAL(tools.find(makeDataView("bc"), ByteIndex{2U}), ByteIndex{4U});
        REQUIRE_EQUAL(tools.find(makeDataView(""), ByteIndex{3U}), ByteIndex{3U});
        REQUIRE(tools.find(makeDataView("bd")).isNoIndex());
        REQUIRE(tools.find(makeDataView("a"), ByteIndex{7U}).isNoIndex());
        REQUIRE(tools.find(makeDataView("a"), ByteIndex::noIndex()).isNoIndex());

        const auto cent = th::stdStringFromHex("C2 A2");
        const auto continuationByte = th::stdStringFromHex("A2");
        const auto utf8Tools = makeComparisonTools(cent);
        REQUIRE(utf8Tools.find(makeDataView(continuationByte)).isNoIndex());
    }

    void testLinearDecodedSearchForLongRepeatedPrefixes() {
        auto needle = std::string(64U, 'a');
        needle.back() = 'b';
        auto data = std::string(4096U, 'a');
        data.push_back('b');
        const auto tools = makeComparisonTools(data);

        REQUIRE_EQUAL(tools.find(makeDataView(needle)), ByteIndex{4033U});
        REQUIRE_EQUAL(tools.count(makeDataView(needle)), ElementCount{1U});
        needle.back() = 'c';
        REQUIRE(tools.find(makeDataView(needle)).isNoIndex());

        auto malformedNeedle = th::stdStringFromHex("A2");
        malformedNeedle.append(64U, 'a');
        auto malformedData = th::stdStringFromHex("C2 A2");
        malformedData.append(64U, 'a');
        REQUIRE_EQUAL(
            makeComparisonTools(malformedData).find(makeDataView(malformedNeedle), ByteIndex{1U}), ByteIndex{1U});
    }

    void testFindFirstOf() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));

        REQUIRE_EQUAL(tools.findFirstOf(CharSet{Char{0x41U}}), ByteIndex{0U});
        REQUIRE_EQUAL(tools.findFirstOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(tools.findFirstOf(CharSet{Char{0x20ACU}}, ByteIndex{2U}), ByteIndex{3U});
        REQUIRE(tools.findFirstOf(CharSet{Char{U'x'}, Char{U'y'}}).isNoIndex());
        REQUIRE(tools.findFirstOf(CharSet{}).isNoIndex());
        REQUIRE(tools.findFirstOf(CharSet{Char{0x41U}}, ByteIndex{10U}).isNoIndex());
        REQUIRE(tools.findFirstOf(CharSet{Char{0x41U}}, ByteIndex{11U}).isNoIndex());
        REQUIRE(tools.findFirstOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testFindFirstNotOf() {
        const auto asciiTools = makeTools("aaab");

        REQUIRE_EQUAL(asciiTools.findFirstNotOf(CharSet{Char{0x61U}}), ByteIndex{3U});
        REQUIRE_EQUAL(asciiTools.findFirstNotOf(CharSet{}), ByteIndex{0U});
        REQUIRE(asciiTools.findFirstNotOf(CharSet{Char{U'a'}, Char{U'b'}}).isNoIndex());

        const auto text = th::stdStringFromHex("61 61 E2 82 AC");
        const auto utf8Tools = makeTools(text);
        REQUIRE_EQUAL(utf8Tools.findFirstNotOf(CharSet{Char{0x61U}}), ByteIndex{2U});
        REQUIRE(utf8Tools.findFirstNotOf(CharSet{Char{0x61U}}, ByteIndex{5U}).isNoIndex());
        REQUIRE(utf8Tools.findFirstNotOf(CharSet{Char{0x61U}}, ByteIndex{6U}).isNoIndex());
        REQUIRE(utf8Tools.findFirstNotOf(CharSet{Char{0x61U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testFindLastOf() {
        const auto tools = makeUtf8Tools(ByteRange::fromSizeT(cUtf8Text.size()));

        REQUIRE_EQUAL(tools.findLastOf(CharSet{Char{0x41U}}), ByteIndex{0U});
        REQUIRE_EQUAL(tools.findLastOf(CharSet{Char{0x20ACU}}), ByteIndex{3U});
        REQUIRE_EQUAL(tools.findLastOf(CharSet{Char{0x20ACU}}, ByteIndex{6U}), ByteIndex{3U});
        REQUIRE_EQUAL(tools.findLastOf(CharSet{Char{0x1F600U}, Char{U'Z'}}), ByteIndex{6U});
        REQUIRE(tools.findLastOf(CharSet{Char{U'x'}, Char{U'y'}}).isNoIndex());
        REQUIRE(tools.findLastOf(CharSet{}).isNoIndex());
        REQUIRE(tools.findLastOf(CharSet{Char{0x41U}}, ByteIndex::zero()).isNoIndex());
        REQUIRE(tools.findLastOf(CharSet{Char{0x41U}}, ByteIndex{11U}).isNoIndex());
        REQUIRE(tools.findLastOf(CharSet{Char{0x41U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testFindLastNotOf() {
        const auto asciiTools = makeTools("aaab");

        REQUIRE_EQUAL(asciiTools.findLastNotOf(CharSet{Char{0x62U}}), ByteIndex{2U});
        REQUIRE_EQUAL(asciiTools.findLastNotOf(CharSet{}), ByteIndex{3U});
        REQUIRE(asciiTools.findLastNotOf(CharSet{Char{U'a'}, Char{U'b'}}).isNoIndex());

        const auto text = th::stdStringFromHex("61 61 E2 82 AC");
        const auto utf8Tools = makeTools(text);
        REQUIRE_EQUAL(utf8Tools.findLastNotOf(CharSet{Char{0x61U}}), ByteIndex{2U});
        REQUIRE(utf8Tools.findLastNotOf(CharSet{Char{0x61U}}, ByteIndex{2U}).isNoIndex());
        REQUIRE(utf8Tools.findLastNotOf(CharSet{Char{0x61U}}, ByteIndex::zero()).isNoIndex());
        REQUIRE(utf8Tools.findLastNotOf(CharSet{Char{0x61U}}, ByteIndex{6U}).isNoIndex());
        REQUIRE(utf8Tools.findLastNotOf(CharSet{Char{0x61U}}, ByteIndex::noIndex()).isNoIndex());
    }

    void testFindWithInvalidUtf8() {
        const auto tools = makeInvalidUtf8Tools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));
        const auto comparisonTools = makeInvalidUtf8ComparisonTools(ByteRange::fromSizeT(cInvalidUtf8Text.size()));

        REQUIRE_FALSE(tools.isValidUtf8());
        REQUIRE_EQUAL(tools.findFirstOf(CharSet{Char::replacement()}), ByteIndex{1U});
        const auto invalidStartByte = th::stdStringFromHex("C0");

        REQUIRE_EQUAL(tools.findFirstNotOf(CharSet{Char{0x41U}}), ByteIndex{1U});
        REQUIRE_EQUAL(comparisonTools.find(makeDataView(invalidStartByte)), ByteIndex{1U});
        REQUIRE_EQUAL(tools.findLastOf(CharSet{Char::replacement()}), ByteIndex{1U});
        REQUIRE_EQUAL(tools.findLastNotOf(CharSet{Char{0x42U}}), ByteIndex{1U});
    }

private:
    [[nodiscard]] static auto makeTools(ByteRange range) noexcept -> el::text::impl::U8StringReadTools {
        return el::text::impl::U8StringReadTools{el::text::impl::U8StringDataView{std::span<const char>{cText}, range}};
    }

    [[nodiscard]] static auto makeUtf8Tools(ByteRange range) noexcept -> el::text::impl::U8StringReadTools {
        return el::text::impl::U8StringReadTools{
            el::text::impl::U8StringDataView{std::span<const char>{cUtf8Text}, range}};
    }

    [[nodiscard]] static auto makeUtf8ComparisonTools(ByteRange range) noexcept
        -> el::text::impl::U8StringComparisonTools {
        return el::text::impl::U8StringComparisonTools{
            el::text::impl::U8StringDataView{std::span<const char>{cUtf8Text}, range}};
    }

    [[nodiscard]] static auto makeInvalidUtf8Tools(ByteRange range) noexcept -> el::text::impl::U8StringReadTools {
        return el::text::impl::U8StringReadTools{
            el::text::impl::U8StringDataView{std::span<const char>{cInvalidUtf8Text}, range}};
    }

    [[nodiscard]] static auto makeInvalidUtf8ComparisonTools(ByteRange range) noexcept
        -> el::text::impl::U8StringComparisonTools {
        return el::text::impl::U8StringComparisonTools{
            el::text::impl::U8StringDataView{std::span<const char>{cInvalidUtf8Text}, range}};
    }

    [[nodiscard]] static auto makeDataView(const std::string_view sequence) noexcept
        -> el::text::impl::U8StringDataView {
        return el::text::impl::U8StringDataView{
            std::span<const char>{sequence.data(), sequence.size()}, ByteRange::fromSizeT(sequence.size())};
    }

    [[nodiscard]] static auto makeSpan(const std::string_view sequence) noexcept -> std::span<const char> {
        return std::span<const char>{sequence.data(), sequence.size()};
    }

    [[nodiscard]] static auto invalidUtf8Text() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }

    void requireAdvanceForSingleValidSequence(const std::string_view sequence) {
        const auto tools = makeTools(sequence);
        auto index = ByteIndex::zero();

        REQUIRE(tools.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), sequence.size());

        for (auto i = std::size_t{1}; i < sequence.size(); ++i) {
            index = ByteIndex::fromSizeT(i);
            REQUIRE(tools.advance(index));
            REQUIRE_EQUAL(index.toSizeT(), i + 1U);
        }

        index = ByteIndex::zero();
        auto steps = std::size_t{0};
        while (tools.advance(index)) {
            ++steps;
            REQUIRE_LESS_EQUAL(steps, sequence.size());
        }
        REQUIRE_EQUAL(index.toSizeT(), sequence.size());
    }

    void requireAdvanceForMalformedSequence(const th::Utf8Error error) {
        const auto sequence = th::invalidUtf8(error);
        const auto tools = makeTools(sequence);
        auto index = ByteIndex::zero();

        REQUIRE(tools.advance(index));
        REQUIRE_EQUAL(index.toSizeT(), expectedMalformedMovement(error));

        for (auto i = std::size_t{1}; i < sequence.size(); ++i) {
            index = ByteIndex::fromSizeT(i);
            REQUIRE(tools.advance(index));
            REQUIRE_EQUAL(index.toSizeT(), i + 1U);
        }

        index = ByteIndex::zero();
        auto steps = std::size_t{0};
        while (tools.advance(index)) {
            ++steps;
            REQUIRE_LESS_EQUAL(steps, sequence.size());
        }
        REQUIRE_EQUAL(index.toSizeT(), sequence.size());
    }

    void requireRetreatForSingleValidSequence(const std::string_view sequence) {
        const auto tools = makeTools(sequence);
        auto index = ByteIndex::fromSizeT(sequence.size());

        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), 0U);

        for (auto i = std::size_t{1}; i < sequence.size(); ++i) {
            index = ByteIndex::fromSizeT(i);
            REQUIRE(tools.retreat(index));
            REQUIRE_EQUAL(index.toSizeT(), i - 1U);
        }

        index = ByteIndex::fromSizeT(sequence.size());
        auto steps = std::size_t{0};
        while (tools.retreat(index)) {
            ++steps;
            REQUIRE_LESS_EQUAL(steps, sequence.size());
        }
        REQUIRE(index.isZero());
    }

    void requireRetreatForMalformedSequence(const th::Utf8Error error) {
        const auto sequence = th::invalidUtf8(error);
        const auto tools = makeTools(sequence);

        auto index = ByteIndex::fromSizeT(sequence.size());
        REQUIRE(tools.retreat(index));
        REQUIRE_EQUAL(index.toSizeT(), sequence.size() - expectedMalformedMovement(error));

        for (auto i = std::size_t{1}; i < sequence.size(); ++i) {
            index = ByteIndex::fromSizeT(i);
            REQUIRE(tools.retreat(index));
            REQUIRE_EQUAL(index.toSizeT(), i - 1U);
        }

        index = ByteIndex::fromSizeT(sequence.size());
        auto steps = std::size_t{0};
        while (tools.retreat(index)) {
            ++steps;
            REQUIRE_LESS_EQUAL(steps, sequence.size());
        }
        REQUIRE(index.isZero());
    }

    [[nodiscard]] static auto expectedMalformedMovement(const th::Utf8Error error) noexcept -> std::size_t {
        switch (error) {
        case th::Utf8Error::SurrogateCodePoint:
            return 3U;
        case th::Utf8Error::CodePointBeyondUnicodeRange:
            return 4U;
        default:
            return 1U;
        }
    }

    [[nodiscard]] static auto makeTools(const std::string_view sequence) noexcept -> el::text::impl::U8StringReadTools {
        return el::text::impl::U8StringReadTools{el::text::impl::U8StringDataView{
            std::span<const char>{sequence.data(), sequence.size()}, ByteRange::fromSizeT(sequence.size())}};
    }

    [[nodiscard]] static auto makeComparisonTools(const std::string_view sequence) noexcept
        -> el::text::impl::U8StringComparisonTools {
        return el::text::impl::U8StringComparisonTools{el::text::impl::U8StringDataView{
            std::span<const char>{sequence.data(), sequence.size()}, ByteRange::fromSizeT(sequence.size())}};
    }
};
