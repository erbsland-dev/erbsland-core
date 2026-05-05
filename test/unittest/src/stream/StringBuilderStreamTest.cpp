// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockView.hpp>
#include <erbsland/stream/StringBuilderStream.hpp>
#include <erbsland/text/AnyString.hpp>
#include <erbsland/text/ByteFormat.hpp>
#include <erbsland/text/IntegerFormatFlag.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using el::unit::CpLength;
using namespace el::text;

TESTED_TARGETS(StringBuilderStream TextPrintContext PrintContextCommonBuilder PrintContextToBuilder)
class StringBuilderStreamTest final : public el::UnitTest {
public:
    void testConstructionAndEncoding() {
        const auto u8Stream = el::stream::StringBuilderStream{StringKind::U8};
        const auto u16Stream = el::stream::StringBuilderStream{StringKind::U16};
        const auto u32Stream = el::stream::StringBuilderStream{StringKind::U32};

        REQUIRE_EQUAL(u8Stream.kind(), StringKind::U8);
        REQUIRE_EQUAL(u8Stream.encoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(u8Stream.effectiveEncoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(u16Stream.kind(), StringKind::U16);
        REQUIRE_EQUAL(u16Stream.encoding(), StringEncoding::Utf16);
        REQUIRE_EQUAL(u16Stream.effectiveEncoding(), StringEncoding::Utf16);
        REQUIRE_EQUAL(u32Stream.kind(), StringKind::U32);
        REQUIRE_EQUAL(u32Stream.encoding(), StringEncoding::Utf32);
        REQUIRE_EQUAL(u32Stream.effectiveEncoding(), StringEncoding::Utf32);
        REQUIRE(u8Stream.isOpen());
        REQUIRE(u8Stream.isEmpty());
        REQUIRE(u8Stream.length().isZero());
    }

    void testWriteMethodsAndClear() {
        using namespace el::text::literals;

        auto stream = el::stream::StringBuilderStream{StringKind::U16};

        stream.write("Hello"_el);
        stream.write(Char{U' '});
        stream.writeLine("World"_el);
        stream.writeLine();

        REQUIRE_EQUAL(stream.length(), CpLength{13U});
        REQUIRE_EQUAL(StringConverter{stream.toU8String()}.toStdString(), std::string{"Hello World\n\n"});
        REQUIRE_EQUAL(StringConverter{stream.toString()}.toStdString(), std::string{"Hello World\n\n"});
        REQUIRE_EQUAL(StringConverter{stream.toU16String()}.toStdString(), std::string{"Hello World\n\n"});
        REQUIRE_EQUAL(StringConverter{stream.toU32String()}.toStdString(), std::string{"Hello World\n\n"});
        REQUIRE_EQUAL(StringConverter{stream.toAnyString().toU8String()}.toStdString(), std::string{"Hello World\n\n"});

        stream.clear();

        REQUIRE(stream.isEmpty());
        REQUIRE(stream.length().isZero());
        REQUIRE_EQUAL(stream.kind(), StringKind::U16);
    }

    void testPrintWritesThroughBuilderContext() {
        auto stream = el::stream::StringBuilderStream{StringKind::U32};
        auto integerFormat = IntegerFormat::hexadecimal();
        integerFormat.setFlags(IntegerFormatFlag::BasePrefix);

        stream.printLine("value=", integerFormat, std::uint16_t{255U}, ", sat=", el::math::SatInt32{42});
        stream.printLine("size=", std::size_t{7U}, ", signed=", std::int8_t{-8});

        REQUIRE_EQUAL(
            StringConverter{stream.toU8String()}.toStdString(),
            std::string{"value=0xff, sat=0x2a\nsize=7, signed=-8\n"});
    }

    void testPrintByteBlocks() {
        auto stream = el::stream::StringBuilderStream{StringKind::U8};
        const auto block = el::mem::ByteBlock{std::vector<std::uint8_t>{0xabU, 0x01U, 0x02U, 0x03U, 0xefU}};

        stream.printLine("hash: ", block);

        auto dump = ByteFormat::memoryDump().setBytesPerLine(el::unit::ByteLength{4U});
        dump.setByteGroupSize(el::unit::ByteLength{2U});
        stream.print(dump, block);

        REQUIRE_EQUAL(
            StringConverter{stream.toU8String()}.toStdString(),
            std::string{"hash: ab010203ef\n00000000 | ab01 0203\n00000004 | ef\n"});
    }

    void testTakeMethodsResetButKeepKind() {
        auto stream = el::stream::StringBuilderStream{StringKind::U32};

        stream.writeLine(String{std::string_view{"first"}});
        const auto first = stream.takeU8String();

        REQUIRE_EQUAL(StringConverter{first}.toStdString(), std::string{"first\n"});
        REQUIRE(stream.isEmpty());
        REQUIRE_EQUAL(stream.kind(), StringKind::U32);

        stream.writeLine(String{std::string_view{"second"}});
        const auto second = stream.takeU16String();

        REQUIRE_EQUAL(StringConverter{second}.toStdString(), std::string{"second\n"});
        REQUIRE(stream.isEmpty());
        REQUIRE_EQUAL(stream.kind(), StringKind::U32);

        stream.writeLine(String{std::string_view{"third"}});
        const auto third = stream.takeU32String();

        REQUIRE_EQUAL(StringConverter{third}.toStdString(), std::string{"third\n"});
        REQUIRE(stream.isEmpty());
        REQUIRE_EQUAL(stream.kind(), StringKind::U32);

        stream.writeLine(String{std::string_view{"fourth"}});
        const auto fourth = stream.takeAnyString();

        REQUIRE_EQUAL(StringConverter{fourth.toU8String()}.toStdString(), std::string{"fourth\n"});
        REQUIRE(stream.isEmpty());
        REQUIRE_EQUAL(stream.kind(), StringKind::U32);
    }

    void testFlushAndCloseAreIgnored() {
        auto stream = el::stream::StringBuilderStream{StringKind::U8};

        stream.writeLine(String{std::string_view{"open"}});
        stream.flush();
        stream.close();

        REQUIRE(stream.isOpen());
        stream.writeLine(String{std::string_view{"still open"}});
        REQUIRE_EQUAL(StringConverter{stream.toU8String()}.toStdString(), std::string{"open\nstill open\n"});
    }
};
