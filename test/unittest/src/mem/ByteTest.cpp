// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/Byte.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>

using el::mem::Byte;

static_assert(sizeof(Byte) == 1U);
static_assert(Byte{0x81U}.rotatedLeft(1).toUInt8() == 0x03U);
static_assert(Byte{0x81U}.shiftedRight(7).toUInt8() == 0x01U);
static_assert(Byte{0xffU}.shiftedLeft(8).toUInt8() == 0U);

TESTED_TARGETS(Byte)
class ByteTest final : public el::UnitTest {
public:
    void testConstructionAndConversion() {
        const auto fromUInt8 = Byte{uint8_t{0xabU}};
        const auto fromStdByte = Byte{std::byte{0xcdU}};

        REQUIRE_EQUAL(fromUInt8.toStdByte(), std::byte{0xabU});
        REQUIRE_EQUAL(fromUInt8.toUInt8(), uint8_t{0xabU});
        REQUIRE_EQUAL(fromUInt8.toUInt16(), uint16_t{0xabU});
        REQUIRE_EQUAL(fromUInt8.toUInt32(), uint32_t{0xabU});
        REQUIRE_EQUAL(fromUInt8.toUInt64(), uint64_t{0xabU});
        REQUIRE_EQUAL(fromUInt8.toRawValue(), std::byte{0xabU});
        REQUIRE_EQUAL(fromStdByte, Byte{0xcdU});
        REQUIRE_EQUAL(Byte::fromChar('A').toChar(), 'A');
        REQUIRE_EQUAL(Byte::fromUInt8(0x42U), Byte{0x42U});
        REQUIRE_EQUAL(Byte::fromCroppedUInt16(0x12abU), Byte{0xabU});
        REQUIRE_EQUAL(Byte::fromCroppedUInt32(0x123456cdU), Byte{0xcdU});
        REQUIRE_EQUAL(Byte::fromCroppedUInt64(0x123456789abcdef0ULL), Byte{0xf0U});
    }

    void testComparisonAndBitwiseOperators() {
        const auto first = Byte{0b10101100U};
        const auto second = Byte{0b11000011U};

        REQUIRE(first < Byte{0xffU});
        REQUIRE_EQUAL(first | second, Byte{0b11101111U});
        REQUIRE_EQUAL(first & second, Byte{0b10000000U});
        REQUIRE_EQUAL(first ^ second, Byte{0b01101111U});
        REQUIRE_EQUAL(~first, Byte{0b01010011U});

        auto value = first;
        value |= second;
        REQUIRE_EQUAL(value, Byte{0b11101111U});
        value &= Byte{0xf0U};
        REQUIRE_EQUAL(value, Byte{0xe0U});
        value ^= Byte{0xffU};
        REQUIRE_EQUAL(value, Byte{0x1fU});
    }

    void testShifts() {
        const auto value = Byte{0b10010110U};

        REQUIRE_EQUAL(value << 0U, value);
        REQUIRE_EQUAL(value << 1U, Byte{0b00101100U});
        REQUIRE_EQUAL(value << 7U, Byte{});
        REQUIRE_EQUAL(value << 8U, Byte{});
        REQUIRE_EQUAL(value << 100U, Byte{});
        REQUIRE_EQUAL(value >> 1U, Byte{0b01001011U});
        REQUIRE_EQUAL(value >> 7U, Byte{1U});
        REQUIRE_EQUAL(value >> 8U, Byte{});

        auto shifted = value;
        shifted <<= 2U;
        REQUIRE_EQUAL(shifted, Byte{0b01011000U});
        shifted >>= 3U;
        REQUIRE_EQUAL(shifted, Byte{0b00001011U});
    }

    void testRotations() {
        const auto value = Byte{0x81U};

        REQUIRE_EQUAL(value.rotatedLeft(1), Byte{0x03U});
        REQUIRE_EQUAL(value.rotatedLeft(9), Byte{0x03U});
        REQUIRE_EQUAL(value.rotatedLeft(-1), Byte{0xc0U});
        REQUIRE_EQUAL(value.rotatedRight(1), Byte{0xc0U});
        REQUIRE_EQUAL(value.rotatedRight(-1), Byte{0x03U});

        auto rotated = value;
        rotated.rotateLeft(4);
        REQUIRE_EQUAL(rotated, Byte{0x18U});
        rotated.rotateRight(4);
        REQUIRE_EQUAL(rotated, value);
    }

    void testMasking() {
        const auto value = Byte{0b10101100U};

        REQUIRE_EQUAL(value.masked(Byte{0xf0U}), Byte{0xa0U});
        REQUIRE(value.matches(Byte{0xe0U}, Byte{0xa0U}));
        REQUIRE_FALSE(value.matches(Byte{0xf0U}, Byte{0xf0U}));
    }
};
