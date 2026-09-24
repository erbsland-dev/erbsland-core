// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/mem/BitReader.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

using el::mem::BitOrder;
using el::mem::BitReader;
using el::mem::Byte;
using el::mem::ByteBlock;

static_assert(!std::is_copy_constructible_v<BitReader>);
static_assert(!std::is_copy_assignable_v<BitReader>);
static_assert(std::is_nothrow_move_constructible_v<BitReader>);
static_assert(std::is_nothrow_move_assignable_v<BitReader>);

TESTED_TARGETS(BitReader BitOrder)
class BitReaderTest final : public el::UnitTest {
public:
    void testMostSignificantFirstFields() {
        auto reader = BitReader{ByteBlock{Byte{0xb2U}, Byte{0x61U}}};

        REQUIRE_EQUAL(reader.bitOrder(), BitOrder::MostSignificantFirst);
        REQUIRE_EQUAL(reader.readBits(4U), uint64_t{0x0bU});
        REQUIRE_EQUAL(reader.readBits(8U), uint64_t{0x26U});
        REQUIRE_EQUAL(reader.readBits(4U), uint64_t{0x01U});
        REQUIRE(reader.isAtEnd());
    }

    void testLeastSignificantFirstFields() {
        auto reader = BitReader{ByteBlock{Byte{0xb2U}, Byte{0x61U}}, BitOrder::LeastSignificantFirst};

        REQUIRE_EQUAL(reader.bitOrder(), BitOrder::LeastSignificantFirst);
        REQUIRE_EQUAL(reader.readBits(4U), uint64_t{0x02U});
        REQUIRE_EQUAL(reader.readBits(8U), uint64_t{0x1bU});
        REQUIRE_EQUAL(reader.readBits(4U), uint64_t{0x06U});
        REQUIRE(reader.isAtEnd());
    }

    void testFullWidthFields() {
        const auto data = ByteBlock{
            Byte{0x01U}, Byte{0x23U}, Byte{0x45U}, Byte{0x67U}, Byte{0x89U}, Byte{0xabU}, Byte{0xcdU}, Byte{0xefU}};
        auto mostFirst = BitReader{data};
        auto leastFirst = BitReader{data, BitOrder::LeastSignificantFirst};

        REQUIRE_EQUAL(mostFirst.readBitsOrThrow(64U), uint64_t{0x0123456789abcdefULL});
        REQUIRE_EQUAL(leastFirst.readBitsOrThrow(64U), uint64_t{0xefcdab8967452301ULL});
        REQUIRE_EQUAL(mostFirst.readBitsOrThrow(0U), uint64_t{});
    }

    void testReaderOwnsInput() {
        auto data = ByteBlock{Byte{0xa5U}};
        auto reader = BitReader{data};
        data = ByteBlock{};

        REQUIRE_EQUAL(reader.readByteOrThrow(), Byte{0xa5U});
    }

