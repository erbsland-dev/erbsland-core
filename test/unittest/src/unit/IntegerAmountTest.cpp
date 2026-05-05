// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/unit/all.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <cstdint>
#include <limits>
#include <ratio>
#include <type_traits>

using el::math::SatInt16;
using el::math::SatInt64;
using el::math::SatUInt64;

namespace erbsland::test::integeramounttest {

struct SecondsUnit {};
struct MetersUnit {};

using DoubleSeconds = el::unit::IntegerAmount<SecondsUnit, std::ratio<2>>;
using Hours = el::unit::IntegerAmount<SecondsUnit, std::ratio<3600>>;
using LargeMilliseconds = el::unit::IntegerAmount<SecondsUnit, std::milli, SatInt64>;
using Meters = el::unit::IntegerAmount<MetersUnit, std::ratio<1>>;
using Milliseconds = el::unit::IntegerAmount<SecondsUnit, std::milli>;
using Minutes = el::unit::IntegerAmount<SecondsUnit, std::ratio<60>>;
using Seconds = el::unit::IntegerAmount<SecondsUnit, std::ratio<1>>;
using SmallMilliseconds = el::unit::IntegerAmount<SecondsUnit, std::milli, SatInt16>;
using SmallSeconds = el::unit::IntegerAmount<SecondsUnit, std::ratio<1>, SatInt16>;
using TripleSeconds = el::unit::IntegerAmount<SecondsUnit, std::ratio<3>>;

template <typename tSource, typename tTarget>
concept CanConvertIntegerAmount = requires(tSource source) { source.template converted<tTarget>(); };

}

using namespace erbsland::test::integeramounttest;

