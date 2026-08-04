// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/math/SignedMagnitude.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

using el::math::SatInt32;

TESTED_TARGETS(SignedMagnitude)
class SignedMagnitudeTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        using I32 = el::math::SignedMagnitude<std::int32_t>;
        using U32 = el::math::SignedMagnitude<std::uint32_t>;

        static_assert(std::same_as<I32::Value, std::int32_t>);
        static_assert(std::same_as<I32::Unsigned, std::uint32_t>);
        static_assert(noexcept(I32::fromValue(std::int32_t{})));
        static_assert(noexcept(I32::fromSignAndMagnitude(true, std::uint32_t{})));
        static_assert(I32::fromSignAndMagnitude(true, std::uint32_t{0}) == I32{});
        static_assert(I32::fromValue(std::int32_t{-5}).isNegative());
        static_assert(I32::fromValue(std::int32_t{-5}).magnitude() == std::uint32_t{5});
        static_assert(noexcept(I32{SatInt32{-5}}));
        static_assert(I32{SatInt32{-5}} == I32::fromValue(std::int32_t{-5}));
        static_assert(I32::fromValue(SatInt32{-5}).isNegative());
        static_assert(I32::fromValue(SatInt32{-5}).magnitude() == std::uint32_t{5});
        static_assert(
            I32::fromValue(std::numeric_limits<std::int32_t>::min()).magnitude() == (std::uint32_t{1} << 31U));
        static_assert(
            U32::fromValue(std::uint32_t{5})
                .negated()
                .toSaturatingValue(std::uint32_t{0}, std::numeric_limits<std::uint32_t>::max()) == std::uint32_t{0});
        static_assert(U32::fromValue(std::uint32_t{5})
                .negated()
                .wouldSaturate(std::uint32_t{0}, std::numeric_limits<std::uint32_t>::max()));
        static_assert(
            I32::fromValue(std::int32_t{10})
                .saturatingAddBounded(
                    I32::fromValue(std::int32_t{-3}),
                    std::numeric_limits<std::int32_t>::min(),
                    std::numeric_limits<std::int32_t>::max()) == std::int32_t{7});
        static_assert(
            I32::fromValue(std::int32_t{-7})
                .saturatingMultiplyBounded(
                    I32::fromValue(std::int32_t{6}),
                    std::numeric_limits<std::int32_t>::min(),
                    std::numeric_limits<std::int32_t>::max()) == std::int32_t{-42});
        static_assert(
            I32::fromValue(std::numeric_limits<std::int32_t>::min())
                .saturatingDivideBounded(
                    I32::fromValue(std::int32_t{-1}),
                    std::numeric_limits<std::int32_t>::min(),
                    std::numeric_limits<std::int32_t>::max()) == std::numeric_limits<std::int32_t>::max());
        static_assert(
            I32::fromValue(std::numeric_limits<std::int32_t>::min())
                .saturatingModuloBounded(
                    I32::fromValue(std::int32_t{-1}),
                    std::numeric_limits<std::int32_t>::min(),
                    std::numeric_limits<std::int32_t>::max()) == std::int32_t{0});
    }

    void testComparison() {
        using Value = el::math::SignedMagnitude<std::int8_t>;

        REQUIRE_EQUAL(Value::fromSignAndMagnitude(true, std::uint8_t{0}), Value{});
        REQUIRE_EQUAL(
            (Value::fromValue(std::int8_t{-2}) <=> Value::fromValue(std::int8_t{-1})), std::strong_ordering::less);
        REQUIRE_EQUAL(
            (Value::fromValue(std::int8_t{-1}) <=> Value::fromValue(std::int8_t{0})), std::strong_ordering::less);
        REQUIRE_EQUAL(
            (Value::fromValue(std::int8_t{1}) <=> Value::fromValue(std::int8_t{2})), std::strong_ordering::less);
        REQUIRE_LESS(Value::fromValue(std::int8_t{-1}), Value::fromValue(std::int8_t{1}));
        REQUIRE_GREATER(Value::fromValue(std::int8_t{2}), Value::fromValue(std::int8_t{-2}));
    }

    void testConversionAllTypes() {
        WITH_CONTEXT(requireConversionType<std::int8_t>());
        WITH_CONTEXT(requireConversionType<std::uint8_t>());
        WITH_CONTEXT(requireConversionType<std::int16_t>());
        WITH_CONTEXT(requireConversionType<std::uint16_t>());
        WITH_CONTEXT(requireConversionType<std::int32_t>());
        WITH_CONTEXT(requireConversionType<std::uint32_t>());
        WITH_CONTEXT(requireConversionType<std::int64_t>());
        WITH_CONTEXT(requireConversionType<std::uint64_t>());
    }

    void testNegationAllTypes() {
        WITH_CONTEXT(requireNegationType<std::int8_t>());
        WITH_CONTEXT(requireNegationType<std::uint8_t>());
        WITH_CONTEXT(requireNegationType<std::int16_t>());
        WITH_CONTEXT(requireNegationType<std::uint16_t>());
        WITH_CONTEXT(requireNegationType<std::int32_t>());
        WITH_CONTEXT(requireNegationType<std::uint32_t>());
        WITH_CONTEXT(requireNegationType<std::int64_t>());
        WITH_CONTEXT(requireNegationType<std::uint64_t>());
    }

    void testAdditionAllTypes() {
        WITH_CONTEXT(requireAdditionType<std::int8_t>());
        WITH_CONTEXT(requireAdditionType<std::uint8_t>());
        WITH_CONTEXT(requireAdditionType<std::int16_t>());
        WITH_CONTEXT(requireAdditionType<std::uint16_t>());
        WITH_CONTEXT(requireAdditionType<std::int32_t>());
        WITH_CONTEXT(requireAdditionType<std::uint32_t>());
        WITH_CONTEXT(requireAdditionType<std::int64_t>());
        WITH_CONTEXT(requireAdditionType<std::uint64_t>());
    }

    void testMultiplicationAllTypes() {
        WITH_CONTEXT(requireMultiplicationType<std::int8_t>());
        WITH_CONTEXT(requireMultiplicationType<std::uint8_t>());
        WITH_CONTEXT(requireMultiplicationType<std::int16_t>());
        WITH_CONTEXT(requireMultiplicationType<std::uint16_t>());
        WITH_CONTEXT(requireMultiplicationType<std::int32_t>());
        WITH_CONTEXT(requireMultiplicationType<std::uint32_t>());
        WITH_CONTEXT(requireMultiplicationType<std::int64_t>());
        WITH_CONTEXT(requireMultiplicationType<std::uint64_t>());
    }

    void testDivisionAllTypes() {
        WITH_CONTEXT(requireDivisionType<std::int8_t>());
        WITH_CONTEXT(requireDivisionType<std::uint8_t>());
        WITH_CONTEXT(requireDivisionType<std::int16_t>());
        WITH_CONTEXT(requireDivisionType<std::uint16_t>());
        WITH_CONTEXT(requireDivisionType<std::int32_t>());
        WITH_CONTEXT(requireDivisionType<std::uint32_t>());
        WITH_CONTEXT(requireDivisionType<std::int64_t>());
        WITH_CONTEXT(requireDivisionType<std::uint64_t>());
    }

    void testModuloAllTypes() {
        WITH_CONTEXT(requireModuloType<std::int8_t>());
        WITH_CONTEXT(requireModuloType<std::uint8_t>());
        WITH_CONTEXT(requireModuloType<std::int16_t>());
        WITH_CONTEXT(requireModuloType<std::uint16_t>());
        WITH_CONTEXT(requireModuloType<std::int32_t>());
        WITH_CONTEXT(requireModuloType<std::uint32_t>());
        WITH_CONTEXT(requireModuloType<std::int64_t>());
        WITH_CONTEXT(requireModuloType<std::uint64_t>());
    }

