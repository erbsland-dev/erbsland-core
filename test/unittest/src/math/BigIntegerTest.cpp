// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/math/BigInteger.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <limits>
#include <utility>

using el::err::OverflowError;
using el::err::ParseError;
using el::math::BigInteger;
using el::math::BigUnsignedInteger;
using namespace el::text::literals;

TESTED_TARGETS(BigInteger)
class BigIntegerTest final : public el::UnitTest {
public:
    void testConstructionAndComparison() {
        const auto minimum = BigInteger{std::numeric_limits<std::int64_t>::min()};
        const auto unsignedMaximum = BigInteger{std::numeric_limits<std::uint64_t>::max()};
        const auto largeMagnitude = BigUnsignedInteger::fromStringOrThrow("1000000000000000000000000000000"_el);
        const auto large = BigInteger{largeMagnitude};

        REQUIRE_EQUAL(minimum.toString(), "-9223372036854775808"_el);
        REQUIRE_EQUAL(unsignedMaximum.toString(), "18446744073709551615"_el);
        REQUIRE_EQUAL(large.magnitude(), largeMagnitude);
        REQUIRE_LESS(minimum, BigInteger{});
        REQUIRE_LESS(BigInteger{-2}, BigInteger{-1});
        REQUIRE_GREATER(large, unsignedMaximum);
        REQUIRE(BigInteger{1}.isOne());

        auto copied = large;
        auto moved = std::move(copied);
        REQUIRE_EQUAL(moved, large);

        const auto negativeZero = BigInteger::fromSignAndMagnitude(true, BigUnsignedInteger{});
        REQUIRE(negativeZero.isZero());
        REQUIRE_FALSE(negativeZero.isNegative());
    }

    void testAdditionAndSubtraction() {
        REQUIRE_EQUAL((BigInteger{-10} + BigInteger{3}), BigInteger{-7});
        REQUIRE_EQUAL((BigInteger{-10} + BigInteger{20}), BigInteger{10});
        REQUIRE_EQUAL((BigInteger{10} + BigInteger{-20}), BigInteger{-10});
        REQUIRE_EQUAL((BigInteger{-10} - BigInteger{-3}), BigInteger{-7});
        REQUIRE_EQUAL((BigInteger{10} - BigInteger{20}), BigInteger{-10});
        REQUIRE((BigInteger{-10} + BigInteger{10}).isZero());
    }

    void testNegationAndMultiplication() {
        REQUIRE_EQUAL(-BigInteger{12}, BigInteger{-12});
        REQUIRE_EQUAL(BigInteger{-12}.negated(), BigInteger{12});
        REQUIRE((-BigInteger{}).isZero());
        REQUIRE_EQUAL((BigInteger{-12} * BigInteger{-11}), BigInteger{132});
        REQUIRE_EQUAL((BigInteger{-12} * BigInteger{11}), BigInteger{-132});
        REQUIRE((BigInteger{-12} * BigInteger{}).isZero());
    }

    void testDivisionSigns() {
        requireDivision(BigInteger{123}, BigInteger{10}, BigInteger{12}, BigInteger{3});
        requireDivision(BigInteger{-123}, BigInteger{10}, BigInteger{-12}, BigInteger{-3});
        requireDivision(BigInteger{123}, BigInteger{-10}, BigInteger{-12}, BigInteger{3});
        requireDivision(BigInteger{-123}, BigInteger{-10}, BigInteger{12}, BigInteger{-3});
    }

    void testLargeDivision() {
        const auto dividend = BigInteger::fromStringOrThrow("-123456789012345678901234567890"_el);
        const auto divisor = BigInteger::fromStringOrThrow("9876543210987654321"_el);
        const auto quotient = dividend / divisor;
        const auto remainder = dividend % divisor;
        REQUIRE_EQUAL(quotient.toString(), "-12499999886"_el);
        REQUIRE_EQUAL(remainder.toString(), "-925925941327160484"_el);
        REQUIRE_EQUAL(quotient * divisor + remainder, dividend);

        auto combined = dividend;
        const auto combinedRemainder = combined.divideGetRemainder(divisor);
        REQUIRE_EQUAL(combined, quotient);
        REQUIRE_EQUAL(combinedRemainder, remainder);
    }

    void testCasts() {
        REQUIRE_EQUAL(BigInteger{-1}.cast<std::uint64_t>(), 0U);
        REQUIRE_THROWS_AS(OverflowError, BigInteger{-1}.castOrThrow<std::uint64_t>());
        REQUIRE_EQUAL(BigInteger{256}.cast<std::int8_t>(), std::numeric_limits<std::int8_t>::max());
        REQUIRE_EQUAL(BigInteger{-256}.cast<std::int8_t>(), std::numeric_limits<std::int8_t>::min());
        REQUIRE_THROWS_AS(OverflowError, BigInteger{256}.castOrThrow<std::int8_t>());
        REQUIRE_THROWS_AS(OverflowError, BigInteger{-129}.castOrThrow<std::int8_t>());
        REQUIRE_EQUAL(BigInteger{-128}.castOrThrow<std::int8_t>(), std::numeric_limits<std::int8_t>::min());
        REQUIRE_EQUAL(BigInteger{127}.castOrThrow<std::int8_t>(), std::numeric_limits<std::int8_t>::max());
    }

    void testTextConversion() {
        const auto negative = BigInteger::fromString("-000123456789012345678901234567890"_el);
        REQUIRE(negative.has_value());
        REQUIRE_EQUAL(negative->toString(), "-123456789012345678901234567890"_el);
        REQUIRE_EQUAL(BigInteger::fromStringOrThrow("+42"_el), BigInteger{42});
        REQUIRE(BigInteger::fromStringOrThrow("-0"_el).isZero());
        REQUIRE_FALSE(BigInteger::fromStringOrThrow("-0"_el).isNegative());

        REQUIRE_FALSE(BigInteger::fromString(""_el).has_value());
        REQUIRE_FALSE(BigInteger::fromString("-"_el).has_value());
        REQUIRE_FALSE(BigInteger::fromString("++1"_el).has_value());
        REQUIRE_FALSE(BigInteger::fromString("1 0"_el).has_value());
        REQUIRE_FALSE(BigInteger::fromString("0x10"_el).has_value());
        REQUIRE_THROWS_AS(ParseError, BigInteger::fromStringOrThrow("--1"_el));
    }

private:
    void requireDivision(
        const BigInteger &dividend,
        const BigInteger &divisor,
        const BigInteger &expectedQuotient,
        const BigInteger &expectedRemainder) {
        const auto quotient = dividend / divisor;
        const auto remainder = dividend % divisor;
        REQUIRE_EQUAL(quotient, expectedQuotient);
        REQUIRE_EQUAL(remainder, expectedRemainder);
        REQUIRE_EQUAL(quotient * divisor + remainder, dividend);
    }
};