    void testTransactionalFailures() {
        auto reader = BitReader{ByteBlock{Byte{0xa5U}}};
        reader.advance(4U);

        REQUIRE_EQUAL(reader.readBits(5U, 0x77U), uint64_t{0x77U});
        REQUIRE_EQUAL(reader.bitPosition(), 4U);
        REQUIRE_EQUAL(reader.readByte(Byte{0x66U}), Byte{0x66U});
        REQUIRE_EQUAL(reader.bitPosition(), 4U);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.readBitsOrThrow(5U));
        REQUIRE_EQUAL(reader.bitPosition(), 4U);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.readBitsOrThrow(65U));
    }

    void testBitAndBytePositions() {
        auto reader = BitReader{ByteBlock{Byte{0xb2U}, Byte{0x61U}, Byte{0xffU}}, BitOrder::MostSignificantFirst, 3U};

        REQUIRE_EQUAL(reader.bitCount(), 24U);
        REQUIRE_EQUAL(reader.byteCount(), 3U);
        REQUIRE_EQUAL(reader.bitPosition(), 3U);
        REQUIRE_EQUAL(reader.bytePosition(), 0U);
        REQUIRE_EQUAL(reader.consumedByteCount(), 1U);
        REQUIRE_FALSE(reader.isByteAligned());
        REQUIRE(reader.canReadBytes(2U));
        reader.alignToByte();
        REQUIRE_EQUAL(reader.bitPosition(), 8U);
        REQUIRE(reader.isByteAligned());
        REQUIRE_EQUAL(reader.readByte(), Byte{0x61U});
        reader.setBytePosition(0U);
        REQUIRE_EQUAL(reader.readByte(), Byte{0xb2U});
        reader.advanceBytes(10U);
        REQUIRE(reader.isAtEnd());
        reader.setBitPosition(100U);
        REQUIRE_EQUAL(reader.bitPosition(), reader.bitCount());
    }

    void testMoveOperations() {
        auto source = BitReader{ByteBlock{Byte{0x80U}}};
        auto moved = std::move(source);
        REQUIRE(moved.readBool());

        auto assigned = BitReader{};
        assigned = std::move(moved);
        REQUIRE_FALSE(assigned.readBool());
        REQUIRE_EQUAL(assigned.bitPosition(), 2U);
    }

    void testRefillPreservesFieldsAndAlignment() {
        const auto data = ByteBlock{
            0x91U,
            0x23U,
            0x45U,
            0x67U,
            0x89U,
            0xabU,
            0xcdU,
            0xefU,
            0x10U,
            0x32U,
            0x54U,
            0x76U,
            0x98U,
            0xbaU,
            0xdcU,
            0xfeU,
            0x55U};
        for (const auto order : {BitOrder::MostSignificantFirst, BitOrder::LeastSignificantFirst}) {
            for (std::size_t offset{}; offset < 8U; ++offset) {
                for (std::size_t width{}; width <= 64U; ++width) {
                    auto expected = BitReader{data, order, offset};
                    auto reader = BitReader{data.slice(el::unit::ByteIndex{}, el::unit::ByteLength{1U}), order, offset};
                    for (std::size_t index{1U}; index < 9U; ++index) {
                        reader.refill(data.slice(el::unit::ByteIndex{index}, el::unit::ByteLength{1U}));
                        if (reader.canRead(width)) {
                            break;
                        }
                    }
                    REQUIRE_EQUAL(reader.readBitsOrThrow(width), expected.readBitsOrThrow(width));
                    REQUIRE_EQUAL(reader.bitPosition() % 8U, expected.bitPosition() % 8U);
                }
            }
        }
    }

    void testRefillIsBoundedAndTransactional() {
        const auto data = ByteBlock{0x91U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU, 0x10U};
        auto reader = BitReader{data};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.refill(ByteBlock{0xffU}));
        REQUIRE_EQUAL(reader.bitPosition(), 0U);
        REQUIRE_EQUAL(reader.byteCount(), 9U);
        reader.advance(8U);
        reader.refill(ByteBlock{0x32U});
        REQUIRE_EQUAL(reader.bitPosition(), 0U);
        REQUIRE_EQUAL(reader.readBitsOrThrow(64U), uint64_t{0x23456789abcdef10ULL});
        REQUIRE_EQUAL(reader.readByteOrThrow(), Byte{0x32U});
        reader.refill({});
        REQUIRE(reader.isAtEnd());
        reader.refill(ByteBlock{0xa5U});
        reader.advance(3U);
        reader.refill(ByteBlock{0x96U});
        reader.refill({});
        REQUIRE_EQUAL(reader.bitPosition(), 3U);
        reader.setBytePosition(1U);
        REQUIRE_EQUAL(reader.readByteOrThrow(), Byte{0x96U});
    }

    void testAlignedByteFields() {
        auto data = ByteBlock{0x91U, 0x23U, 0x45U, 0x67U};
        data.markAsSensitive();
        for (const auto order : {BitOrder::MostSignificantFirst, BitOrder::LeastSignificantFirst}) {
            auto reader = BitReader{data, order};
            reader.advanceBytes(1U);
            const auto field = reader.readBytesOrThrow(el::unit::ByteLength{2U});
            REQUIRE_EQUAL(field, ByteBlock({0x23U, 0x45U}));
            REQUIRE(field.span().data() == data.span().data() + 1U);
            REQUIRE(field.isSensitive());
            REQUIRE_EQUAL(reader.bitPosition(), 24U);
            REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.readBytesOrThrow(el::unit::ByteLength{2U}));
            REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.readBytesOrThrow(el::unit::ByteLength::infinite()));
            REQUIRE_EQUAL(reader.bitPosition(), 24U);
            reader.advance(1U);
            REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.readBytesOrThrow(el::unit::ByteLength{}));
            REQUIRE_EQUAL(reader.bitPosition(), 25U);
        }
        auto empty = BitReader{};
        REQUIRE(empty.readBytesOrThrow(el::unit::ByteLength{}).isEmpty());
    }

    void testByteFieldsAcrossRefillTail() {
        for (const auto order : {BitOrder::MostSignificantFirst, BitOrder::LeastSignificantFirst}) {
            auto reader = BitReader{ByteBlock{0x12U, 0x34U, 0x56U}, order};
            reader.advance(3U);
            reader.refill(ByteBlock{0x78U, 0x9aU});
            reader.alignToByte();
            const auto tail = reader.readBytesOrThrow(el::unit::ByteLength{1U});
            REQUIRE_EQUAL(tail, ByteBlock({0x34U}));
            REQUIRE(tail.isSensitive());
            const auto joined = reader.readBytesOrThrow(el::unit::ByteLength{2U});
            REQUIRE_EQUAL(joined, ByteBlock({0x56U, 0x78U}));
            REQUIRE(joined.isSensitive());
            REQUIRE_EQUAL(reader.readBytesOrThrow(el::unit::ByteLength{1U}), ByteBlock({0x9aU}));
            REQUIRE(reader.isAtEnd());
        }
    }

    void testEmptyReader() {
        auto reader = BitReader{};

        REQUIRE_EQUAL(reader.bitCount(), 0U);
        REQUIRE_EQUAL(reader.byteCount(), 0U);
        REQUIRE(reader.isAtEnd());
        REQUIRE(reader.isByteAligned());
        REQUIRE(reader.canRead(0U));
        REQUIRE_FALSE(reader.canRead(1U));
        REQUIRE_FALSE(reader.readBool());
    }
};
