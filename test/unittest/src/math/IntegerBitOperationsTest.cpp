// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/IntegerBitOperations.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

using namespace el::math;

TESTED_TARGETS(IntegerBitOperations)
class IntegerBitOperationsTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(UnsignedNativeInteger<std::uint32_t>);
        static_assert(!UnsignedNativeInteger<std::int32_t>);
        static_assert(!UnsignedNativeInteger<bool>);
        static_assert(noexcept(rotateLeft(std::uint32_t{}, 1)));
        static_assert(noexcept(rotateRight(std::uint64_t{}, -1)));
        static_assert(std::same_as<decltype(rotateLeft(std::uint8_t{}, 1)), std::uint8_t>);
        static_assert(std::same_as<decltype(loadBigEndian<std::uint16_t>(std::array<std::byte, 2>{})), std::uint16_t>);
        static_assert(rotateLeft(std::uint8_t{0x81U}, 1) == std::uint8_t{0x03U});
        static_assert(rotateRight(std::uint16_t{0x0001U}, 1) == std::uint16_t{0x8000U});
        static_assert(rotateLeft(std::uint32_t{0x12345678U}, 8) == 0x34567812U);
        static_assert(noexcept(loadBigEndian<std::uint32_t>(std::array<std::byte, 4>{})));
        static_assert(noexcept(loadLittleEndian<std::uint64_t>(std::array<std::byte, 8>{})));

        constexpr auto bytes = std::array{
            std::byte{0x01},
            std::byte{0x23},
            std::byte{0x45},
            std::byte{0x67},
        };
        static_assert(loadBigEndian<std::uint32_t>(bytes) == 0x01234567U);
        static_assert(loadLittleEndian<std::uint32_t>(bytes) == 0x67452301U);
        static_assert([] {
            auto output = std::array<std::byte, 4>{};
            storeBigEndian(std::uint32_t{0x01234567U}, std::span<std::byte, 4>{output});
            return output;
        }() == bytes);

        auto bigOutput = std::array<std::byte, 4>{};
        storeBigEndian(std::uint32_t{0x01234567U}, std::span<std::byte, 4>{bigOutput});
        REQUIRE_EQUAL(bigOutput, bytes);

        auto littleOutput = std::array<std::byte, 4>{};
        storeLittleEndian(std::uint32_t{0x67452301U}, std::span<std::byte, 4>{littleOutput});
        REQUIRE_EQUAL(littleOutput, bytes);
    }

    void testRotationsAllWidths() {
        WITH_CONTEXT(requireRotations<std::uint8_t>());
        WITH_CONTEXT(requireRotations<std::uint16_t>());
        WITH_CONTEXT(requireRotations<std::uint32_t>());
        WITH_CONTEXT(requireRotations<std::uint64_t>());
    }

    void testEndianAllWidths() {
        WITH_CONTEXT(requireEndian<std::uint8_t>());
        WITH_CONTEXT(requireEndian<std::uint16_t>());
        WITH_CONTEXT(requireEndian<std::uint32_t>());
        WITH_CONTEXT(requireEndian<std::uint64_t>());
    }

    void testUnalignedStorage() {
        WITH_CONTEXT(requireUnaligned<std::uint8_t>());
        WITH_CONTEXT(requireUnaligned<std::uint16_t>());
        WITH_CONTEXT(requireUnaligned<std::uint32_t>());
        WITH_CONTEXT(requireUnaligned<std::uint64_t>());
    }

private:
    template <std::unsigned_integral T>
    void requireRotations() {
        constexpr auto width = std::numeric_limits<T>::digits;
        constexpr auto value = static_cast<T>(T{1} | (T{1} << (width - 1)));
        const auto amounts = std::array<int, 8>{0, 1, width - 1, width, width + 1, width * 3 + 1, -1, -width - 1};
        for (const auto amount : amounts) {
            REQUIRE_EQUAL(rotateLeft(value, amount), std::rotl(value, amount));
            REQUIRE_EQUAL(rotateRight(value, amount), std::rotr(value, amount));
            REQUIRE_EQUAL(rotateRight(rotateLeft(value, amount), amount), value);
        }
    }

    template <std::unsigned_integral T>
    void requireEndian() {
        constexpr auto values = std::array<T, 5>{
            T{0},
            T{1},
            std::numeric_limits<T>::max(),
            static_cast<T>(std::numeric_limits<T>::max() / T{3}),
            static_cast<T>(T{1} << (std::numeric_limits<T>::digits - 1)),
        };
        for (const auto value : values) {
            WITH_CONTEXT(requireEndianValue(value));
        }
        auto generated = static_cast<T>(0xa5U);
        for (std::size_t i = 0; i < 257U; ++i) {
            generated = static_cast<T>(generated * static_cast<T>(33U) + static_cast<T>(17U + i));
            WITH_CONTEXT(requireEndianValue(generated));
        }
    }

    template <std::unsigned_integral T>
    void requireEndianValue(const T value) {
        auto bigBytes = std::array<std::byte, sizeof(T)>{};
        auto littleBytes = std::array<std::byte, sizeof(T)>{};
        storeBigEndian(value, std::span<std::byte, sizeof(T)>{bigBytes});
        storeLittleEndian(value, std::span<std::byte, sizeof(T)>{littleBytes});
        REQUIRE_EQUAL(loadBigEndian<T>(bigBytes), value);
        REQUIRE_EQUAL(loadLittleEndian<T>(littleBytes), value);
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            REQUIRE_EQUAL(bigBytes[i], littleBytes[sizeof(T) - i - 1U]);
        }
    }

    template <std::unsigned_integral T>
    void requireUnaligned() {
        auto storage = std::array<std::byte, sizeof(T) + 2U>{};
        const auto value = static_cast<T>(0x0123456789abcdefULL);
        auto writable = std::span<std::byte, sizeof(T)>{storage.data() + 1, sizeof(T)};
        storeBigEndian(value, writable);
        const auto readable = std::span<const std::byte, sizeof(T)>{storage.data() + 1, sizeof(T)};
        REQUIRE_EQUAL(loadBigEndian<T>(readable), value);
    }
};
