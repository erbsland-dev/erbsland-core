// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/math/BigUnsignedInteger.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <utility>

using el::err::OverflowError;
using el::err::ParseError;
using el::math::BigUnsignedInteger;
using namespace el::text::literals;

TESTED_TARGETS(BigUnsignedInteger)
class BigUnsignedIntegerTest final : public el::UnitTest {
public:
    void testConstructionAndComparison() {
        const auto zero = BigUnsignedInteger{};
        const auto one = BigUnsignedInteger{1U};
        const auto maximum = BigUnsignedInteger{std::numeric_limits<std::uint64_t>::max()};

        REQUIRE(zero.isZero());
        REQUIRE_FALSE(zero.isOne());
        REQUIRE(one.isOne());
        REQUIRE_LESS(zero, one);
        REQUIRE_GREATER(maximum, one);
        REQUIRE_EQUAL(maximum.toString(), "18446744073709551615"_el);
        REQUIRE_THROWS_AS(OverflowError, BigUnsignedInteger{-1});

        auto copied = maximum;
        auto moved = std::move(copied);
        REQUIRE_EQUAL(moved, maximum);
    }

    void testAdditionAndSubtraction() {
        auto value = BigUnsignedInteger::fromStringOrThrow("999999999999999999999999999"_el);
        value += BigUnsignedInteger{2U};
        REQUIRE_EQUAL(value.toString(), "1000000000000000000000000001"_el);
        REQUIRE_EQUAL((value - BigUnsignedInteger{2U}).toString(), "999999999999999999999999999"_el);

        const auto unchanged = BigUnsignedInteger{5U};
        auto underflow = unchanged;
        REQUIRE_THROWS_AS(OverflowError, underflow -= BigUnsignedInteger{6U});
        REQUIRE_EQUAL(underflow, unchanged);
        REQUIRE((value - value).isZero());
    }

    void testMultiplication() {
        const auto first = BigUnsignedInteger::fromStringOrThrow("999999999999999999"_el);
        const auto second = BigUnsignedInteger::fromStringOrThrow("1000000001"_el);
        REQUIRE_EQUAL((first * second).toString(), "1000000000999999998999999999"_el);

        auto self = BigUnsignedInteger::fromStringOrThrow("12345678901234567890"_el);
        self *= self;
        REQUIRE_EQUAL(self.toString(), "152415787532388367501905199875019052100"_el);
        REQUIRE((self * BigUnsignedInteger{}).isZero());
    }

    void testDivision() {
        const auto dividend = BigUnsignedInteger::fromStringOrThrow("123456789012345678901234567890"_el);
        const auto divisor = BigUnsignedInteger::fromStringOrThrow("9876543210987654321"_el);
        const auto quotient = dividend / divisor;
        const auto remainder = dividend % divisor;
        REQUIRE_EQUAL(quotient.toString(), "12499999886"_el);
        REQUIRE_EQUAL(remainder.toString(), "925925941327160484"_el);
        REQUIRE_EQUAL(quotient * divisor + remainder, dividend);
        REQUIRE_LESS(remainder, divisor);

        auto combined = dividend;
        const auto combinedRemainder = combined.divideGetRemainder(divisor);
        REQUIRE_EQUAL(combined, quotient);
        REQUIRE_EQUAL(combinedRemainder, remainder);

        REQUIRE_EQUAL((BigUnsignedInteger{5U} / BigUnsignedInteger{7U}), BigUnsignedInteger{});
        REQUIRE_EQUAL((BigUnsignedInteger{5U} % BigUnsignedInteger{7U}), BigUnsignedInteger{5U});
        REQUIRE_EQUAL((dividend / dividend), BigUnsignedInteger{1U});
        REQUIRE((dividend % dividend).isZero());

        const auto secondDividend = BigUnsignedInteger::fromStringOrThrow(
            "1000000000000000000000000000000000000000000000000000000000123456789"_el);
        const auto secondDivisor = BigUnsignedInteger::fromStringOrThrow("100000000000000000000000098765"_el);
        REQUIRE_EQUAL((secondDividend / secondDivisor).toString(), "9999999999999999999999990123500000000"_el);
        REQUIRE_EQUAL((secondDividend % secondDivisor).toString(), "975452522623456789"_el);
    }

    void testNativeRangeDivision() {
        auto state = std::uint64_t{0x4d595df4d0f33173ULL};
        for (auto index = 0; index < 500; ++index) {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            const auto dividend = state;
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            const auto divisor = state | 1ULL;
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto bigDividend = BigUnsignedInteger{dividend};
                    const auto bigDivisor = BigUnsignedInteger{divisor};
                    REQUIRE_EQUAL((bigDividend / bigDivisor).castOrThrow<std::uint64_t>(), dividend / divisor);
                    REQUIRE_EQUAL((bigDividend % bigDivisor).castOrThrow<std::uint64_t>(), dividend % divisor);
                },
                [&]() -> std::string {
                    return std::format("division case {}: dividend={} divisor={}", index, dividend, divisor);
                });
        }
    }

    void testCasts() {
        const auto value = BigUnsignedInteger{256U};
        REQUIRE_EQUAL(value.cast<std::uint8_t>(), std::numeric_limits<std::uint8_t>::max());
        REQUIRE_EQUAL(value.cast<std::int16_t>(), 256);
        REQUIRE_THROWS_AS(OverflowError, value.castOrThrow<std::uint8_t>());
        REQUIRE_EQUAL(value.castOrThrow<std::uint16_t>(), 256U);

        const auto huge = BigUnsignedInteger::fromStringOrThrow("18446744073709551616"_el);
        REQUIRE_EQUAL(huge.cast<std::uint64_t>(), std::numeric_limits<std::uint64_t>::max());
        REQUIRE_THROWS_AS(OverflowError, huge.castOrThrow<std::uint64_t>());
    }

    void testTextConversion() {
        const auto value = BigUnsignedInteger::fromString("+000123456789012345678901234567890"_el);
        REQUIRE(value.has_value());
        REQUIRE_EQUAL(value->toString(), "123456789012345678901234567890"_el);
        REQUIRE(BigUnsignedInteger::fromString("0"_el)->isZero());

        REQUIRE_FALSE(BigUnsignedInteger::fromString(""_el).has_value());
        REQUIRE_FALSE(BigUnsignedInteger::fromString("+"_el).has_value());
        REQUIRE_FALSE(BigUnsignedInteger::fromString("-1"_el).has_value());
        REQUIRE_FALSE(BigUnsignedInteger::fromString("1'000"_el).has_value());
        REQUIRE_FALSE(BigUnsignedInteger::fromString(" 1"_el).has_value());
        REQUIRE_FALSE(BigUnsignedInteger::fromString("0x10"_el).has_value());
        REQUIRE_THROWS_AS(ParseError, BigUnsignedInteger::fromStringOrThrow("12x"_el));
    }
};
