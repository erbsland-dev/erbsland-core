// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/ByteFormat.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <vector>

using el::mem::ByteBlock;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::CpLength;
using el::unit::ItemCount;
using namespace el::text;
using namespace el::text::literals;

TESTED_TARGETS(ByteFormatFlag ByteFormat AnyStringBuilder String U16String U32String)
class ByteFormatTest final : public el::UnitTest {
public:
    void testStableEnumValuesAndFlags() {
        REQUIRE_EQUAL(static_cast<std::uint8_t>(ByteFormatFlag::Separator), 1U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(ByteFormatFlag::ByteGroups), 2U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(ByteFormatFlag::Lines), 4U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(ByteFormatFlag::Offset), 8U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(ByteFormatFlag::LineGroups), 16U);
        REQUIRE_EQUAL(static_cast<std::uint8_t>(ByteFormatFlag::All), 31U);

        const auto flags = ByteFormatFlag::Separator | ByteFormatFlag::Offset;
        REQUIRE(flags.isSet(ByteFormatFlag::Separator));
        REQUIRE(flags.isSet(ByteFormatFlag::Offset));
        REQUIRE_FALSE(flags.isSet(ByteFormatFlag::Lines));
    }

    void testDefaultCompactAndUppercase() {
        const auto block = makeBlock({0xabU, 0x01U, 0x02U, 0x03U, 0xefU});

        REQUIRE_EQUAL(format(block), std::string{"ab010203ef"});
        REQUIRE_EQUAL(format(ByteBlock{}, ByteFormat::defaultFormat()), std::string{});

        auto uppercase = ByteFormat::compact().setLetterCase(LetterCase::Uppercase);
        REQUIRE_EQUAL(format(block, uppercase), std::string{"AB010203EF"});
    }

    void testSeparatedAndByteGroups() {
        const auto block = makeBlock({0xabU, 0x01U, 0x02U, 0x03U, 0xefU});

        REQUIRE_EQUAL(format(block, ByteFormat::separated()), std::string{"ab 01 02 03 ef"});

        auto grouped = ByteFormat{}
                           .setFlags(ByteFormatFlag::Separator | ByteFormatFlag::ByteGroups)
                           .setByteGroupSize(ByteLength{2U});
        REQUIRE_EQUAL(format(block, grouped), std::string{"ab01 0203 ef"});
    }

    void testLinesOffsetsAndLineGroups() {
        const auto block = makeBlock({0xabU, 0x01U, 0x02U, 0x03U, 0xefU});
        auto dump = ByteFormat::memoryDump().setBytesPerLine(ByteLength{4U}).setByteGroupSize(ByteLength{2U});
        dump.setStartOffset(ByteIndex{0x10U});

        REQUIRE_EQUAL(format(block, dump), std::string{"00000010 | ab01 0203\n00000014 | ef\n"});

        auto groupedLines =
            ByteFormat{ByteFormatFlag::Lines | ByteFormatFlag::LineGroups}.setBytesPerLine(ByteLength{2U});
        groupedLines.setLineGroupSize(ItemCount{2U});
        REQUIRE_EQUAL(format(block, groupedLines), std::string{"ab01\n0203\n\nef\n"});
    }