private:
    template <typename Result>
    void requireConversionType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;
        using Value = el::math::SignedMagnitude<Result>;
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        REQUIRE_EQUAL(Value::fromValue(Result{0}).toSaturatingValue(min, max), Result{0});
        REQUIRE_EQUAL(Value::fromValue(Result{1}).toSaturatingValue(min, max), Result{1});

        if constexpr (std::signed_integral<Result>) {
            REQUIRE(Value::fromValue(min).isNegative());
            REQUIRE_EQUAL(Value::fromValue(min).magnitude(), minimumAbsolute<Result>());
            REQUIRE_EQUAL(Value::fromValue(min).toSaturatingValue(min, max), min);
            REQUIRE_EQUAL(
                Value::fromValue(static_cast<Unsigned>(minimumAbsolute<Result>())).toSaturatingValue(min, max), max);
        } else {
            REQUIRE(Value::fromValue(Signed{-1}).isNegative());
            REQUIRE_EQUAL(Value::fromValue(Signed{-1}).toSaturatingValue(min, max), min);
            REQUIRE(Value::fromValue(Signed{-1}).wouldSaturate(min, max));
        }

        constexpr auto customMinimum = static_cast<Result>(min + Result{1});
        constexpr auto customMaximum = static_cast<Result>(max - Result{1});
        REQUIRE_EQUAL(Value::fromValue(min).toSaturatingValue(customMinimum, max), customMinimum);
        REQUIRE_EQUAL(Value::fromValue(max).toSaturatingValue(min, customMaximum), customMaximum);
    }

    template <typename Result>
    void requireNegationType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;
        using Value = el::math::SignedMagnitude<Result>;
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        REQUIRE_EQUAL(Value::fromValue(Result{0}).negated(), Value{});
        if constexpr (std::signed_integral<Result>) {
            REQUIRE_EQUAL(Value::fromValue(Result{-5}).negated().toSaturatingValue(min, max), Result{5});
            REQUIRE_EQUAL(Value::fromValue(min).negated().toSaturatingValue(min, max), max);
            REQUIRE(Value::fromValue(min).negated().wouldSaturate(min, max));
        } else {
            REQUIRE_EQUAL(Value::fromValue(Signed{-5}).negated().toSaturatingValue(min, max), Result{5});
            REQUIRE_EQUAL(Value::fromValue(Result{5}).negated().toSaturatingValue(min, max), min);
            REQUIRE(Value::fromValue(Result{5}).negated().wouldSaturate(min, max));
        }
        REQUIRE_EQUAL(Value::fromSignAndMagnitude(true, static_cast<Unsigned>(5)).negated().isNegative(), false);
    }

    template <typename Result>
    void requireAdditionType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;
        using Value = el::math::SignedMagnitude<Result>;
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireAdd<Result>(Result{5}, Result{6}, min, max, Result{11}, false));
        WITH_CONTEXT(requireAdd<Result>(Result{5}, Signed{-3}, min, max, Result{2}, false));
        WITH_CONTEXT(requireAdd<Result>(max, Result{1}, min, max, max, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireAdd<Result>(Result{5}, Signed{-8}, min, max, Result{-3}, false));
            WITH_CONTEXT(requireAdd<Result>(min, Result{-1}, min, max, min, true));
            WITH_CONTEXT(
                requireAdd<Result>(Result{0}, static_cast<Unsigned>(minimumAbsolute<Result>()), min, max, max, true));
        } else {
            WITH_CONTEXT(requireAdd<Result>(Result{5}, Signed{-8}, min, max, min, true));
            WITH_CONTEXT(requireAdd<Result>(Result{0}, Signed{-1}, min, max, min, true));
            WITH_CONTEXT(requireAdd<Result>(Result{0}, Signed{-5}, min, max, min, true));
        }

        REQUIRE_EQUAL(
            Value::fromSignAndMagnitude(false, std::numeric_limits<Unsigned>::max())
                .saturatingAddBounded(Value::fromSignAndMagnitude(false, Unsigned{1}), min, max),
            max);
        REQUIRE(
            Value::fromSignAndMagnitude(false, std::numeric_limits<Unsigned>::max())
                .wouldAddBoundedSaturate(Value::fromSignAndMagnitude(false, Unsigned{1}), min, max));
    }

    template <typename Result>
    void requireMultiplicationType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;
        using Value = el::math::SignedMagnitude<Result>;
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireMultiply<Result>(Result{5}, Result{6}, min, max, Result{30}, false));
        WITH_CONTEXT(requireMultiply<Result>(Result{0}, Result{7}, Result{1}, max, Result{1}, true));
        WITH_CONTEXT(requireMultiply<Result>(max, Result{2}, min, max, max, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireMultiply<Result>(Result{5}, Signed{-1}, min, max, Result{-5}, false));
            WITH_CONTEXT(requireMultiply<Result>(min, Result{-1}, min, max, max, true));
            WITH_CONTEXT(requireMultiply<Result>(min, Result{2}, min, max, min, true));
        } else {
            WITH_CONTEXT(requireMultiply<Result>(Result{5}, Signed{-1}, min, max, min, true));
            WITH_CONTEXT(requireMultiply<Result>(Signed{-5}, Result{2}, min, max, min, true));
        }

        REQUIRE_EQUAL(
            Value::fromSignAndMagnitude(false, std::numeric_limits<Unsigned>::max())
                .saturatingMultiplyBounded(Value::fromValue(Result{2}), min, max),
            max);
        REQUIRE(
            Value::fromSignAndMagnitude(false, std::numeric_limits<Unsigned>::max())
                .wouldMultiplyBoundedSaturate(Value::fromValue(Result{2}), min, max));
    }

    template <typename Result>
    void requireDivisionType() {
        using Signed = std::make_signed_t<Result>;
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireDivide<Result>(Result{30}, Result{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireDivide<Result>(Result{5}, Result{8}, min, max, Result{0}, false));
        WITH_CONTEXT(requireDivide<Result>(Result{0}, Result{2}, Result{1}, max, Result{1}, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireDivide<Result>(min, Result{-1}, min, max, max, true));
            WITH_CONTEXT(requireDivide<Result>(min, Result{2}, min, max, static_cast<Result>(min / Result{2}), false));
        } else {
            WITH_CONTEXT(requireDivide<Result>(Result{5}, Signed{-1}, min, max, min, true));
        }
    }

    template <typename Result>
    void requireModuloType() {
        using Signed = std::make_signed_t<Result>;
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireModulo<Result>(Result{32}, Result{5}, min, max, Result{2}, false));
        WITH_CONTEXT(requireModulo<Result>(Result{0}, Result{2}, Result{1}, max, Result{1}, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireModulo<Result>(Result{-32}, Result{5}, min, max, Result{-2}, false));
            WITH_CONTEXT(requireModulo<Result>(min, Result{-1}, min, max, Result{0}, false));
        } else {
            WITH_CONTEXT(requireModulo<Result>(Signed{-5}, Result{2}, min, max, min, true));
        }
    }

    template <typename Result, typename First, typename Second>
    void requireAdd(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        using Value = el::math::SignedMagnitude<Result>;
        const auto firstValue = Value::fromValue(first);
        const auto secondValue = Value::fromValue(second);
        REQUIRE_EQUAL(firstValue.saturatingAddBounded(secondValue, minimum, maximum), expected);
        REQUIRE_EQUAL(firstValue.wouldAddBoundedSaturate(secondValue, minimum, maximum), saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireMultiply(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        using Value = el::math::SignedMagnitude<Result>;
        const auto firstValue = Value::fromValue(first);
        const auto secondValue = Value::fromValue(second);
        REQUIRE_EQUAL(firstValue.saturatingMultiplyBounded(secondValue, minimum, maximum), expected);
        REQUIRE_EQUAL(firstValue.wouldMultiplyBoundedSaturate(secondValue, minimum, maximum), saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireDivide(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        using Value = el::math::SignedMagnitude<Result>;
        const auto firstValue = Value::fromValue(first);
        const auto secondValue = Value::fromValue(second);
        REQUIRE_EQUAL(firstValue.saturatingDivideBounded(secondValue, minimum, maximum), expected);
        REQUIRE_EQUAL(firstValue.wouldDivideBoundedSaturate(secondValue, minimum, maximum), saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireModulo(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        using Value = el::math::SignedMagnitude<Result>;
        const auto firstValue = Value::fromValue(first);
        const auto secondValue = Value::fromValue(second);
        REQUIRE_EQUAL(firstValue.saturatingModuloBounded(secondValue, minimum, maximum), expected);
        REQUIRE_EQUAL(firstValue.wouldModuloBoundedSaturate(secondValue, minimum, maximum), saturated);
    }

    template <std::signed_integral T>
    static constexpr auto minimumAbsolute() noexcept -> std::make_unsigned_t<T> {
        return std::make_unsigned_t<T>{1} << std::numeric_limits<T>::digits;
    }
};
