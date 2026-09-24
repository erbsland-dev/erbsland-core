// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/mem/BitReader.hpp>
#include <erbsland/mem/BitWriter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

using el::mem::BitOrder;
using el::mem::BitReader;
using el::mem::BitWriter;
using el::mem::Byte;
using el::mem::ByteBlock;

static_assert(!std::is_copy_constructible_v<BitWriter>);
static_assert(!std::is_copy_assignable_v<BitWriter>);
static_assert(std::is_nothrow_move_constructible_v<BitWriter>);
static_assert(std::is_nothrow_move_assignable_v<BitWriter>);

TESTED_TARGETS(BitWriter)
class BitWriterTest final : public el::UnitTest {
public:
    void testSensitiveStorageAcrossReuse() {
        auto writer = el::mem::BitWriter{};
        writer.markAsSensitive();
        writer.reserveBytes(16U);
        writer.writeBits(5U, 3U);
        REQUIRE(writer.toByteBlock().isSensitive());
        const auto shared = writer.toByteBlock();
        writer.reserveBytes(131072U);
        writer.writeByte(el::mem::Byte{0xa5U});
        REQUIRE(shared.isSensitive());
        REQUIRE(writer.toByteBlock().isSensitive());
        auto taken = writer.takeByteBlockEditor();
        REQUIRE(taken.isSensitive());
        writer.writeBits(1U, 1U);
        REQUIRE(writer.toByteBlock().isSensitive());
        writer.reset();
        writer.writeBits(2U, 2U);
        REQUIRE(writer.toByteBlock().isSensitive());
    }

