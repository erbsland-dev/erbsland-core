// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/ByteIntegerFormat.hpp>
#include <erbsland/mem/ByteReader.hpp>
#include <erbsland/mem/ByteTextOptions.hpp>
#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/mem/Endianness.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringEncoding.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <concepts>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

using el::mem::Byte;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::mem::ByteIntegerFormat;
using el::mem::ByteReader;
using el::mem::ByteTextFormat;
using el::mem::ByteTextOptions;
using el::mem::ByteWriter;
using el::mem::Endianness;
using el::unit::ByteIndex;
using el::unit::ByteLength;
using namespace el::text::literals;

template <typename T>
concept ReaderAcceptsInteger = requires(ByteReader &reader, T value) { reader.readInteger(value); };

template <typename T>
concept WriterAcceptsInteger = requires(ByteWriter &writer, T value) { writer.writeInteger(value); };

static_assert(ReaderAcceptsInteger<int8_t>);
static_assert(ReaderAcceptsInteger<uint64_t>);
static_assert(!ReaderAcceptsInteger<bool>);
static_assert(!ReaderAcceptsInteger<char>);
static_assert(!ReaderAcceptsInteger<ByteIntegerFormat::Value>);
static_assert(WriterAcceptsInteger<int8_t>);
static_assert(WriterAcceptsInteger<uint64_t>);
static_assert(!WriterAcceptsInteger<bool>);
static_assert(!WriterAcceptsInteger<char>);
static_assert(!WriterAcceptsInteger<ByteIntegerFormat::Value>);
constexpr auto cCompactTextOptions = ByteTextOptions::compact();
static_assert(cCompactTextOptions.countFormat().has_value());
static_assert(*cCompactTextOptions.countFormat() == ByteIntegerFormat::UnsignedVariableLength);

TESTED_TARGETS(ByteReader ByteWriter ByteIntegerFormat ByteTextOptions Endianness)
class ByteReaderWriterTest final : public el::UnitTest {
public:
    void testByteReaderBasics() {
        auto reader = ByteReader{makeBlock({1U, 2U})};

        REQUIRE_EQUAL(reader.length(), ByteLength{2U});
        REQUIRE_EQUAL(reader.position(), ByteIndex::zero());
        REQUIRE_FALSE(reader.isAtEnd());
        REQUIRE_EQUAL(reader.peekByte(), Byte{1U});
        REQUIRE_EQUAL(reader.position(), ByteIndex::zero());
        REQUIRE_EQUAL(reader.readByte(), Byte{1U});
        REQUIRE_EQUAL(reader.position(), ByteIndex{1U});
        REQUIRE_EQUAL(reader.readByte(), Byte{2U});
        REQUIRE(reader.isAtEnd());
        REQUIRE_EQUAL(reader.readByte(), Byte{});
        REQUIRE_EQUAL(reader.position(), ByteIndex{2U});
        REQUIRE_THROWS(reader.readByteOrThrow());
    }

    void testByteReaderLookaheadAndAdvance() {
        auto reader = ByteReader{makeBlock({1U, 2U, 3U})};

        REQUIRE(reader.canRead(3U));
        REQUIRE_FALSE(reader.canRead(4U));
        REQUIRE_EQUAL(reader.peekByte(1U), Byte{2U});
        REQUIRE_EQUAL(reader.peekByte(ByteIndex{2U}), Byte{3U});
        REQUIRE_EQUAL(reader.peekByte(99U, Byte{9U}), Byte{9U});
        REQUIRE_EQUAL(reader.position(), ByteIndex::zero());

        reader.advance(2U);
        REQUIRE_EQUAL(reader.position(), ByteIndex{2U});
        REQUIRE(reader.canRead(1U));
        REQUIRE_FALSE(reader.canRead(2U));

        reader.advance(99U);
        REQUIRE(reader.isAtEnd());
        REQUIRE_EQUAL(reader.position(), ByteIndex{3U});
    }