    void testZeroSizesNormalizeToOne() {
        const auto block = makeBlock({0xabU, 0x01U});
        auto format = ByteFormat{
            ByteFormatFlag::Separator | ByteFormatFlag::ByteGroups | ByteFormatFlag::Lines |
            ByteFormatFlag::LineGroups};
        format.setBytesPerLine(ByteLength::zero())
            .setByteGroupSize(ByteLength::zero())
            .setLineGroupSize(ItemCount::zero());

        REQUIRE_EQUAL(format.bytesPerLine(), ByteLength::one());
        REQUIRE_EQUAL(format.byteGroupSize(), ByteLength::one());
        REQUIRE_EQUAL(format.lineGroupSize(), ItemCount::one());
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"ab\n\n01\n"});
    }

    void testStringWidthFactoriesAndBuilderKind() {
        const auto block = makeBlock({0x12U, 0x34U});

        REQUIRE_EQUAL(StringConverter{String::fromByteBlock(block)}.toStdString(), std::string{"1234"});
        REQUIRE_EQUAL(StringConverter{U16String::fromByteBlock(block)}.toStdString(), std::string{"1234"});
        REQUIRE_EQUAL(StringConverter{U32String::fromByteBlock(block)}.toStdString(), std::string{"1234"});

        const auto truncated = ByteFormat::compact().setMaximum(ByteLength{1U}).setEllipsis("…"_el);
        for (const auto kind : {StringKind::U8, StringKind::U16, StringKind::U32}) {
            auto builder = AnyStringBuilder{kind};
            builder.append(U'[').appendByteBlock(block).append(U']');

            REQUIRE_EQUAL(builder.kind(), kind);
            REQUIRE_EQUAL(builder.length(), CpLength{6U});
            REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"[1234]"});

            builder.clear();
            builder.append(U'[').appendByteBlock(block, truncated).append(U']');
            REQUIRE_EQUAL(builder.length(), CpLength{3U});
            REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"[…]"});
        }
    }

    void testFactoryDefaults() {
        REQUIRE_FALSE(ByteFormat::compact().hasFlag(ByteFormatFlag::Separator));
        REQUIRE(ByteFormat::separated().hasFlag(ByteFormatFlag::Separator));

        const auto dump = ByteFormat::memoryDump();
        REQUIRE(dump.hasFlag(ByteFormatFlag::Separator));
        REQUIRE(dump.hasFlag(ByteFormatFlag::ByteGroups));
        REQUIRE(dump.hasFlag(ByteFormatFlag::Lines));
        REQUIRE(dump.hasFlag(ByteFormatFlag::Offset));
        REQUIRE_FALSE(dump.hasFlag(ByteFormatFlag::LineGroups));
        REQUIRE_EQUAL(dump.bytesPerLine(), ByteLength{32U});
        REQUIRE_EQUAL(dump.byteGroupSize(), ByteLength{4U});
    }

    void testTruncationDefaultsAndAccessors() {
        auto format = ByteFormat{};
        REQUIRE(format.maximum().isInfinite());
        REQUIRE_EQUAL(format.truncateMode(), TruncateMode::End);
        REQUIRE(format.ellipsis().isEmpty());

        format.setMaximum(ByteLength{7U}).setTruncateMode(TruncateMode::Begin).setEllipsis("..."_el);
        const auto copy = format;

        REQUIRE_EQUAL(copy.maximum(), ByteLength{7U});
        REQUIRE_EQUAL(copy.truncateMode(), TruncateMode::Begin);
        REQUIRE_EQUAL(copy.ellipsis(), "..."_el);
    }

    void testTruncateModesAndEllipsisSlots() {
        const auto block = makeBlock({0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U});

        auto format = ByteFormat::compact().setMaximum(ByteLength{5U}).setEllipsis("..."_el);
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"00010203..."});

        format.setTruncateMode(TruncateMode::Middle);
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"0001...0809"});

        format.setTruncateMode(TruncateMode::Begin);
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"...06070809"});

        format.addFlags(ByteFormatFlag::Separator).setTruncateMode(TruncateMode::Middle);
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"00 01 ... 08 09"});
    }

    void testTruncationBoundariesAndEmptyEllipsis() {
        const auto block = makeBlock({0U, 1U, 2U, 3U, 4U, 5U});
        auto format = ByteFormat::compact().setEllipsis("..."_el);

        format.setMaximum(ByteLength::zero());
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{});

        format.setMaximum(ByteLength::one());
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"..."});

        format.setMaximum(ByteLength{6U});
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"000102030405"});

        format.setMaximum(ByteLength{7U});
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"000102030405"});

        format.setMaximum(ByteLength{5U}).setEllipsis({});
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"0001020304"});
    }

    void testDiagnosticFactoryAndLayoutFallback() {
        const auto block =
            makeBlock({0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U});
        const auto diagnostic = ByteFormat::forDiagnostic();

        REQUIRE_EQUAL(diagnostic.maximum(), ByteLength{16U});
        REQUIRE_EQUAL(diagnostic.truncateMode(), TruncateMode::Middle);
        REQUIRE_EQUAL(diagnostic.ellipsis(), "..."_el);
        REQUIRE_EQUAL(ByteFormatTest::format(block, diagnostic), std::string{"0001020304050607...0d0e0f10111213"});

        auto lines = ByteFormat{ByteFormatFlag::Lines}
                         .setBytesPerLine(ByteLength{3U})
                         .setMaximum(ByteLength{5U})
                         .setTruncateMode(TruncateMode::Middle)
                         .setEllipsis("..."_el);
        REQUIRE_EQUAL(ByteFormatTest::format(block, lines), std::string{"000102\n03...\n"});
    }

private:
    [[nodiscard]] static auto makeBlock(std::initializer_list<std::uint8_t> bytes) -> ByteBlock {
        return ByteBlock::fromVector(std::vector<std::uint8_t>{bytes});
    }

    [[nodiscard]] static auto format(const ByteBlock &block, const ByteFormat byteFormat = ByteFormat::defaultFormat())
        -> std::string {
        return StringConverter{String::fromByteBlock(block, byteFormat)}.toStdString();
    }
};
