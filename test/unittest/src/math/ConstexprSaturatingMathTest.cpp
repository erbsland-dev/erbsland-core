// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/ConstexprSaturatingMath.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

using namespace el::math;

TESTED_TARGETS(ConstexprSaturatingMath)
class ConstexprSaturatingMathTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(
            noexcept(saturatingAddBounded(std::uint32_t{}, std::int32_t{}, std::uint32_t{}, std::uint32_t{})));
        static_assert(std::same_as<
            decltype(saturatingAddBounded(std::uint32_t{}, std::int32_t{}, std::uint32_t{}, std::uint32_t{})),
            std::uint32_t>);
        static_assert(
            saturatingAddBounded(
                std::uint8_t{5}, std::int8_t{-8}, std::uint8_t{0}, std::numeric_limits<std::uint8_t>::max()) ==
            std::uint8_t{0});
        static_assert(willAddBoundedSaturate(
            std::uint8_t{5}, std::int8_t{-8}, std::uint8_t{0}, std::numeric_limits<std::uint8_t>::max()));
        static_assert(
            saturatingSubtractBounded(
                std::uint32_t{0},
                (std::uint32_t{1} << 31U),
                std::numeric_limits<std::int32_t>::min(),
                std::numeric_limits<std::int32_t>::max()) == std::numeric_limits<std::int32_t>::min());
        static_assert(!willSubtractBoundedSaturate(
            std::uint32_t{0},
            (std::uint32_t{1} << 31U),
            std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max()));
        static_assert(
            saturatingNegateBounded(
                std::numeric_limits<std::int64_t>::min(),
                std::numeric_limits<std::int64_t>::min(),
                std::numeric_limits<std::int64_t>::max()) == std::numeric_limits<std::int64_t>::max());
        static_assert(willNegateBoundedSaturate(
            std::numeric_limits<std::int64_t>::min(),
            std::numeric_limits<std::int64_t>::min(),
            std::numeric_limits<std::int64_t>::max()));
        static_assert(
            saturatingIncrementBounded(std::uint16_t{65534}, std::uint16_t{0}, std::uint16_t{65534}) ==
            std::uint16_t{65534});
        static_assert(
            saturatingDecrementBounded(std::int16_t{-32767}, std::int16_t{-32767}, std::int16_t{32767}) ==
            std::int16_t{-32767});
        static_assert(
            saturatingMultiplyBounded(std::uint8_t{20}, std::uint8_t{20}, std::uint8_t{0}, std::uint8_t{100}) ==
            std::uint8_t{100});
        static_assert(
            willMultiplyBoundedSaturate(std::uint8_t{20}, std::uint8_t{20}, std::uint8_t{0}, std::uint8_t{100}));
        static_assert(
            saturatingDivideBounded(
                std::numeric_limits<std::int8_t>::min(),
                std::int8_t{-1},
                std::numeric_limits<std::int8_t>::min(),
                std::numeric_limits<std::int8_t>::max()) == std::numeric_limits<std::int8_t>::max());
        static_assert(willDivideBoundedSaturate(
            std::numeric_limits<std::int8_t>::min(),
            std::int8_t{-1},
            std::numeric_limits<std::int8_t>::min(),
            std::numeric_limits<std::int8_t>::max()));
        static_assert(
            saturatingModuloBounded(
                std::numeric_limits<std::int8_t>::min(),
                std::int8_t{-1},
                std::numeric_limits<std::int8_t>::min(),
                std::numeric_limits<std::int8_t>::max()) == std::int8_t{0});
        static_assert(!willModuloBoundedSaturate(
            std::numeric_limits<std::int8_t>::min(),
            std::int8_t{-1},
            std::numeric_limits<std::int8_t>::min(),
            std::numeric_limits<std::int8_t>::max()));
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

    void testSubtractionAllTypes() {
        WITH_CONTEXT(requireSubtractionType<std::int8_t>());
        WITH_CONTEXT(requireSubtractionType<std::uint8_t>());
        WITH_CONTEXT(requireSubtractionType<std::int16_t>());
        WITH_CONTEXT(requireSubtractionType<std::uint16_t>());
        WITH_CONTEXT(requireSubtractionType<std::int32_t>());
        WITH_CONTEXT(requireSubtractionType<std::uint32_t>());
        WITH_CONTEXT(requireSubtractionType<std::int64_t>());
        WITH_CONTEXT(requireSubtractionType<std::uint64_t>());
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

    void testIncrementAndDecrementAllTypes() {
        WITH_CONTEXT(requireIncrementAndDecrementType<std::int8_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::uint8_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::int16_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::uint16_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::int32_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::uint32_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::int64_t>());
        WITH_CONTEXT(requireIncrementAndDecrementType<std::uint64_t>());
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
    void requireAdditionType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;

        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireAdd<Result>(Signed{5}, Signed{6}, min, max, Result{11}, false));
        WITH_CONTEXT(requireAdd<Result>(Signed{5}, Unsigned{6}, min, max, Result{11}, false));
        WITH_CONTEXT(requireAdd<Result>(Unsigned{5}, Signed{6}, min, max, Result{11}, false));
        WITH_CONTEXT(requireAdd<Result>(Unsigned{5}, Unsigned{6}, min, max, Result{11}, false));
        WITH_CONTEXT(requireAdd<Result>(max, Result{1}, min, max, max, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireAdd<Result>(min, Result{-1}, min, max, min, true));
            WITH_CONTEXT(requireAdd<Result>(Result{0}, signedMaximumPlusOne<Result>(), min, max, max, true));
        } else {
            WITH_CONTEXT(requireAdd<Result>(Result{5}, Signed{-3}, min, max, Result{2}, false));
            WITH_CONTEXT(requireAdd<Result>(Result{5}, Signed{-8}, min, max, min, true));
        }

        constexpr auto customMaximum = static_cast<Result>(max - Result{1});
        WITH_CONTEXT(requireAdd<Result>(customMaximum, Result{1}, min, customMaximum, customMaximum, true));
        WITH_CONTEXT(
            requireAdd<Result>(
                static_cast<Result>(customMaximum - Result{1}), Result{1}, min, customMaximum, customMaximum, false));
    }

    template <typename Result>
    void requireSubtractionType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;

        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireSubtract<Result>(Signed{11}, Signed{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireSubtract<Result>(Signed{11}, Unsigned{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireSubtract<Result>(Unsigned{11}, Signed{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireSubtract<Result>(Unsigned{11}, Unsigned{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireSubtract<Result>(max, Signed{-1}, min, max, max, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireSubtract<Result>(min, Result{1}, min, max, min, true));
            WITH_CONTEXT(requireSubtract<Result>(Result{0}, minimumAbsolute<Result>(), min, max, min, false));
            WITH_CONTEXT(
                requireSubtract<Result>(
                    Result{0}, static_cast<Unsigned>(minimumAbsolute<Result>() + Unsigned{1}), min, max, min, true));
        } else {
            WITH_CONTEXT(requireSubtract<Result>(Result{0}, Result{1}, min, max, min, true));
            WITH_CONTEXT(requireSubtract<Result>(Result{0}, Signed{-5}, min, max, Result{5}, false));
        }

        constexpr auto customMinimum = static_cast<Result>(min + Result{1});
        WITH_CONTEXT(requireSubtract<Result>(customMinimum, Result{1}, customMinimum, max, customMinimum, true));
        WITH_CONTEXT(
            requireSubtract<Result>(
                static_cast<Result>(customMinimum + Result{1}), Result{1}, customMinimum, max, customMinimum, false));
    }

    template <typename Result>
    void requireNegationType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;

        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireNegate<Result>(Result{0}, min, max, Result{0}, false));
        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireNegate<Result>(min, min, max, max, true));
            WITH_CONTEXT(requireNegate<Result>(max, min, max, static_cast<Result>(-max), false));
            constexpr auto customMaximum = static_cast<Result>(max - Result{1});
            WITH_CONTEXT(requireNegate<Result>(min, min, customMaximum, customMaximum, true));
        } else {
            WITH_CONTEXT(requireNegate<Result>(Result{1}, min, max, min, true));
            WITH_CONTEXT(requireNegate<Result>(Signed{-5}, min, max, Result{5}, false));
        }
        WITH_CONTEXT(requireNegate<Result>(minimumAbsolute<Signed>(), min, max, min, !std::signed_integral<Result>));
        WITH_CONTEXT(
            requireNegate<Result>(static_cast<Unsigned>(minimumAbsolute<Signed>() + Unsigned{1}), min, max, min, true));
    }

    template <typename Result>
    void requireIncrementAndDecrementType() {
        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireIncrement<Result>(max, min, max, max, true));
        WITH_CONTEXT(requireIncrement<Result>(static_cast<Result>(max - Result{1}), min, max, max, false));
        WITH_CONTEXT(requireDecrement<Result>(min, min, max, min, true));
        WITH_CONTEXT(requireDecrement<Result>(static_cast<Result>(min + Result{1}), min, max, min, false));

        constexpr auto customMaximum = static_cast<Result>(max - Result{1});
        constexpr auto customMinimum = static_cast<Result>(min + Result{1});
        WITH_CONTEXT(requireIncrement<Result>(customMaximum, min, customMaximum, customMaximum, true));
        WITH_CONTEXT(requireDecrement<Result>(customMinimum, customMinimum, max, customMinimum, true));
    }

    template <typename Result>
    void requireMultiplicationType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;

        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireMultiply<Result>(Signed{5}, Signed{6}, min, max, Result{30}, false));
        WITH_CONTEXT(requireMultiply<Result>(Signed{5}, Unsigned{6}, min, max, Result{30}, false));
        WITH_CONTEXT(requireMultiply<Result>(Unsigned{5}, Signed{6}, min, max, Result{30}, false));
        WITH_CONTEXT(requireMultiply<Result>(Unsigned{5}, Unsigned{6}, min, max, Result{30}, false));
        WITH_CONTEXT(requireMultiply<Result>(max, Result{2}, min, max, max, true));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireMultiply<Result>(min, Result{-1}, min, max, max, true));
            WITH_CONTEXT(requireMultiply<Result>(min, Result{2}, min, max, min, true));
        } else {
            WITH_CONTEXT(requireMultiply<Result>(Result{5}, Signed{-1}, min, max, min, true));
        }

        constexpr auto customMaximum = static_cast<Result>(max - Result{1});
        WITH_CONTEXT(requireMultiply<Result>(customMaximum, Result{2}, min, customMaximum, customMaximum, true));
        WITH_CONTEXT(requireMultiply<Result>(Result{0}, Result{2}, Result{1}, customMaximum, Result{1}, true));
    }

    template <typename Result>
    void requireDivisionType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;

        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireDivide<Result>(Signed{30}, Signed{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireDivide<Result>(Signed{30}, Unsigned{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireDivide<Result>(Unsigned{30}, Signed{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireDivide<Result>(Unsigned{30}, Unsigned{6}, min, max, Result{5}, false));
        WITH_CONTEXT(requireDivide<Result>(Result{5}, Result{8}, min, max, Result{0}, false));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireDivide<Result>(min, Result{-1}, min, max, max, true));
            WITH_CONTEXT(requireDivide<Result>(min, Result{2}, min, max, static_cast<Result>(min / Result{2}), false));
        } else {
            WITH_CONTEXT(requireDivide<Result>(Result{5}, Signed{-1}, min, max, min, true));
        }

        constexpr auto customMaximum = static_cast<Result>(max - Result{1});
        constexpr auto positiveMinimum = Result{1};
        WITH_CONTEXT(requireDivide<Result>(Result{0}, Result{2}, positiveMinimum, max, positiveMinimum, true));
        WITH_CONTEXT(requireDivide<Result>(max, Result{1}, min, customMaximum, customMaximum, true));
    }

    template <typename Result>
    void requireModuloType() {
        using Signed = std::make_signed_t<Result>;
        using Unsigned = std::make_unsigned_t<Result>;

        constexpr auto min = std::numeric_limits<Result>::min();
        constexpr auto max = std::numeric_limits<Result>::max();

        WITH_CONTEXT(requireModulo<Result>(Signed{32}, Signed{5}, min, max, Result{2}, false));
        WITH_CONTEXT(requireModulo<Result>(Signed{32}, Unsigned{5}, min, max, Result{2}, false));
        WITH_CONTEXT(requireModulo<Result>(Unsigned{32}, Signed{5}, min, max, Result{2}, false));
        WITH_CONTEXT(requireModulo<Result>(Unsigned{32}, Unsigned{5}, min, max, Result{2}, false));

        if constexpr (std::signed_integral<Result>) {
            WITH_CONTEXT(requireModulo<Result>(Result{-32}, Result{5}, min, max, Result{-2}, false));
            WITH_CONTEXT(requireModulo<Result>(min, Result{-1}, min, max, Result{0}, false));
        }

        constexpr auto positiveMinimum = Result{1};
        WITH_CONTEXT(requireModulo<Result>(Result{0}, Result{2}, positiveMinimum, max, positiveMinimum, true));
    }

    template <typename Result, typename First, typename Second>
    void requireAdd(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingAddBounded(first, second, minimum, maximum) == expected);
        REQUIRE(willAddBoundedSaturate(first, second, minimum, maximum) == saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireSubtract(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingSubtractBounded(first, second, minimum, maximum) == expected);
        REQUIRE(willSubtractBoundedSaturate(first, second, minimum, maximum) == saturated);
    }

    template <typename Result, typename Value>
    void requireNegate(Value value, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingNegateBounded(value, minimum, maximum) == expected);
        REQUIRE(willNegateBoundedSaturate(value, minimum, maximum) == saturated);
    }

    template <typename Result, typename Value>
    void requireIncrement(Value value, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingIncrementBounded(value, minimum, maximum) == expected);
        REQUIRE(willAddBoundedSaturate(value, Result{1}, minimum, maximum) == saturated);
    }

    template <typename Result, typename Value>
    void requireDecrement(Value value, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingDecrementBounded(value, minimum, maximum) == expected);
        REQUIRE(willSubtractBoundedSaturate(value, Result{1}, minimum, maximum) == saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireMultiply(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingMultiplyBounded(first, second, minimum, maximum) == expected);
        REQUIRE(willMultiplyBoundedSaturate(first, second, minimum, maximum) == saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireDivide(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingDivideBounded(first, second, minimum, maximum) == expected);
        REQUIRE(willDivideBoundedSaturate(first, second, minimum, maximum) == saturated);
    }

    template <typename Result, typename First, typename Second>
    void requireModulo(First first, Second second, Result minimum, Result maximum, Result expected, bool saturated) {
        REQUIRE(saturatingModuloBounded(first, second, minimum, maximum) == expected);
        REQUIRE(willModuloBoundedSaturate(first, second, minimum, maximum) == saturated);
    }

    template <std::signed_integral T>
    static constexpr auto minimumAbsolute() noexcept -> std::make_unsigned_t<T> {
        return std::make_unsigned_t<T>{1} << std::numeric_limits<T>::digits;
    }

    template <std::signed_integral T>
    static constexpr auto signedMaximumPlusOne() noexcept -> std::make_unsigned_t<T> {
        return minimumAbsolute<T>();
    }
};