    void testByteReaderReadsByteBlocks() {
        auto reader = ByteReader{makeBlock({1U, 2U, 3U})};

        REQUIRE_EQUAL(reader.readBytes(ByteLength{2U}).value(), makeBlock({1U, 2U}));
        REQUIRE_EQUAL(reader.position(), ByteIndex{2U});
        REQUIRE_FALSE(reader.readBytes(ByteLength{2U}).has_value());
        REQUIRE_EQUAL(reader.position(), ByteIndex{2U});
        REQUIRE_EQUAL(reader.readBytesOrThrow(ByteLength{1U}), makeBlock({3U}));
        REQUIRE(reader.isAtEnd());
        REQUIRE_THROWS(reader.readBytesOrThrow(ByteLength{1U}));
    }

    void testReaderDefaultFailureDoesNotAdvance() {
        auto reader = ByteReader{makeBlock({0x34U})};

        REQUIRE_EQUAL(reader.readUInt16(0x9999U), uint16_t{0x9999U});
        REQUIRE_EQUAL(reader.position(), ByteIndex::zero());

        auto value = uint16_t{0};
        REQUIRE_FALSE(reader.readIntegerInto(value));
        REQUIRE_EQUAL(reader.position(), ByteIndex::zero());
    }

    void testEndianIntegerReads() {
        auto reader = ByteReader{makeBlock({0x34U, 0x12U, 0xabu, 0xcdu})};

        REQUIRE_EQUAL(reader.endianness(), Endianness::Little);
        REQUIRE_EQUAL(reader.readUInt16(), uint16_t{0x1234U});

        reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(reader.readUInt16(), uint16_t{0xabcdU});
        REQUIRE(reader.isAtEnd());
        REQUIRE_THROWS(reader.readUInt16OrThrow());
    }

    void testSignedReadsAndPositionClamping() {
        auto reader = ByteReader{makeBlock({0xffU, 0xfeU, 0xffU})};
        reader.setPosition(ByteIndex{99U});
        REQUIRE(reader.isAtEnd());

        reader.setPosition(ByteIndex::zero());
        REQUIRE_EQUAL(reader.readInt8(), int8_t{-1});
        REQUIRE_EQUAL(reader.readInt16(), int16_t{-2});
    }

    void testWriterBasicsAndOverwrite() {
        auto writer = ByteWriter{};
        writer.reserve(ByteLength{8U});
        writer.writeByte(Byte{1U}).writeByte(Byte{2U});
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        writer.setPosition(ByteIndex{1U});
        writer.writeByte(Byte{9U});
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({1U, 9U}));

