// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/ByteReader.hpp>
#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/mem/Endianness.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <vector>

using el::mem::Byte;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::mem::ByteReader;
using el::mem::ByteWriter;
using el::mem::Endianness;
using el::unit::ByteIndex;
using el::unit::ByteLength;

TESTED_TARGETS(ByteReader ByteWriter Endianness)
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

private:
    [[nodiscard]] static auto makeBlock(std::initializer_list<uint8_t> bytes) -> ByteBlock {
        return ByteBlock::fromVector(std::vector<uint8_t>{bytes});
    }
};