    void testMostSignificantFirstFields() {
        auto writer = BitWriter{};
        writer.writeBits(0x0bU, 4U).writeByte(Byte{0x26U}).writeBits(0x01U, 4U);

        REQUIRE_EQUAL(writer.bitOrder(), BitOrder::MostSignificantFirst);
        REQUIRE_EQUAL(writer.bitCount(), 16U);
        REQUIRE_EQUAL(writer.byteCount(), 2U);
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0xb2U, 0x61U}));
    }

    void testLeastSignificantFirstFields() {
        auto writer = BitWriter{BitOrder::LeastSignificantFirst};
        writer.writeBits(0x02U, 4U).writeByte(Byte{0x1bU}).writeBits(0x06U, 4U);

        REQUIRE_EQUAL(writer.bitOrder(), BitOrder::LeastSignificantFirst);
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0xb2U, 0x61U}));
    }

    void testPartialByteAndAlignment() {
        auto writer = BitWriter{};
        writer.writeBits(0x05U, 3U);

        REQUIRE_EQUAL(writer.bitCount(), 3U);
        REQUIRE_EQUAL(writer.byteCount(), 1U);
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0xa0U}));
        REQUIRE_FALSE(writer.isByteAligned());
        writer.alignToByte();
        REQUIRE_EQUAL(writer.bitCount(), 8U);
        REQUIRE(writer.isByteAligned());
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0xa0U}));
    }

    void testOverwriteAndExtension() {
        auto writer = BitWriter{};
        writer.writeByte(Byte{0xaaU});
        writer.setBitPosition(4U);
        writer.writeByte(Byte{0x5cU});

        REQUIRE_EQUAL(writer.bitCount(), 12U);
        REQUIRE_EQUAL(writer.bitPosition(), 12U);
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0xa5U, 0xc0U}));
        writer.setBitPosition(2U);
        writer.writeBits(0U, 2U);
        REQUIRE_EQUAL(writer.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0x85U, 0xc0U}));
    }

    void testCursorAndStorageOperations() {
        auto writer = BitWriter{};
        writer.reserveBits(129U).reserveBytes(32U);
        writer.writeByte(Byte{0x12U}).writeByte(Byte{0x34U}).writeBool(true);

        REQUIRE_EQUAL(writer.bytePosition(), 2U);
        REQUIRE_EQUAL(writer.consumedByteCount(), 3U);
        REQUIRE_EQUAL(writer.remainingBitCount(), 0U);
        REQUIRE(writer.isAtEnd());
        writer.setBytePosition(1U);
        REQUIRE_EQUAL(writer.bitPosition(), 8U);
        writer.advanceBytes(10U);
        REQUIRE_EQUAL(writer.bitPosition(), writer.bitCount());
        const auto output = writer.takeByteBlockEditor();
        REQUIRE_EQUAL(ByteBlock{output}.toUInt8Vector(), std::vector<uint8_t>({0x12U, 0x34U, 0x80U}));
        REQUIRE_EQUAL(writer.bitCount(), 0U);
        REQUIRE_EQUAL(writer.byteCount(), 0U);
        writer.writeBool(true);
        writer.reset();
        REQUIRE_EQUAL(writer.bitCount(), 0U);
    }

    void testRoundTripBothOrders() {
        for (const auto order : {BitOrder::MostSignificantFirst, BitOrder::LeastSignificantFirst}) {
            auto writer = BitWriter{order};
            writer.writeBits(0x05U, 3U).writeBits(0x0123456789abcdefULL, 64U).writeBits(0x03U, 2U);
            auto reader = BitReader{writer.toByteBlock(), order};

            REQUIRE_EQUAL(reader.readBitsOrThrow(3U), uint64_t{0x05U});
            REQUIRE_EQUAL(reader.readBitsOrThrow(64U), uint64_t{0x0123456789abcdefULL});
            REQUIRE_EQUAL(reader.readBitsOrThrow(2U), uint64_t{0x03U});
        }
    }

    void testInvalidFieldWidth() {
        auto writer = BitWriter{};

        REQUIRE_THROWS_AS(el::err::OutOfRangeError, writer.writeBits(0U, 65U));
        REQUIRE_EQUAL(writer.bitCount(), 0U);
    }

    void testAlignedByteFieldsAndAliases() {
        const auto data = ByteBlock{0x12U, 0x34U, 0x56U, 0x78U};
        for (const auto order : {BitOrder::MostSignificantFirst, BitOrder::LeastSignificantFirst}) {
            auto writer = BitWriter{order};
            writer.markAsSensitive();
            writer.writeBytes(data.span());
            REQUIRE_EQUAL(writer.toByteBlock(), data);
            REQUIRE_EQUAL(writer.bitCount(), 32U);
            const auto original = writer.toByteBlock();
            writer.writeBytes(original.span());
            REQUIRE_EQUAL(writer.toByteBlock(), ByteBlock({0x12U, 0x34U, 0x56U, 0x78U, 0x12U, 0x34U, 0x56U, 0x78U}));
            REQUIRE_EQUAL(original, data);
            REQUIRE(writer.toByteBlock().isSensitive());
            writer.setBytePosition(1U);
            writer.writeBytes(data.span());
            REQUIRE_EQUAL(writer.toByteBlock(), ByteBlock({0x12U, 0x12U, 0x34U, 0x56U, 0x78U, 0x34U, 0x56U, 0x78U}));
            REQUIRE_EQUAL(writer.bitCount(), 64U);
            REQUIRE_EQUAL(writer.bitPosition(), 40U);
            writer.writeBool(true);
            const auto before = writer.toByteBlock();
            REQUIRE_THROWS_AS(el::err::OutOfRangeError, writer.writeBytes(data.span()));
            REQUIRE_EQUAL(writer.toByteBlock(), before);
            REQUIRE_EQUAL(writer.bitPosition(), 41U);
            writer.reset();
            writer.writeBool(true).setBitPosition(0U);
            writer.writeBytes(data.span());
            REQUIRE_EQUAL(writer.toByteBlock(), data);
            REQUIRE_EQUAL(writer.bitCount(), 32U);
            REQUIRE(writer.toByteBlock().isSensitive());
        }
    }

    void testMoveOperations() {
        auto source = BitWriter{BitOrder::LeastSignificantFirst};
        source.writeBits(0x03U, 2U);
        auto moved = std::move(source);
        moved.writeBits(0x01U, 2U);

        auto assigned = BitWriter{};
        assigned = std::move(moved);
        REQUIRE_EQUAL(assigned.bitOrder(), BitOrder::LeastSignificantFirst);
        REQUIRE_EQUAL(assigned.toByteBlock().toUInt8Vector(), std::vector<uint8_t>({0x07U}));
    }
};