        writer.setPosition(ByteIndex{99U});
        writer.writeByte(Byte{3U});
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({1U, 9U, 3U}));
    }

    void testEndianIntegerWrites() {
        auto writer = ByteWriter{};
        writer.writeUInt16(0x1234U);
        writer.setEndianness(Endianness::Big);
        writer.writeUInt16(0xabcdU);
        writer.writeInt8(-1);

        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0x34U, 0x12U, 0xabU, 0xcdU, 0xffU}));

        auto reader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(reader.readUInt16(), uint16_t{0x1234U});
        reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(reader.readUInt16(), uint16_t{0xabcdU});
        REQUIRE_EQUAL(reader.readInt8(), int8_t{-1});
    }

    void testWriterReturnsSharedByteBlock() {
        auto writer = ByteWriter{};
        writer.writeUInt8(1U).writeUInt8(2U);

        const auto first = writer.toByteBlock();
        auto editor = ByteBlockEditor{first};
        editor.set(ByteIndex{0U}, Byte{9U});
        writer.writeUInt8(3U);

        REQUIRE_EQUAL(first.toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));
        REQUIRE_EQUAL(editor.toUInt8Vector(), std::vector<uint8_t>({9U, 2U}));
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({1U, 2U, 3U}));
    }

    void testWriterResetAndTake() {
        auto writer = ByteWriter{};
        writer.setEndianness(Endianness::Big);
        writer.writeUInt16(0x1234U);

        const auto taken = writer.takeByteBlockEditor();
        REQUIRE_EQUAL(taken.toUInt8Vector(), std::vector<uint8_t>({0x12U, 0x34U}));
        REQUIRE_EQUAL(writer.length(), ByteLength::zero());
        REQUIRE_EQUAL(writer.position(), ByteIndex::zero());
        REQUIRE_EQUAL(writer.endianness(), Endianness::Big);

        writer.writeUInt8(1U);
        writer.reset();
        REQUIRE_EQUAL(writer.length(), ByteLength::zero());
        REQUIRE_EQUAL(writer.position(), ByteIndex::zero());
        REQUIRE_EQUAL(writer.endianness(), Endianness::Big);
    }

    void testWriterWritesByteBlocksAndSpans() {
        auto writer = ByteWriter{};
        const auto block = makeBlock({1U, 2U, 3U});
        const auto bytes = std::array<std::byte, 2U>{std::byte{4U}, std::byte{5U}};

        writer.writeBytes(block.span()).writeBytes(std::span{bytes}).writeBytes(block);
        REQUIRE_EQUAL(writer.toByteBlock(), makeBlock({1U, 2U, 3U, 4U, 5U, 1U, 2U, 3U}));

        writer.setPosition(ByteIndex{2U});
        writer.writeBytes(makeBlock({9U, 8U, 7U, 6U}));
        REQUIRE_EQUAL(writer.toByteBlock(), makeBlock({1U, 2U, 9U, 8U, 7U, 6U, 2U, 3U}));
    }

    void testTextReadWriteRoundTrip() {
        const auto text = el::text::String{"A¢😀"_el};
        auto writer = ByteWriter{};
        writer.setEndianness(Endianness::Big);
        const auto utf16 = ByteTextOptions{el::text::StringEncoding::Utf16BigEndian};
        writer.writeTextOrThrow(text).writeTextOrThrow("A"_el, utf16);

        REQUIRE_EQUAL(
            writer.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>(
                {0U, 0U, 0U, 7U, 0x41U, 0xc2U, 0xa2U, 0xf0U, 0x9fU, 0x98U, 0x80U, 0U, 0U, 0U, 1U, 0U, 0x41U}));

        auto reader = ByteReader{writer.toByteBlock()};
        reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(reader.readText().value(), text);
        REQUIRE_EQUAL(reader.readText(utf16).value(), "A"_el);
        REQUIRE(reader.isAtEnd());
    }

    void testTextReadWaitsForTheCompleteValue() {
        auto reader = ByteReader{makeBlock({3U, 0U, 0U, 0U, 0x41U, 0x42U})};

        REQUIRE_FALSE(reader.readText().has_value());
        REQUIRE_EQUAL(reader.position(), ByteIndex::zero());
    }

    void testIntegerFormatsAndConversions() {
        REQUIRE_EQUAL(ByteIntegerFormat{ByteIntegerFormat::UnsignedFixed16Bit}.byteCount(), ByteLength{2U});
        REQUIRE_EQUAL(ByteIntegerFormat{ByteIntegerFormat::UnsignedFixed24Bit}.byteCount(), ByteLength{3U});
        REQUIRE(ByteIntegerFormat{ByteIntegerFormat::SignedVariableLength}.isSigned());
        REQUIRE_FALSE(ByteIntegerFormat{ByteIntegerFormat::UnsignedVariableLength}.isSigned());
        REQUIRE(ByteIntegerFormat{ByteIntegerFormat::UnsignedVariableLength}.byteCount().isInfinite());
        REQUIRE_FALSE(ByteIntegerFormat{ByteIntegerFormat::UnsignedBase128}.isSigned());

        auto writer = ByteWriter{};
        writer.setEndianness(Endianness::Big);
        writer.writeIntegerOrThrow<int>(-128, ByteIntegerFormat::SignedFixed8Bit)
            .writeIntegerOrThrow<unsigned>(255U, ByteIntegerFormat::UnsignedFixed8Bit)
            .writeIntegerOrThrow<uint32_t>(0x123456U, ByteIntegerFormat::UnsignedFixed24Bit)
            .writeIntegerOrThrow<int64_t>(-1, ByteIntegerFormat::SignedVariableLength)
            .writeIntegerOrThrow<int64_t>(64, ByteIntegerFormat::SignedVariableLength)
            .writeIntegerOrThrow<uint64_t>(0x1234U, ByteIntegerFormat::UnsignedVariableLength)
            .writeIntegerOrThrow<uint64_t>(uint64_t{1} << 56U, ByteIntegerFormat::UnsignedVariableLength);
        REQUIRE_EQUAL(
            writer.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>(
                {0x80U,
                    0xffU,
                    0x12U,
                    0x34U,
                    0x56U,
                    0x01U,
                    0x80U,
                    0x80U,
                    0x92U,
                    0x34U,
                    0xffU,
                    0x01U,
                    0U,
                    0U,
                    0U,
                    0U,
                    0U,
                    0U,
                    0U}));

        auto reader = ByteReader{writer.toByteBlock()};
        reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<int16_t>(ByteIntegerFormat::SignedFixed8Bit), int16_t{-128});
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint16_t>(ByteIntegerFormat::UnsignedFixed8Bit), uint16_t{255U});
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint32_t>(ByteIntegerFormat::UnsignedFixed24Bit), uint32_t{0x123456U});
        REQUIRE_EQUAL(reader.readIntegerOrThrow<int>(ByteIntegerFormat::SignedVariableLength), -1);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<unsigned>(ByteIntegerFormat::SignedVariableLength), 64U);
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<uint32_t>(ByteIntegerFormat::UnsignedVariableLength), uint32_t{0x1234U});
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedVariableLength), uint64_t{1} << 56U);
        REQUIRE_THROWS(writer.writeIntegerOrThrow<int>(-1, ByteIntegerFormat::UnsignedFixed8Bit));
    }

    void testFormattedIntegerFailuresAreTransactional() {
        auto incomplete = ByteReader{makeBlock({0x80U})};
        REQUIRE_FALSE(incomplete.readInteger<uint64_t>(ByteIntegerFormat::UnsignedVariableLength).has_value());
        REQUIRE_EQUAL(incomplete.position(), ByteIndex::zero());

        auto overflow = ByteReader{makeBlock({0x01U, 0x00U})};
        overflow.setEndianness(Endianness::Big);
        REQUIRE_FALSE(overflow.readInteger<uint8_t>(ByteIntegerFormat::UnsignedFixed16Bit).has_value());
        REQUIRE_EQUAL(overflow.position(), ByteIndex::zero());
        REQUIRE_THROWS(overflow.readIntegerOrThrow<uint8_t>(ByteIntegerFormat::UnsignedFixed16Bit));

        auto incompleteBase128 = ByteReader{makeBlock({0x81U})};
        REQUIRE_FALSE(incompleteBase128.readInteger<uint64_t>(ByteIntegerFormat::UnsignedBase128).has_value());
        REQUIRE_EQUAL(incompleteBase128.position(), ByteIndex::zero());

        auto overflowingBase128 =
            ByteReader{makeBlock({0x82U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x80U, 0x00U})};
        REQUIRE_FALSE(overflowingBase128.readInteger<uint64_t>(ByteIntegerFormat::UnsignedBase128).has_value());
        REQUIRE_EQUAL(overflowingBase128.position(), ByteIndex::zero());

        auto nonminimalBase128 = ByteReader{makeBlock({0x80U, 0x00U})};
        REQUIRE_FALSE(nonminimalBase128.readInteger<uint64_t>(ByteIntegerFormat::UnsignedBase128).has_value());
        REQUIRE_EQUAL(nonminimalBase128.position(), ByteIndex::zero());
    }

    void testUnsignedBase128IntegerFormat() {
        auto writer = ByteWriter{};
        writer.writeIntegerOrThrow<uint64_t>(0U, ByteIntegerFormat::UnsignedBase128)
            .writeIntegerOrThrow<uint64_t>(127U, ByteIntegerFormat::UnsignedBase128)
            .writeIntegerOrThrow<uint64_t>(128U, ByteIntegerFormat::UnsignedBase128)
            .writeIntegerOrThrow<uint64_t>(16384U, ByteIntegerFormat::UnsignedBase128)
            .writeIntegerOrThrow<uint64_t>(std::numeric_limits<uint64_t>::max(), ByteIntegerFormat::UnsignedBase128);
        REQUIRE_EQUAL(
            writer.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>(
                {0x00U,
                    0x7fU,
                    0x81U,
                    0x00U,
                    0x81U,
                    0x80U,
                    0x00U,
                    0x81U,
                    0xffU,
                    0xffU,
                    0xffU,
                    0xffU,
                    0xffU,
                    0xffU,
                    0xffU,
                    0xffU,
                    0x7fU}));

        auto reader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedBase128), 0U);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedBase128), 127U);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedBase128), 128U);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedBase128), 16384U);
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedBase128),
            std::numeric_limits<uint64_t>::max());
        REQUIRE(reader.isAtEnd());
    }

    void testOddByteWidthIntegerFormats() {
        auto writer = ByteWriter{};
        writer.setEndianness(Endianness::Big);
        writer.writeIntegerOrThrow<uint64_t>(0x123456U, ByteIntegerFormat::UnsignedFixed24Bit)
            .writeIntegerOrThrow<uint64_t>(0x123456789aU, ByteIntegerFormat::UnsignedFixed40Bit)
            .writeIntegerOrThrow<uint64_t>(0x123456789abcU, ByteIntegerFormat::UnsignedFixed48Bit)
            .writeIntegerOrThrow<uint64_t>(0x123456789abcdeU, ByteIntegerFormat::UnsignedFixed56Bit);
        REQUIRE_EQUAL(
            writer.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>(
                {0x12U,
                    0x34U,
                    0x56U,
                    0x12U,
                    0x34U,
                    0x56U,
                    0x78U,
                    0x9aU,
                    0x12U,
                    0x34U,
                    0x56U,
                    0x78U,
                    0x9aU,
                    0xbcU,
                    0x12U,
                    0x34U,
                    0x56U,
                    0x78U,
                    0x9aU,
                    0xbcU,
                    0xdeU}));

        auto reader = ByteReader{writer.toByteBlock()};
        reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedFixed24Bit), 0x123456U);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedFixed40Bit), 0x123456789aU);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedFixed48Bit), 0x123456789abcU);
        REQUIRE_EQUAL(reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedFixed56Bit), 0x123456789abcdeU);
        REQUIRE(reader.isAtEnd());
    }

    void testFormattedIntegerBoundaries() {
        auto writer = ByteWriter{};
        writer.setEndianness(Endianness::Big);
        writer.writeIntegerOrThrow<int8_t>(std::numeric_limits<int8_t>::min(), ByteIntegerFormat::SignedFixed8Bit)
            .writeIntegerOrThrow<int8_t>(std::numeric_limits<int8_t>::max(), ByteIntegerFormat::SignedFixed8Bit)
            .writeIntegerOrThrow<uint8_t>(std::numeric_limits<uint8_t>::max(), ByteIntegerFormat::UnsignedFixed8Bit)
            .writeIntegerOrThrow<int16_t>(std::numeric_limits<int16_t>::min(), ByteIntegerFormat::SignedFixed16Bit)
            .writeIntegerOrThrow<int32_t>(std::numeric_limits<int32_t>::max(), ByteIntegerFormat::SignedFixed32Bit)
            .writeIntegerOrThrow<int64_t>(std::numeric_limits<int64_t>::min(), ByteIntegerFormat::SignedFixed64Bit)
            .writeIntegerOrThrow<uint64_t>(std::numeric_limits<uint64_t>::max(), ByteIntegerFormat::UnsignedFixed64Bit)
            .writeIntegerOrThrow<int64_t>(std::numeric_limits<int64_t>::min(), ByteIntegerFormat::SignedVariableLength)
            .writeIntegerOrThrow<int64_t>(std::numeric_limits<int64_t>::max(), ByteIntegerFormat::SignedVariableLength)
            .writeIntegerOrThrow<uint64_t>(
                std::numeric_limits<uint64_t>::max(), ByteIntegerFormat::UnsignedVariableLength);

        auto reader = ByteReader{writer.toByteBlock()};
        reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int8_t>(ByteIntegerFormat::SignedFixed8Bit), std::numeric_limits<int8_t>::min());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int8_t>(ByteIntegerFormat::SignedFixed8Bit), std::numeric_limits<int8_t>::max());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<uint8_t>(ByteIntegerFormat::UnsignedFixed8Bit),
            std::numeric_limits<uint8_t>::max());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int16_t>(ByteIntegerFormat::SignedFixed16Bit),
            std::numeric_limits<int16_t>::min());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int32_t>(ByteIntegerFormat::SignedFixed32Bit),
            std::numeric_limits<int32_t>::max());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int64_t>(ByteIntegerFormat::SignedFixed64Bit),
            std::numeric_limits<int64_t>::min());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedFixed64Bit),
            std::numeric_limits<uint64_t>::max());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int64_t>(ByteIntegerFormat::SignedVariableLength),
            std::numeric_limits<int64_t>::min());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<int64_t>(ByteIntegerFormat::SignedVariableLength),
            std::numeric_limits<int64_t>::max());
        REQUIRE_EQUAL(
            reader.readIntegerOrThrow<uint64_t>(ByteIntegerFormat::UnsignedVariableLength),
            std::numeric_limits<uint64_t>::max());
        REQUIRE(reader.isAtEnd());
    }

    void testFormattedIntegerWriteFailuresAreTransactional() {
        auto writer = ByteWriter{};
        writer.writeByte(Byte{0x42U});

        REQUIRE_THROWS(writer.writeIntegerOrThrow<int16_t>(-129, ByteIntegerFormat::SignedFixed8Bit));
        REQUIRE_THROWS(writer.writeIntegerOrThrow<int16_t>(128, ByteIntegerFormat::SignedFixed8Bit));
        REQUIRE_THROWS(writer.writeIntegerOrThrow<uint16_t>(256U, ByteIntegerFormat::UnsignedFixed8Bit));
        REQUIRE_THROWS(writer.writeIntegerOrThrow<int8_t>(-1, ByteIntegerFormat::UnsignedFixed8Bit));
        REQUIRE_THROWS(writer.writeIntegerOrThrow<uint64_t>(
            std::numeric_limits<uint64_t>::max(), ByteIntegerFormat::SignedVariableLength));
        REQUIRE_EQUAL(writer.toByteBlock(), makeBlock({0x42U}));
    }

    void testTextEndMarksAndPaddedFields() {
        auto dynamic = ByteTextOptions{};
        dynamic.clearCountFormat().setEndMark(U'|').setLength(ByteLength{8U});
        auto writer = ByteWriter{};
        writer.writeTextOrThrow("A¢"_el, dynamic);
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0x41U, 0xc2U, 0xa2U, 0x7cU}));
        auto reader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(reader.readText(dynamic).value(), "A¢"_el);

        auto bounded = ByteTextOptions{};
        bounded.clearCountFormat().setEndMark(U'!').setLength(ByteLength{1U});
        writer.reset();
        writer.writeTextOrThrow("A"_el, bounded);
        auto boundedReader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(boundedReader.readTextOrThrow(bounded), "A"_el);

        auto invalid = ByteReader{makeBlock({0x41U, 0x21U})};
        REQUIRE_FALSE(invalid.readText(dynamic).has_value());
        REQUIRE_EQUAL(invalid.position(), ByteIndex::zero());

        auto padded = ByteTextOptions{ByteTextFormat::PaddedField};
        padded.setLength(ByteLength{8U}).setPadding(Byte{0x20U}).setEndMark(U'!');
        writer.reset();
        writer.writeTextOrThrow("A"_el, padded);
        auto paddedReader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(paddedReader.readTextOrThrow(padded), "A"_el);
        REQUIRE(paddedReader.isAtEnd());
    }

    void testTextUnicodeEncodingsAndTruncation() {
        const auto text = el::text::String{"A😀"_el};
        auto utf16 = ByteTextOptions{el::text::StringEncoding::Utf16BigEndian};
        utf16.setLength(ByteLength{6U});
        auto writer = ByteWriter{};
        writer.writeTextOrThrow(text, utf16);
        auto reader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(reader.readTextOrThrow(utf16), text);

        auto utf32 = ByteTextOptions{el::text::StringEncoding::Utf32LittleEndian};
        writer.reset();
        writer.writeTextOrThrow(text, utf32);
        reader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(reader.readTextOrThrow(utf32), text);

        auto truncated = ByteTextOptions{};
        truncated.setLength(ByteLength{4U});
        writer.reset();
        writer.writeText(text, truncated);
        reader = ByteReader{writer.toByteBlock()};
        REQUIRE_EQUAL(reader.readTextOrThrow(truncated), "A"_el);
        REQUIRE_THROWS(writer.writeTextOrThrow(text, truncated));
    }

    void testTextReadsAreTransactionalAtNonzeroPositions() {
        auto terminated = ByteTextOptions{};
        terminated.clearCountFormat().setEndMark(U'|').setLength(ByteLength{8U});
        auto reader = ByteReader{makeBlock({0xffU, 0x41U, 0x7cU, 0x42U})};
        reader.setPosition(ByteIndex{1U});
        REQUIRE_EQUAL(reader.readTextOrThrow(terminated), "A"_el);
        REQUIRE_EQUAL(reader.position(), ByteIndex{3U});

        auto missingMark = ByteReader{makeBlock({0xffU, 0x41U, 0x42U})};
        missingMark.setPosition(ByteIndex{1U});
        REQUIRE_FALSE(missingMark.readText(terminated).has_value());
        REQUIRE_EQUAL(missingMark.position(), ByteIndex{1U});

        auto incompleteCount = ByteReader{makeBlock({0xffU, 0x01U, 0x00U, 0x00U})};
        incompleteCount.setPosition(ByteIndex{1U});
        REQUIRE_FALSE(incompleteCount.readText().has_value());
        REQUIRE_EQUAL(incompleteCount.position(), ByteIndex{1U});

        auto misaligned = ByteTextOptions{el::text::StringEncoding::Utf16LittleEndian};
        misaligned.clearCountFormat().setEndMark(U'!').setLength(ByteLength{6U});
        auto misalignedReader = ByteReader{makeBlock({0xffU, 0x41U, 0x00U, 0x00U, 0x21U, 0x00U})};
        misalignedReader.setPosition(ByteIndex{1U});
        REQUIRE_FALSE(misalignedReader.readText(misaligned).has_value());
        REQUIRE_EQUAL(misalignedReader.position(), ByteIndex{1U});

        auto padded = ByteTextOptions{ByteTextFormat::PaddedField};
        padded.clearCountFormat().setEndMark(U'!').setLength(ByteLength{4U}).setPadding(Byte{0x20U});
        auto paddedReader = ByteReader{makeBlock({0xffU, 0x41U, 0x21U, 0x20U, 0x20U, 0xeeU})};
        paddedReader.setPosition(ByteIndex{1U});
        REQUIRE_EQUAL(paddedReader.readTextOrThrow(padded), "A"_el);
        REQUIRE_EQUAL(paddedReader.position(), ByteIndex{5U});
    }

private:
    [[nodiscard]] static auto makeBlock(std::initializer_list<uint8_t> bytes) -> ByteBlock {
        return ByteBlock::fromVector(std::vector<uint8_t>{bytes});
    }
};
