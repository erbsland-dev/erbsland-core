// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/AnyStringBuilder.hpp>
#include <erbsland/text/ByteFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ElementCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <vector>

using el::mem::ByteBlock;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using el::unit::CpLength;
using el::unit::ElementCount;
using namespace el::text;

TESTED_TARGETS(ByteFormatFlag ByteFormat AnyStringBuilder U8String U16String U32String)
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
        groupedLines.setLineGroupSize(ElementCount{2U});
        REQUIRE_EQUAL(format(block, groupedLines), std::string{"ab01\n0203\n\nef\n"});
    }

    void testZeroSizesNormalizeToOne() {
        const auto block = makeBlock({0xabU, 0x01U});
        auto format = ByteFormat{
            ByteFormatFlag::Separator | ByteFormatFlag::ByteGroups | ByteFormatFlag::Lines |
            ByteFormatFlag::LineGroups};
        format.setBytesPerLine(ByteLength::zero())
            .setByteGroupSize(ByteLength::zero())
            .setLineGroupSize(ElementCount::zero());

        REQUIRE_EQUAL(format.bytesPerLine(), ByteLength::one());
        REQUIRE_EQUAL(format.byteGroupSize(), ByteLength::one());
        REQUIRE_EQUAL(format.lineGroupSize(), ElementCount::one());
        REQUIRE_EQUAL(ByteFormatTest::format(block, format), std::string{"ab\n\n01\n"});
    }

    void testStringWidthFactoriesAndBuilderKind() {
        const auto block = makeBlock({0x12U, 0x34U});

        REQUIRE_EQUAL(StringConverter{U8String::fromByteBlock(block)}.toStdString(), std::string{"1234"});
        REQUIRE_EQUAL(StringConverter{U16String::fromByteBlock(block)}.toStdString(), std::string{"1234"});
        REQUIRE_EQUAL(StringConverter{U32String::fromByteBlock(block)}.toStdString(), std::string{"1234"});

        for (const auto kind : {StringKind::U8, StringKind::U16, StringKind::U32}) {
            auto builder = AnyStringBuilder{kind};
            builder.append(U'[').appendByteBlock(block).append(U']');

            REQUIRE_EQUAL(builder.kind(), kind);
            REQUIRE_EQUAL(builder.length(), CpLength{6U});
            REQUIRE_EQUAL(StringConverter{builder.toU8String()}.toStdString(), std::string{"[1234]"});
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

private:
    [[nodiscard]] static auto makeBlock(std::initializer_list<std::uint8_t> bytes) -> ByteBlock {
        return ByteBlock{std::vector<std::uint8_t>{bytes}};
    }

    [[nodiscard]] static auto format(const ByteBlock &block, const ByteFormat byteFormat = ByteFormat::defaultFormat())
        -> std::string {
        return StringConverter{U8String::fromByteBlock(block, byteFormat)}.toStdString();
    }
};