TESTED_TARGETS(IntegerAmount)
class IntegerAmountTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(Seconds{}.isZero());
        static_assert(Seconds::one().isOne());
        static_assert(Seconds::minusOne().isMinusOne());
        static_assert(Seconds{-1}.isNegative());
        static_assert(Seconds{1}.isPositive());
        static_assert(Seconds::minimum().isMinimum());
        static_assert(Seconds::maximum().isMaximum());
        static_assert(Seconds::isBaseUnit());
        static_assert(!Milliseconds::isBaseUnit());
        static_assert(Milliseconds::ratioNumerator() == 1);
        static_assert(Milliseconds::ratioDenominator() == 1000);
        static_assert(std::same_as<typename Seconds::Unit, SecondsUnit>);
        static_assert(std::same_as<typename Seconds::Ratio, std::ratio<1>>);
        static_assert(std::same_as<typename Seconds::Value, SatInt64>);
        static_assert(std::same_as<typename Seconds::NativeValue, int64_t>);
        static_assert(std::same_as<typename SmallSeconds::Value, SatInt16>);
        static_assert(std::same_as<typename SmallSeconds::NativeValue, int16_t>);
        static_assert(el::unit::impl::ValidIntegerAmountRatio<std::ratio<1000>>);
        static_assert(el::unit::impl::ValidIntegerAmountRatio<std::milli>);
        static_assert(!el::unit::impl::ValidIntegerAmountRatio<std::ratio<2, 3>>);
        static_assert(el::unit::impl::ValidIntegerAmountValue<SatInt64>);
        static_assert(!el::unit::impl::ValidIntegerAmountValue<int64_t>);
        static_assert(!std::constructible_from<Seconds, uint64_t>);
        static_assert(!std::constructible_from<Seconds, SatUInt64>);
        static_assert(!std::equality_comparable_with<Seconds, int64_t>);
        static_assert(!std::equality_comparable_with<Seconds, Meters>);
        static_assert(!el::unit::impl::CompatibleIntegerAmount<Meters, typename Seconds::Unit>);
        static_assert(std::ratio_divide<typename Milliseconds::Ratio, typename Seconds::Ratio>::den != 1);
        static_assert(CanConvertIntegerAmount<Seconds, Milliseconds>);
        static_assert(!CanConvertIntegerAmount<DoubleSeconds, TripleSeconds>);
    }

    void testConstructionAndComparison() {
        REQUIRE_EQUAL(Seconds{}.toRawValue(), 0);
        REQUIRE_EQUAL(Seconds{42}.toRawValue(), 42);
        REQUIRE_EQUAL(Seconds{SatInt64{23}}.toRawValue(), 23);
        REQUIRE_EQUAL(Seconds{23}.toValue(), SatInt64{23});
        REQUIRE_EQUAL(SmallSeconds{int64_t{40000}}.toRawValue(), std::numeric_limits<int16_t>::max());
        REQUIRE_EQUAL(SmallSeconds{SatInt64{-40000}}.toRawValue(), std::numeric_limits<int16_t>::min());
        REQUIRE(Seconds{1} < Seconds{2});
        REQUIRE_EQUAL(Seconds{3}.compare(Seconds{3}), std::strong_ordering::equal);
    }

    void testSaturatingArithmetic() {
        REQUIRE_EQUAL(Seconds{2} + Seconds{3}, Seconds{5});
        REQUIRE_EQUAL(Seconds{2} - Seconds{5}, Seconds{-3});
        REQUIRE(Seconds::maximum() + Seconds{1} == Seconds::maximum());
        REQUIRE(Seconds::minimum() - Seconds{1} == Seconds::minimum());
        REQUIRE(-Seconds::minimum() == Seconds::maximum());

        auto value = Seconds{1};
        REQUIRE_EQUAL((value++), Seconds{1});
        REQUIRE_EQUAL(value, Seconds{2});
        REQUIRE_EQUAL((--value), Seconds{1});

        value += Seconds{5};
        REQUIRE_EQUAL(value, Seconds{6});
        value -= Seconds{8};
        REQUIRE_EQUAL(value, Seconds{-2});
    }

    void testFactorsNegationAndClamping() {
        REQUIRE_EQUAL(Seconds{6} * int64_t{-3}, Seconds{-18});
        REQUIRE_EQUAL(int64_t{4} * Seconds{-5}, Seconds{-20});
        REQUIRE_EQUAL(Seconds{7} / int64_t{2}, Seconds{3});
        REQUIRE_EQUAL(Seconds{-7} / int64_t{2}, Seconds{-3});
        REQUIRE(Seconds::maximum() * int64_t{2} == Seconds::maximum());
        REQUIRE(Seconds::minimum() / int64_t{-1} == Seconds::maximum());

        auto value = Seconds{-12};
        value.negate();
        REQUIRE_EQUAL(value, Seconds{12});
        REQUIRE_EQUAL(Seconds{12}.clamped(Seconds{-4}, Seconds{8}), Seconds{8});
        value.clamp(Seconds{-4}, Seconds{8});
        REQUIRE_EQUAL(value, Seconds{8});
    }

    void testRatioConversions() {
        REQUIRE_EQUAL(Milliseconds{1500}.converted<Seconds>(), Seconds{1});
        REQUIRE_EQUAL(Milliseconds{-1500}.converted<Seconds>(), Seconds{-1});
        REQUIRE_EQUAL(Seconds{2}.converted<Milliseconds>(), Milliseconds{2000});
        REQUIRE_EQUAL(Hours{1}.toBaseUnit(), Seconds{3600});
        REQUIRE_EQUAL(Hours{1}.toBaseUnitOrThrow(), Seconds{3600});
        REQUIRE_EQUAL(Milliseconds{1500}.convert<Seconds>(), Seconds{1});
        REQUIRE_EQUAL(Milliseconds{1500}.convertOrThrow<Seconds>(), Seconds{1});
        REQUIRE_FALSE(Milliseconds{1500}.wouldConvertSaturate<Seconds>());
        REQUIRE_FALSE(Seconds{2}.wouldConvertSaturate<Milliseconds>());
        REQUIRE((Seconds::maximum() / int64_t{2}).converted<Milliseconds>().isMaximum());
        REQUIRE((Seconds::maximum() / int64_t{2}).wouldConvertSaturate<Milliseconds>());
        REQUIRE_THROWS((Seconds::maximum() / int64_t{2}).convertedOrThrow<Milliseconds>());
        REQUIRE_EQUAL(SmallSeconds{33}.converted<LargeMilliseconds>(), LargeMilliseconds{33'000});
        REQUIRE_FALSE(SmallSeconds{33}.wouldConvertSaturate<LargeMilliseconds>());
        REQUIRE(SmallSeconds{33}.converted<SmallMilliseconds>().isMaximum());
        REQUIRE(SmallSeconds{33}.wouldConvertSaturate<SmallMilliseconds>());
        REQUIRE_THROWS(SmallSeconds{33}.convertedOrThrow<SmallMilliseconds>());
    }

    void testExtractLargerUnits() {
        auto positive = Milliseconds{3'723'004};
        const auto positiveHours = positive.extract<Hours>();
        REQUIRE_EQUAL(positiveHours, Hours{1});
        REQUIRE_EQUAL(positive, Milliseconds{123'004});
        const auto positiveSeconds = positive.extract<Seconds>();
        REQUIRE_EQUAL(positiveSeconds, Seconds{123});
        REQUIRE_EQUAL(positive, Milliseconds{4});

        auto negative = Milliseconds{-3'723'004};
        const auto negativeHours = negative.extract<Hours>();
        REQUIRE_EQUAL(negativeHours, Hours{-1});
        REQUIRE_EQUAL(negative, Milliseconds{-123'004});
        const auto negativeSeconds = negative.extract<Seconds>();
        REQUIRE_EQUAL(negativeSeconds, Seconds{-123});
        REQUIRE_EQUAL(negative, Milliseconds{-4});
    }
};
