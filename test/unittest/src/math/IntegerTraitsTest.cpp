// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/IntegerConversion.hpp>
#include <erbsland/math/IntegerTraits.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <cstdint>
#include <type_traits>

using namespace el::math;

TESTED_TARGETS(IntegerTraits)
class IntegerTraitsTest final : public el::UnitTest {
public:
    void testWiderNativeIntegerSameSize() {
        // Same size: first wins (>= comparison)
        static_assert(std::same_as<WiderNativeInteger<std::int8_t, std::uint8_t>::type, std::int8_t>);
        static_assert(std::same_as<WiderNativeIntegerT<std::int8_t, std::uint8_t>, std::int8_t>);
    }

    void testWiderNativeIntegerFirstWider() {
        static_assert(std::same_as<WiderNativeInteger<std::int64_t, std::int8_t>::type, std::int64_t>);
        static_assert(std::same_as<WiderNativeIntegerT<std::int64_t, std::int8_t>, std::int64_t>);
    }

    void testWiderNativeIntegerSecondWider() {
        static_assert(std::same_as<WiderNativeInteger<std::int8_t, std::int64_t>::type, std::int64_t>);
        static_assert(std::same_as<WiderNativeIntegerT<std::int8_t, std::int64_t>, std::int64_t>);
    }

    void testCompatibleNativeIntegerSameSignedness() {
        // Same signedness: returns the wider type
        static_assert(std::same_as<CompatibleNativeInteger<std::int8_t, std::int16_t>::type, std::int16_t>);
        static_assert(std::same_as<CompatibleNativeIntegerT<std::int8_t, std::int16_t>, std::int16_t>);

        static_assert(std::same_as<CompatibleNativeInteger<std::uint32_t, std::uint64_t>::type, std::uint64_t>);
        static_assert(std::same_as<CompatibleNativeIntegerT<std::uint32_t, std::uint64_t>, std::uint64_t>);
    }

    void testCompatibleNativeIntegerSameSizeSameSignedness() {
        // Same size, same signedness: first wins
        static_assert(std::same_as<CompatibleNativeInteger<std::int8_t, std::int8_t>::type, std::int8_t>);
        static_assert(std::same_as<CompatibleNativeInteger<std::uint16_t, std::int16_t>::type, void>);
    }

    void testCompatibleNativeIntegerMixedSignedness() {
        // Mixed signedness: returns void
        static_assert(std::same_as<CompatibleNativeInteger<std::int8_t, std::uint8_t>::type, void>);
        static_assert(std::same_as<CompatibleNativeIntegerT<std::int8_t, std::uint8_t>, void>);
        static_assert(std::same_as<CompatibleNativeInteger<std::int16_t, std::uint32_t>::type, void>);
    }

    void testNativeIntegerOf() {
        // Native integer passes through
        static_assert(std::same_as<NativeIntegerOf<std::int32_t>::type, std::int32_t>);
        static_assert(std::same_as<NativeIntegerOfT<std::int32_t>, std::int32_t>);
    }

    void testToNativeInteger() {
        // Native integer returns as-is
        std::int32_t val{42};
        REQUIRE(toNativeInteger(val) == 42);

        std::int64_t cval{1234567};
        REQUIRE(toNativeInteger(cval) == 1234567);
    }

    void testSameSignednessNativeIntegers() {
        // Same signedness: true
        static_assert(SameSignednessNativeIntegers<std::int32_t, std::int64_t>);
        static_assert(SameSignednessNativeIntegers<std::uint32_t, std::uint64_t>);

        // Different signedness: false
        static_assert(!SameSignednessNativeIntegers<std::int32_t, std::uint64_t>);
        static_assert(!SameSignednessNativeIntegers<std::uint32_t, std::int64_t>);
    }

    void testSecondHasGreaterPositiveRange() {
        // Same size, signed vs unsigned: true
        static_assert(SecondHasGreaterPositiveRange<std::int8_t, std::uint8_t>);
        static_assert(SecondHasGreaterPositiveRange<std::int32_t, std::uint32_t>);

        // Second is wider: true
        static_assert(SecondHasGreaterPositiveRange<std::int8_t, std::int16_t>);
        static_assert(SecondHasGreaterPositiveRange<std::uint16_t, std::uint64_t>);

        // First is wider: false
        static_assert(!SecondHasGreaterPositiveRange<std::int64_t, std::int32_t>);
        static_assert(!SecondHasGreaterPositiveRange<std::uint64_t, std::uint32_t>);

        // Same type: false
        static_assert(!SecondHasGreaterPositiveRange<std::int32_t, std::int32_t>);
        static_assert(!SecondHasGreaterPositiveRange<std::uint32_t, std::uint32_t>);
    }

    void testSignCompatibleIntegerOperand() {
        // Same signedness: true
        static_assert(SignCompatibleIntegerOperand<std::int32_t, std::int64_t>);
        static_assert(SignCompatibleIntegerOperand<std::uint32_t, std::uint64_t>);

        // Different signedness: false
        static_assert(!SignCompatibleIntegerOperand<std::int32_t, std::uint64_t>);
        static_assert(!SignCompatibleIntegerOperand<std::uint32_t, std::int64_t>);
    }

    void testSignCompatibleIntegerOperandPair() {
        // All same signedness
        static_assert(SignCompatibleIntegerOperandPair<std::int32_t, std::int64_t, std::int16_t>);
        static_assert(SignCompatibleIntegerOperandPair<std::uint32_t, std::uint64_t, std::uint8_t>);

        // Mixed signedness
        static_assert(!SignCompatibleIntegerOperandPair<std::int32_t, std::int64_t, std::uint16_t>);
        static_assert(!SignCompatibleIntegerOperandPair<std::uint32_t, std::int64_t, std::uint8_t>);
    }
};
