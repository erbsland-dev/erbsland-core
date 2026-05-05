// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

using el::err::OverflowError;
using namespace el::math;

template <typename T>
concept CanCreateSaturatingInteger = requires { typename SaturatingInteger<T>; };

TESTED_TARGETS(SaturatingInteger)
class SaturatingIntegerTest final : public el::UnitTest {
public:
    void testTypeConstraints() {
        static_assert(CanCreateSaturatingInteger<std::int8_t>);
        static_assert(CanCreateSaturatingInteger<std::uint64_t>);
        static_assert(!CanCreateSaturatingInteger<bool>);
        static_assert(!CanCreateSaturatingInteger<const int>);
        static_assert(!CanCreateSaturatingInteger<volatile int>);
        static_assert(!CanCreateSaturatingInteger<int &>);
    }
    void testSignedAliases() {
        WITH_CONTEXT(requireSignedType<SatInt8, std::int8_t>());
        WITH_CONTEXT(requireSignedType<SatInt16, std::int16_t>());
        WITH_CONTEXT(requireSignedType<SatInt32, std::int32_t>());
        WITH_CONTEXT(requireSignedType<SatInt64, std::int64_t>());
    }
    void testUnsignedAliases() {
        WITH_CONTEXT(requireUnsignedType<SatUInt8, std::uint8_t>());
        WITH_CONTEXT(requireUnsignedType<SatUInt16, std::uint16_t>());
        WITH_CONTEXT(requireUnsignedType<SatUInt32, std::uint32_t>());
        WITH_CONTEXT(requireUnsignedType<SatUInt64, std::uint64_t>());
    }
    void testOperatorResultTypes() {
        static_assert(std::same_as<decltype(SatInt8{100} + SatInt16{20}), SatInt16>);
        static_assert(std::same_as<decltype(SatInt8{100} - SatInt16{20}), SatInt16>);
        static_assert(std::same_as<decltype(SatInt8{100} * SatInt16{2}), SatInt16>);
        static_assert(std::same_as<decltype(SatInt8{100} / SatInt16{2}), SatInt16>);
        static_assert(std::same_as<decltype(SatInt8{100} % SatInt16{7}), SatInt16>);
        static_assert(std::same_as<decltype(SatUInt8{100} + SatUInt16{20}), SatUInt16>);
        static_assert(std::same_as<decltype(std::int16_t{100} + SatInt8{30}), SatInt16>);
        static_assert(std::same_as<decltype(std::uint16_t{250} + SatUInt8{10}), SatUInt16>);

        REQUIRE((SatInt8{100} + SatInt16{40}).toRawValue() == 140);
        REQUIRE((std::int16_t{100} + SatInt8{30}).toRawValue() == 130);
        REQUIRE((SatUInt8{250} + SatUInt16{20}).toRawValue() == 270);
        REQUIRE((std::uint16_t{250} + SatUInt8{10}).toRawValue() == 260);
    }
    void testMixedSignednessNamedOperations() {
        REQUIRE(SatInt8{-5}.added(SatUInt8{10}).toRawValue() == 5);
        REQUIRE(SatInt8{-120}.subtracted(SatUInt8{20}).toRawValue() == -128);
        REQUIRE(SatInt8{-20}.multiplied(SatUInt8{10}).toRawValue() == -128);
        REQUIRE(SatUInt8{10}.added(SatInt8{-5}).toRawValue() == 5);
        REQUIRE(SatUInt8{10}.subtracted(SatInt8{20}).toRawValue() == 0);
        REQUIRE(SatUInt8{10}.divided(SatInt8{-2}).toRawValue() == 0);
        REQUIRE(SatUInt8{10}.wouldDivideSaturate(SatInt8{-2}));
    }
    void testStaticConstruction() {
        REQUIRE(SatInt8::fromAddition(120, 20).toRawValue() == 127);
        REQUIRE(SatInt8::fromSubtraction(-120, 20).toRawValue() == -128);
        REQUIRE(SatInt8::fromMultiplication(20, 20).toRawValue() == 127);
        REQUIRE(SatInt8::fromDivision(-128, -1).toRawValue() == 127);
        REQUIRE(SatInt8::fromModulo(-128, -1).toRawValue() == 0);
        REQUIRE(SatUInt8::fromAddition(250U, 20U).toRawValue() == 255);
        REQUIRE(SatUInt8::fromSubtraction(10U, 20U).toRawValue() == 0);
        REQUIRE(SatUInt8::fromMultiplication(20U, 20U).toRawValue() == 255);
        REQUIRE(SatUInt8::fromDivision(255U, 2U).toRawValue() == 127);
        REQUIRE(SatUInt8::fromModulo(255U, 10U).toRawValue() == 5);

        const auto [signedQuotient, signedRemainder] = SatInt8::fromDivisionWithRemainder(127, 10);
        REQUIRE(signedQuotient.toRawValue() == 12);
        REQUIRE(signedRemainder.toRawValue() == 7);
        const auto [unsignedQuotient, unsignedRemainder] = SatUInt8::fromDivisionWithRemainder(255U, 10U);
        REQUIRE(unsignedQuotient.toRawValue() == 25);
        REQUIRE(unsignedRemainder.toRawValue() == 5);
    }
    void testRangeConversionAndWrapApi() {
        using Signed = SatInt8;
        using Unsigned = SatUInt8;

        static_assert(Signed::range().minimum() == std::numeric_limits<std::int8_t>::min());
        static_assert(Signed::range().maximum() == std::numeric_limits<std::int8_t>::max());
        REQUIRE(Signed{42}.template castOrThrow<std::int16_t>().toRawValue() == std::int16_t{42});
        REQUIRE_THROWS_AS(OverflowError, SatInt16{128}.template castOrThrow<std::int8_t>());

        auto value = Signed{12};
        value.clamp(IntegerRange<int>{-5, 5});
        REQUIRE(value.toRawValue() == 5);
        REQUIRE(Signed{-12}.clamped(IntegerRange<int>{-5, 5}).toRawValue() == -5);

        REQUIRE(Signed{12}.wrapped(0, 9).toRawValue() == 2);
        REQUIRE(Signed{-1}.wrapped(0, 9).toRawValue() == 9);
        REQUIRE(Signed{3}.wrapped(5, 3).toRawValue() == 0);
        REQUIRE(Signed{7}.wrapped(4, 4).toRawValue() == 4);
        REQUIRE(Signed{10}.wrapped(IntegerRange<int>{-200, -190}).toRawValue() == 0);

        value = Signed{12};
        value.wrap(0, 9);
        REQUIRE(value.toRawValue() == 2);

        const auto [positiveWrap, positiveCount] = Signed{25}.wrappedAndCount(0, 9);
        REQUIRE(positiveWrap.toRawValue() == 5);
        REQUIRE(positiveCount.toRawValue() == 2);

        const auto [negativeWrap, negativeCount] = Signed{-11}.wrappedAndCount(0, 9);
        REQUIRE(negativeWrap.toRawValue() == 9);
        REQUIRE(negativeCount.toRawValue() == -2);

        value = Signed{25};
        const auto count = value.wrapAndCount(0, 9);
        REQUIRE(value.toRawValue() == 5);
        REQUIRE(count.toRawValue() == 2);

        auto unsignedValue = Unsigned{4};
        const auto unsignedCount = unsignedValue.wrapAndCount(IntegerRange<std::uint8_t>{5U, 9U});
        REQUIRE(unsignedValue.toRawValue() == 9U);
        REQUIRE(unsignedCount.toRawValue() == -1);
    }

private:
    template <typename Sat, typename Native>
    void requireSignedType() {
        static_assert(std::same_as<typename Sat::NativeValue, Native>);
        static_assert(Sat::isSigned());
        static_assert(Sat::bitCount() == sizeof(Native) * 8);
        using Unsigned = std::make_unsigned_t<Native>;

        constexpr auto min = std::numeric_limits<Native>::min();
        constexpr auto max = std::numeric_limits<Native>::max();

        REQUIRE(Sat{}.toRawValue() == Native{0});
        REQUIRE(Sat::zero().isZero());
        REQUIRE(Sat::minimum().toRawValue() == min);
        REQUIRE(Sat::maximum().toRawValue() == max);
        REQUIRE(Sat::minimum().isMinimum());
        REQUIRE(Sat::maximum().isMaximum());
        REQUIRE(Sat{std::numeric_limits<std::uint64_t>::max()}.toRawValue() == max);
        REQUIRE(Sat{min}.toRawValue() == min);
        REQUIRE(Sat{Sat{Native{42}}}.toRawValue() == Native{42});

        WITH_CONTEXT(requireComparisonApi<Sat, Native>());
        WITH_CONTEXT(requireSignedArithmetic<Sat, Native>());
        WITH_CONTEXT(requireSignedConversion<Sat, Native, Unsigned>());
        WITH_CONTEXT(requireIncrementAndDecrement<Sat, Native>());
    }
    template <typename Sat, typename Native>
    void requireUnsignedType() {
        static_assert(std::same_as<typename Sat::NativeValue, Native>);
        static_assert(!Sat::isSigned());
        static_assert(Sat::bitCount() == sizeof(Native) * 8);
        constexpr auto max = std::numeric_limits<Native>::max();

        REQUIRE(Sat{}.toRawValue() == Native{0});
        REQUIRE(Sat::zero().isZero());
        REQUIRE(Sat::minimum().toRawValue() == Native{0});
        REQUIRE(Sat::maximum().toRawValue() == max);
        REQUIRE(Sat::minimum().isMinimum());
        REQUIRE(Sat::maximum().isMaximum());
        REQUIRE(Sat{-1}.toRawValue() == Native{0});
        REQUIRE(Sat{std::numeric_limits<std::uint64_t>::max()}.toRawValue() == max);
        REQUIRE(Sat{Sat{Native{42}}}.toRawValue() == Native{42});

        WITH_CONTEXT(requireComparisonApi<Sat, Native>());
        WITH_CONTEXT(requireUnsignedArithmetic<Sat, Native>());
        WITH_CONTEXT(requireUnsignedConversion<Sat, Native>());
        WITH_CONTEXT(requireIncrementAndDecrement<Sat, Native>());
    }
    template <typename Sat, typename Native>
    void requireComparisonApi() {
        const auto value = Sat{Native{10}};
        REQUIRE(value == Native{10});
        REQUIRE(value != Native{11});
        REQUIRE(value < Native{11});
        REQUIRE(value <= Native{10});
        REQUIRE(value > Native{9});
        REQUIRE(value >= Native{10});
        REQUIRE(Native{10} == value);
        REQUIRE(Native{11} != value);
        REQUIRE(Native{9} < value);
        REQUIRE(Native{10} <= value);
        REQUIRE(Native{11} > value);
        REQUIRE(Native{10} >= value);
        REQUIRE(value.compare(Native{10}) == std::strong_ordering::equal);
        REQUIRE(value.compare(Native{11}) == std::strong_ordering::less);
        REQUIRE(value.compare(Native{9}) == std::strong_ordering::greater);
    }
    template <typename Sat, typename Native>
    void requireSignedArithmetic() {
        constexpr auto min = std::numeric_limits<Native>::min();
        constexpr auto max = std::numeric_limits<Native>::max();

        REQUIRE((Sat{Native{20}} + Sat{Native{5}}).toRawValue() == Native{25});
        REQUIRE((Sat{max} + Sat{Native{1}}).toRawValue() == max);
        REQUIRE((Sat{Native{20}} - Sat{Native{5}}).toRawValue() == Native{15});
        REQUIRE((Sat{min} - Sat{Native{1}}).toRawValue() == min);
        REQUIRE((Sat{Native{20}} * Sat{Native{5}}).toRawValue() == Native{100});
        REQUIRE((Sat{max} * Sat{Native{2}}).toRawValue() == max);
        REQUIRE((Sat{Native{20}} / Sat{Native{5}}).toRawValue() == Native{4});
        REQUIRE((Sat{min} / Sat{Native{-1}}).toRawValue() == max);
        REQUIRE((Sat{Native{23}} % Sat{Native{5}}).toRawValue() == Native{3});
        REQUIRE((Sat{min} % Sat{Native{-1}}).toRawValue() == Native{0});

        auto value = Sat{Native{10}};
        value += Native{5};
        REQUIRE(value.toRawValue() == Native{15});
        value -= Native{20};
        REQUIRE(value.toRawValue() == Native{-5});
        value *= Native{-3};
        REQUIRE(value.toRawValue() == Native{15});
        value /= Native{2};
        REQUIRE(value.toRawValue() == Native{7});
        value %= Native{5};
        REQUIRE(value.toRawValue() == Native{2});

        value = Sat{Native{10}};
        value.add(Native{5});
        REQUIRE(value.toRawValue() == Native{15});
        value.subtract(Native{20});
        REQUIRE(value.toRawValue() == Native{-5});
        value.multiply(Native{-3});
        REQUIRE(value.toRawValue() == Native{15});
        value.divide(Native{2});
        REQUIRE(value.toRawValue() == Native{7});
        value.applyModulo(Native{5});
        REQUIRE(value.toRawValue() == Native{2});

        REQUIRE(Sat{max}.added(Native{1}).toRawValue() == max);
        REQUIRE(Sat{min}.subtracted(Native{1}).toRawValue() == min);
        REQUIRE(Sat{max}.multiplied(Native{2}).toRawValue() == max);
        REQUIRE(Sat{min}.divided(Native{-1}).toRawValue() == max);
        REQUIRE(Sat{Native{23}}.modulo(Native{5}).toRawValue() == Native{3});

        value = Sat{Native{23}};
        const auto remainder = value.divideGetRemainder(Native{5});
        REQUIRE(value.toRawValue() == Native{4});
        REQUIRE(remainder.toRawValue() == Native{3});
        value = Sat{Native{23}};
        const auto quotient = value.divideKeepRemainder(Native{5});
        REQUIRE(value.toRawValue() == Native{3});
        REQUIRE(quotient.toRawValue() == Native{4});

        REQUIRE(Sat{max}.wouldAddSaturate(Native{1}));
        REQUIRE(Sat{min}.wouldSubtractSaturate(Native{1}));
        REQUIRE(Sat{max}.wouldMultiplySaturate(Native{2}));
        REQUIRE(Sat{min}.wouldDivideSaturate(Native{-1}));
        REQUIRE(!Sat{min}.wouldModuloSaturate(Native{-1}));
    }
    template <typename Sat, typename Native>
    void requireUnsignedArithmetic() {
        constexpr auto max = std::numeric_limits<Native>::max();

        REQUIRE((Sat{Native{20}} + Sat{Native{5}}).toRawValue() == Native{25});
        REQUIRE((Sat{max} + Sat{Native{1}}).toRawValue() == max);
        REQUIRE((Sat{Native{20}} - Sat{Native{5}}).toRawValue() == Native{15});
        REQUIRE((Sat{Native{0}} - Sat{Native{1}}).toRawValue() == Native{0});
        REQUIRE((Sat{Native{20}} * Sat{Native{5}}).toRawValue() == Native{100});
        REQUIRE((Sat{max} * Sat{Native{2}}).toRawValue() == max);
        REQUIRE((Sat{Native{20}} / Sat{Native{5}}).toRawValue() == Native{4});
        REQUIRE((Sat{Native{23}} % Sat{Native{5}}).toRawValue() == Native{3});

        auto value = Sat{Native{10}};
        value += Native{5};
        REQUIRE(value.toRawValue() == Native{15});
        value -= Native{20};
        REQUIRE(value.toRawValue() == Native{0});
        value += Native{15};
        value *= Native{3};
        REQUIRE(value.toRawValue() == Native{45});
        value /= Native{2};
        REQUIRE(value.toRawValue() == Native{22});
        value %= Native{5};
        REQUIRE(value.toRawValue() == Native{2});

        value = Sat{Native{10}};
        value.add(Native{5});
        REQUIRE(value.toRawValue() == Native{15});
        value.subtract(Native{20});
        REQUIRE(value.toRawValue() == Native{0});
        value.add(Native{15});
        value.multiply(Native{3});
        REQUIRE(value.toRawValue() == Native{45});
        value.divide(Native{2});
        REQUIRE(value.toRawValue() == Native{22});
        value.applyModulo(Native{5});
        REQUIRE(value.toRawValue() == Native{2});

        REQUIRE(Sat{max}.added(Native{1}).toRawValue() == max);
        REQUIRE(Sat{Native{0}}.subtracted(Native{1}).toRawValue() == Native{0});
        REQUIRE(Sat{max}.multiplied(Native{2}).toRawValue() == max);
        REQUIRE(Sat{Native{23}}.divided(Native{5}).toRawValue() == Native{4});
        REQUIRE(Sat{Native{23}}.modulo(Native{5}).toRawValue() == Native{3});

        value = Sat{Native{23}};
        const auto remainder = value.divideGetRemainder(Native{5});
        REQUIRE(value.toRawValue() == Native{4});
        REQUIRE(remainder.toRawValue() == Native{3});
        value = Sat{Native{23}};
        const auto quotient = value.divideKeepRemainder(Native{5});
        REQUIRE(value.toRawValue() == Native{3});
        REQUIRE(quotient.toRawValue() == Native{4});

        REQUIRE(Sat{max}.wouldAddSaturate(Native{1}));
        REQUIRE(Sat{Native{0}}.wouldSubtractSaturate(Native{1}));
        REQUIRE(Sat{max}.wouldMultiplySaturate(Native{2}));
        REQUIRE(!Sat{max}.wouldDivideSaturate(Native{2}));
        REQUIRE(!Sat{max}.wouldModuloSaturate(Native{2}));
    }
    template <typename Sat, typename Native, typename Unsigned>
    void requireSignedConversion() {
        constexpr auto min = std::numeric_limits<Native>::min();
        constexpr auto max = std::numeric_limits<Native>::max();
        constexpr auto minimumAbsolute = static_cast<Unsigned>(Unsigned{1} << (Sat::bitCount() - 1U));

        REQUIRE(Sat{Native{-5}}.template cast<std::uint8_t>().toRawValue() == std::uint8_t{0});
        REQUIRE(Sat{Native{-5}}.toSizeT() == std::size_t{0});
        REQUIRE(Sat{Native{5}}.toSizeT() == std::size_t{5});
        REQUIRE(Sat{Native{-5}}.isNegative());
        REQUIRE(!Sat{Native{5}}.isNegative());
        REQUIRE(Sat{Native{1}}.isOne());
        REQUIRE(Sat{Native{-5}}.toAbsolute().toRawValue() == Native{5});
        REQUIRE(Sat{min}.toAbsolute().toRawValue() == max);
        REQUIRE(Sat{Native{-5}}.toUnsignedAbsolute().toRawValue() == Unsigned{5});
        REQUIRE(Sat{min}.toUnsignedAbsolute().toRawValue() == minimumAbsolute);
        REQUIRE(Sat{Native{-5}}.negated().toRawValue() == Native{5});
        REQUIRE(Sat{min}.negated().toRawValue() == max);
        REQUIRE((-Sat{Native{-5}}).toRawValue() == Native{5});

        auto value = Sat{Native{-5}};
        value.negate();
        REQUIRE(value.toRawValue() == Native{5});
        value = Sat{Native{-10}};
        value.clamp(Native{-5}, Native{5});
        REQUIRE(value.toRawValue() == Native{-5});
        REQUIRE(Sat{Native{10}}.clamped(Native{-5}, Native{5}).toRawValue() == Native{5});
        REQUIRE(Sat{Native{2}}.raisedTo(Native{3}).toRawValue() == Native{8});
        REQUIRE(Sat{Native{2}}.raisedTo(Native{-1}).toRawValue() == Native{0});
        REQUIRE(Sat{Native{-2}}.raisedTo(Native{3}).toRawValue() == Native{-8});
        REQUIRE(Sat{Native{-2}}.raisedTo(Native{2}).toRawValue() == Native{4});
        REQUIRE(Sat{Native{2}}.raisedTo(Native{Sat::bitCount()}).toRawValue() == max);
    }
    template <typename Sat, typename Native>
    void requireUnsignedConversion() {
        constexpr auto max = std::numeric_limits<Native>::max();

        REQUIRE(Sat{Native{5}}.template cast<std::uint8_t>().toRawValue() == std::uint8_t{5});
        REQUIRE(Sat{max}.template cast<std::uint8_t>().toRawValue() == std::numeric_limits<std::uint8_t>::max());
        REQUIRE(Sat{Native{5}}.toSizeT() == std::size_t{5});
        REQUIRE(!Sat{Native{5}}.isNegative());
        REQUIRE(Sat{Native{1}}.isOne());
        REQUIRE(Sat{Native{5}}.toAbsolute().toRawValue() == Native{5});
        REQUIRE(Sat{Native{5}}.toUnsignedAbsolute().toRawValue() == Native{5});
        REQUIRE(Sat{Native{5}}.negated().toRawValue() == Native{0});

        auto value = Sat{Native{5}};
        value.negate();
        REQUIRE(value.toRawValue() == Native{0});
        value = Sat{Native{2}};
        value.clamp(Native{5}, Native{10});
        REQUIRE(value.toRawValue() == Native{5});
        REQUIRE(Sat{Native{12}}.clamped(Native{5}, Native{10}).toRawValue() == Native{10});
        REQUIRE(Sat{Native{2}}.raisedTo(Native{3}).toRawValue() == Native{8});
        REQUIRE(Sat{Native{2}}.raisedTo(Native{Sat::bitCount()}).toRawValue() == max);
    }
    template <typename Sat, typename Native>
    void requireIncrementAndDecrement() {
        constexpr auto min = std::numeric_limits<Native>::min();
        constexpr auto max = std::numeric_limits<Native>::max();

        auto value = Sat{static_cast<Native>(max - Native{1})};
        const auto beforeIncrement = value++;
        REQUIRE(beforeIncrement.toRawValue() == static_cast<Native>(max - Native{1}));
        REQUIRE(value.toRawValue() == max);
        REQUIRE((++value).toRawValue() == max);

        value = Sat{static_cast<Native>(min + Native{1})};
        const auto beforeDecrement = value--;
        REQUIRE(beforeDecrement.toRawValue() == static_cast<Native>(min + Native{1}));
        REQUIRE(value.toRawValue() == min);
        REQUIRE((--value).toRawValue() == min);
    }
};
