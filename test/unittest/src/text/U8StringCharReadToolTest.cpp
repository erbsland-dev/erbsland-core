// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/u8/impl/U8StringCharReadTool.hpp>
#include <erbsland/text/u8/impl/U8StringDataView.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ByteRange.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/CpRange.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <span>
#include <string>
#include <string_view>

using el::text::Char;
using el::text::CharSet;
using namespace el::unit;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8StringCharReadTool)
class U8StringCharReadToolTest final : public el::UnitTest {
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
        const auto tool = el::text::impl::U8StringCharReadTool{el::text::impl::U8StringDataView{}};

        REQUIRE_EQUAL(tool.charLength().toSizeT(), 0U);
        REQUIRE(tool.charAt(CpIndex::zero()).isEndOfData());
        REQUIRE_EQUAL(tool.byteIndexAt(CpIndex::zero()), ByteIndex::zero());
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex::zero()), CpIndex::zero());
        REQUIRE(tool.byteIndexAt(CpIndex{1U}).isNoIndex());
        REQUIRE(tool.charIndexAt(ByteIndex{1U}).isNoIndex());
        REQUIRE_THROWS(tool.charAtOrThrow(CpIndex::zero()));
        REQUIRE(tool.find(el::text::impl::U8StringDataView{}, CpIndex::zero()).isZero());
        REQUIRE(tool.findFirstOf(CharSet{Char{U'A'}}).isNoIndex());
        REQUIRE(tool.findLastNotOf(CharSet{Char{U'A'}}).isNoIndex());
    }

    void testReadAndSliceRange() {
        const auto tool = makeUtf8Tool(ByteRange::fromSizeT(cUtf8Text.size()));
        const auto expectedRange = ByteRange{ByteIndex{1}, ByteLength{5}};

        REQUIRE_EQUAL(tool.charLength().toSizeT(), 4U);
        REQUIRE_EQUAL(tool.charAt(CpIndex{0}).toRawValue(), U'A');
        REQUIRE_EQUAL(tool.charAt(CpIndex{1}).toRawValue(), U'\u00A2');
        REQUIRE_EQUAL(tool.charAt(CpIndex{2}).toRawValue(), U'\u20AC');
        REQUIRE_EQUAL(tool.charAt(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE(tool.charAt(CpIndex{4}).isEndOfData());
        REQUIRE(tool.charAt(CpIndex{5}).isNoCodePoint());
        REQUIRE(tool.charAt(CpIndex::noIndex()).isNoCodePoint());
        REQUIRE_EQUAL(tool.charAtOrThrow(CpIndex{3}).toRawValue(), U'\U0001F600');
        REQUIRE_THROWS(tool.charAtOrThrow(CpIndex{4}));
        REQUIRE_THROWS(tool.charAtOrThrow(CpIndex::noIndex()));
        REQUIRE_EQUAL(tool.sliceRange(CpRange{CpIndex{1}, CpLength{2}}), expectedRange);
    }

    void testIndexConversions() {
        const auto tool = makeUtf8Tool(ByteRange::fromSizeT(cUtf8Text.size()));

        REQUIRE_EQUAL(tool.byteIndexAt(CpIndex{0}), ByteIndex{0});
        REQUIRE_EQUAL(tool.byteIndexAt(CpIndex{1}), ByteIndex{1});
        REQUIRE_EQUAL(tool.byteIndexAt(CpIndex{2}), ByteIndex{3});
        REQUIRE_EQUAL(tool.byteIndexAt(CpIndex{3}), ByteIndex{6});
        REQUIRE_EQUAL(tool.byteIndexAt(CpIndex{4}), ByteIndex{10});
        REQUIRE(tool.byteIndexAt(CpIndex{5}).isNoIndex());
        REQUIRE(tool.byteIndexAt(CpIndex::noIndex()).isNoIndex());

        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{0}), CpIndex{0});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{1}), CpIndex{1});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{2}), CpIndex{1});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{3}), CpIndex{2});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{5}), CpIndex{2});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{6}), CpIndex{3});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{9}), CpIndex{3});
        REQUIRE_EQUAL(tool.charIndexAt(ByteIndex{10}), CpIndex{4});
        REQUIRE(tool.charIndexAt(ByteIndex{11}).isNoIndex());
        REQUIRE(tool.charIndexAt(ByteIndex::noIndex()).isNoIndex());
    }

    void testCharacterSliceRange() {
        const auto tool = makeUtf8Tool(ByteRange{ByteIndex{1U}, ByteLength{9U}});

        REQUIRE_EQUAL(tool.sliceRange(CpRange{CpIndex{1U}, CpLength{2U}}), (ByteRange{ByteIndex{3U}, ByteLength{7U}}));
        REQUIRE_EQUAL(
            tool.sliceRange(CpRange{CpIndex{1U}, CpLength::infinite()}), (ByteRange{ByteIndex{3U}, ByteLength{7U}}));
        REQUIRE_EQUAL(tool.sliceRange(CpRange{CpIndex{2U}, CpLength{99U}}), (ByteRange{ByteIndex{6U}, ByteLength{4U}}));
        REQUIRE(tool.sliceRange(CpRange{CpIndex{3U}, CpLength{1U}}).isEmpty());
        REQUIRE(tool.sliceRange(CpRange{CpIndex::noIndex(), CpLength{1U}}).isEmpty());
        REQUIRE(tool.sliceRange(CpRange::noRange()).isEmpty());
    }

    void testCharacterSliceRangeWithInvalidUtf8() {
        const auto tool = makeInvalidUtf8Tool(ByteRange::fromSizeT(cInvalidUtf8Text.size()));

        REQUIRE_EQUAL(tool.sliceRange(CpRange{CpIndex{1U}, CpLength{1U}}), (ByteRange{ByteIndex{1U}, ByteLength{1U}}));
        REQUIRE_EQUAL(tool.sliceRange(CpRange{CpIndex{1U}, CpLength{2U}}), (ByteRange{ByteIndex{1U}, ByteLength{2U}}));
    }

    void testForwardFind() {
        const auto tool = makeUtf8Tool(ByteRange::fromSizeT(cUtf8Text.size()));
        const auto euroAndEmojiText = th::stdStringFromHex("E2 82 AC F0 9F 98 80");
        const auto euroAndEmoji = makeDataView(euroAndEmojiText);

        REQUIRE_EQUAL(tool.find(euroAndEmoji), CpIndex{2U});
        REQUIRE_EQUAL(tool.find(euroAndEmoji, CpIndex{2U}), CpIndex{2U});
        REQUIRE(tool.find(euroAndEmoji, CpIndex{3U}).isNoIndex());
        REQUIRE_EQUAL(tool.find(el::text::impl::U8StringDataView{}, CpIndex{4U}), CpIndex{4U});
        REQUIRE_EQUAL(tool.findFirstOf(CharSet{Char{0x20ACU}}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findFirstOf(CharSet{Char{0x20ACU}, Char{U'Z'}}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findFirstNotOf(CharSet{Char{0x41U}}), CpIndex{1U});
        REQUIRE_EQUAL(tool.findFirstNotOf(CharSet{Char{U'A'}, Char{0x00A2U}}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findFirstNotOf(CharSet{}), CpIndex{0U});
    }

    void testReverseFind() {
        const auto tool = makeUtf8Tool(ByteRange::fromSizeT(cUtf8Text.size()));
        REQUIRE_EQUAL(tool.findLastOf(CharSet{Char{0x20ACU}}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findLastOf(CharSet{Char{0x20ACU}}, CpIndex{3U}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findLastOf(CharSet{Char{0x20ACU}, Char{0x1F600U}}), CpIndex{3U});
        REQUIRE_EQUAL(tool.findLastNotOf(CharSet{Char{0x1F600U}}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findLastNotOf(CharSet{Char{0x20ACU}, Char{0x1F600U}}), CpIndex{1U});
        REQUIRE_EQUAL(tool.findLastNotOf(CharSet{}), CpIndex{3U});
        REQUIRE(tool.findLastOf(CharSet{Char{0x41U}}, CpIndex::zero()).isNoIndex());
    }

    void testInvalidUtf8Find() {
        const auto tool = makeInvalidUtf8Tool(ByteRange::fromSizeT(cInvalidUtf8Text.size()));
        const auto invalidNeedleText = th::stdStringFromHex("C0");
        const auto invalidNeedle = makeDataView(invalidNeedleText);

        REQUIRE_EQUAL(tool.charLength().toSizeT(), 3U);
        REQUIRE(tool.charAt(CpIndex{1}).isReplacement());
        REQUIRE_EQUAL(tool.find(invalidNeedle), CpIndex{1U});
        REQUIRE_EQUAL(tool.findFirstOf(CharSet{Char::replacement()}), CpIndex{1U});
        REQUIRE_EQUAL(tool.findLastOf(CharSet{Char::replacement()}), CpIndex{1U});
        REQUIRE_EQUAL(tool.findLastNotOf(CharSet{Char{0x42U}}), CpIndex{1U});
    }

    void testSlicedRangeFind() {
        const auto tool = makeUtf8Tool(ByteRange{ByteIndex{1}, ByteLength{9}});
        const auto euroAndEmojiText = th::stdStringFromHex("E2 82 AC F0 9F 98 80");
        const auto euroAndEmoji = makeDataView(euroAndEmojiText);

        REQUIRE_EQUAL(tool.charLength().toSizeT(), 3U);
        REQUIRE_EQUAL(tool.find(euroAndEmoji), CpIndex{1U});
        REQUIRE_EQUAL(tool.findFirstOf(CharSet{Char{0x1F600U}}), CpIndex{2U});
        REQUIRE_EQUAL(tool.findLastNotOf(CharSet{Char{0x20ACU}, Char{0x1F600U}}), CpIndex{0U});
    }

private:
    [[nodiscard]] static auto makeUtf8Tool(const ByteRange range) noexcept -> el::text::impl::U8StringCharReadTool {
        return el::text::impl::U8StringCharReadTool{
            el::text::impl::U8StringDataView{std::span<const char>{cUtf8Text}, range}};
    }

    [[nodiscard]] static auto makeInvalidUtf8Tool(const ByteRange range) noexcept
        -> el::text::impl::U8StringCharReadTool {
        return el::text::impl::U8StringCharReadTool{
            el::text::impl::U8StringDataView{std::span<const char>{cInvalidUtf8Text}, range}};
    }

    [[nodiscard]] static auto makeDataView(const std::string_view sequence) noexcept
        -> el::text::impl::U8StringDataView {
        return el::text::impl::U8StringDataView{
            std::span<const char>{sequence.data(), sequence.size()}, ByteRange::fromSizeT(sequence.size())};
    }
};
