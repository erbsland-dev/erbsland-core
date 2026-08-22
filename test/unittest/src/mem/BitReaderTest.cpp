// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/BitReader.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

using el::mem::BitReader;
using el::mem::Byte;
using el::mem::ConstByteSpan;

static_assert(std::same_as<decltype(std::declval<BitReader &>().readInteger<std::uint8_t>()), std::uint8_t>);

TESTED_TARGETS(BitReader)
class BitReaderTest final : public el::UnitTest {
public:
    void testReadBitsMostSignificantFirst() {
        const auto data = std::array{Byte{0b10110010U}};
        auto reader = BitReader{ConstByteSpan{data}};

        REQUIRE_EQUAL(reader.bitCount(), 8U);
        REQUIRE_EQUAL(reader.bitPosition(), 0U);
        REQUIRE_EQUAL(reader.remainingBitCount(), 8U);
        REQUIRE(reader.canRead(8U));
        REQUIRE(reader.readBool());
        REQUIRE_FALSE(reader.readBool());
        REQUIRE_EQUAL(reader.readInteger<std::uint8_t>(), std::uint8_t{1U});
        REQUIRE_EQUAL(reader.readInteger<int>(), 1);
        REQUIRE_FALSE(reader.readBool());
        REQUIRE_FALSE(reader.readBool());
        REQUIRE(reader.readBool());
        REQUIRE_FALSE(reader.readBool());
        REQUIRE(reader.isAtEnd());
        REQUIRE_EQUAL(reader.remainingBitCount(), 0U);
    }

    void testPositionsClampToInput() {
        const auto data = std::array{Byte{0b10110010U}, Byte{0b01100001U}};
        auto reader = BitReader{ConstByteSpan{data}, 9U};

        REQUIRE(reader.readBool());
        REQUIRE(reader.readBool());
        REQUIRE_FALSE(reader.readBool());
        REQUIRE_EQUAL(reader.bitPosition(), 12U);
        reader.advance(2U);
        REQUIRE_EQUAL(reader.bitPosition(), 14U);
        REQUIRE(reader.canRead(2U));
        REQUIRE_FALSE(reader.canRead(3U));
        reader.setBitPosition(1U);
        REQUIRE_FALSE(reader.readBool());
        reader.setBitPosition(100U);
        REQUIRE_EQUAL(reader.bitPosition(), reader.bitCount());
        REQUIRE_FALSE(reader.readBool());
        REQUIRE_EQUAL(reader.bitPosition(), reader.bitCount());
    }

    void testEmptyReader() {
        auto reader = BitReader{};

        REQUIRE_EQUAL(reader.bitCount(), 0U);
        REQUIRE_EQUAL(reader.remainingBitCount(), 0U);
        REQUIRE(reader.isAtEnd());
        REQUIRE(reader.canRead(0U));
        REQUIRE_FALSE(reader.canRead(1U));
        REQUIRE_FALSE(reader.readBool());
        REQUIRE_EQUAL(reader.readInteger<std::uint64_t>(), std::uint64_t{0U});
    }
};
