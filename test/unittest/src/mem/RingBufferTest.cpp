// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteRingBuffer.hpp>
#include <erbsland/mem/impl/UnsafeRingBufferAccess.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <span>
#include <vector>

using el::mem::Byte;
using el::unit::ByteLength;

TESTED_TARGETS(RingBuffer ByteRingBuffer UnsafeRingBufferAccess)
class RingBufferTest final : public el::UnitTest {
public:
    void testSafeWrapAround() {
        auto buffer = el::mem::RingBuffer{ByteLength{4U}};
        const auto first = std::array{Byte{1U}, Byte{2U}, Byte{3U}};
        REQUIRE_EQUAL(buffer.write(first), ByteLength{3U});

        auto read = std::array<Byte, 2>{};
        REQUIRE_EQUAL(buffer.read(read), ByteLength{2U});
        const auto second = std::array{Byte{4U}, Byte{5U}, Byte{6U}};
        REQUIRE_EQUAL(buffer.write(second), ByteLength{3U});

        auto result = std::array<Byte, 4>{};
        REQUIRE_EQUAL(buffer.read(result), ByteLength{4U});
        REQUIRE_EQUAL(
            std::vector<uint8_t>({result[0].toUInt8(), result[1].toUInt8(), result[2].toUInt8(), result[3].toUInt8()}),
            std::vector<uint8_t>({3U, 4U, 5U, 6U}));
    }

    void testGrowingWriteIsAtomic() {
        auto buffer = el::mem::RingBuffer{ByteLength{2U}, ByteLength{5U}};
        const auto first = std::array{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        REQUIRE(isSuccessful(buffer.writeExact(first)));
        REQUIRE_GREATER_EQUAL(buffer.capacity(), ByteLength{4U});

        const auto tooLarge = std::array{Byte{5U}, Byte{6U}};
        REQUIRE(isFailure(buffer.writeExact(tooLarge)));
        REQUIRE_EQUAL(buffer.length(), ByteLength{4U});
    }

    void testUnsafeNativeAccess() {
        auto buffer = el::mem::RingBuffer{ByteLength{4U}};
        {
            auto access = el::mem::impl::UnsafeRingBufferAccess{buffer};
            auto spans = access.writableSpans();
            spans[0][0] = Byte{0xaaU};
            spans[0][1] = Byte{0xbbU};
            access.commitWritten(ByteLength{2U});
            REQUIRE_THROWS(buffer.write(std::array{Byte{0xccU}}));
        }
        const auto result = buffer.read(ByteLength::infinite());
        REQUIRE_EQUAL(result.toUInt8Vector(), std::vector<uint8_t>({0xaaU, 0xbbU}));
    }

    void testEndianIntegerOperations() {
        auto buffer = el::mem::ByteRingBuffer{ByteLength{8U}};
        buffer.setEndianness(el::mem::Endianness::Big);
        REQUIRE(isSuccessful(buffer.writeInteger<uint32_t>(0x12345678U)));
        REQUIRE_EQUAL(buffer.readInteger<uint32_t>().value(), uint32_t{0x12345678U});
    }
};
