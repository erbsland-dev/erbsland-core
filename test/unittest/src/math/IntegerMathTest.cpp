// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/IntegerMath.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <compare>
#include <concepts>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using el::math::mixedIntegerCompare;
using el::math::orderMinimumMaximum;
using el::math::toIntegerNormal;
using el::math::toUnsignedAbsolute;

TESTED_TARGETS(IntegerMath)
class IntegerMathTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(noexcept(mixedIntegerCompare(std::int8_t{}, std::uint64_t{})));
        static_assert(mixedIntegerCompare(std::int8_t{-1}, std::uint8_t{0}) == std::strong_ordering::less);
        static_assert(mixedIntegerCompare(std::uint8_t{0}, std::int8_t{-1}) == std::strong_ordering::greater);
        static_assert(mixedIntegerCompare(std::uint8_t{42}, std::int16_t{42}) == std::strong_ordering::equal);
        static_assert(
            mixedIntegerCompare(std::uint64_t{std::numeric_limits<std::uint64_t>::max()}, std::int64_t{-1}) ==
            std::strong_ordering::greater);

        static_assert(noexcept(toUnsignedAbsolute(std::int8_t{})));
        static_assert(std::same_as<decltype(toUnsignedAbsolute(std::int8_t{})), std::uint8_t>);
        static_assert(toUnsignedAbsolute(std::numeric_limits<std::int8_t>::min()) == std::uint8_t{128});
        static_assert(toUnsignedAbsolute(std::uint16_t{65535}) == std::uint16_t{65535});

        static_assert(noexcept(toIntegerNormal(std::int16_t{})));
        static_assert(std::same_as<decltype(toIntegerNormal(std::uint32_t{})), std::uint32_t>);
        static_assert(toIntegerNormal(std::int16_t{-4}) == std::int16_t{-1});
        static_assert(toIntegerNormal(std::uint16_t{4}) == std::uint16_t{1});

        auto orderedMinimum = 2;
        auto orderedMaximum = 1;
        orderMinimumMaximum(orderedMinimum, orderedMaximum);
        REQUIRE_EQUAL(orderedMinimum, 1);
        REQUIRE_EQUAL(orderedMaximum, 2);
    }

    void testMixedIntegerCompareMatchesStandardOrder() {
        WITH_CONTEXT(requireCompareFirst<std::int8_t>());
        WITH_CONTEXT(requireCompareFirst<std::uint8_t>());
        WITH_CONTEXT(requireCompareFirst<std::int16_t>());
        WITH_CONTEXT(requireCompareFirst<std::uint16_t>());
        WITH_CONTEXT(requireCompareFirst<std::int32_t>());
        WITH_CONTEXT(requireCompareFirst<std::uint32_t>());
        WITH_CONTEXT(requireCompareFirst<std::int64_t>());
        WITH_CONTEXT(requireCompareFirst<std::uint64_t>());
    }

    void testToUnsignedAbsoluteAllTypes() {
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::int8_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::uint8_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::int16_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::uint16_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::int32_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::uint32_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::int64_t>());
        WITH_CONTEXT(requireToUnsignedAbsoluteType<std::uint64_t>());
    }

    void testToIntegerNormalAllTypes() {
        WITH_CONTEXT(requireToIntegerNormalType<std::int8_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::uint8_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::int16_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::uint16_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::int32_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::uint32_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::int64_t>());
        WITH_CONTEXT(requireToIntegerNormalType<std::uint64_t>());
    }

private:
    template <std::integral First>
    void requireCompareFirst() {
        WITH_CONTEXT(requireComparePair<First, std::int8_t>());
        WITH_CONTEXT(requireComparePair<First, std::uint8_t>());
        WITH_CONTEXT(requireComparePair<First, std::int16_t>());
        WITH_CONTEXT(requireComparePair<First, std::uint16_t>());
        WITH_CONTEXT(requireComparePair<First, std::int32_t>());
        WITH_CONTEXT(requireComparePair<First, std::uint32_t>());
        WITH_CONTEXT(requireComparePair<First, std::int64_t>());
        WITH_CONTEXT(requireComparePair<First, std::uint64_t>());
    }

    template <std::integral First, std::integral Second>
    void requireComparePair() {
        WITH_CONTEXT(requireCompare(First{0}, Second{0}));
        WITH_CONTEXT(requireCompare(First{1}, Second{0}));
        WITH_CONTEXT(requireCompare(First{0}, Second{1}));
        WITH_CONTEXT(requireCompare(std::numeric_limits<First>::min(), std::numeric_limits<Second>::min()));
        WITH_CONTEXT(requireCompare(std::numeric_limits<First>::min(), std::numeric_limits<Second>::max()));
        WITH_CONTEXT(requireCompare(std::numeric_limits<First>::max(), std::numeric_limits<Second>::min()));
        WITH_CONTEXT(requireCompare(std::numeric_limits<First>::max(), std::numeric_limits<Second>::max()));

        if constexpr (std::is_signed_v<First>) {
            WITH_CONTEXT(requireCompare(First{-1}, Second{0}));
            WITH_CONTEXT(requireCompare(First{-1}, std::numeric_limits<Second>::max()));
        }
        if constexpr (std::is_signed_v<Second>) {
            WITH_CONTEXT(requireCompare(First{0}, Second{-1}));
            WITH_CONTEXT(requireCompare(std::numeric_limits<First>::max(), Second{-1}));
        }
    }

    template <std::integral First, std::integral Second>
    void requireCompare(First first, Second second) {
        const auto expected = expectedCompare(first, second);
        const auto actual = mixedIntegerCompare(first, second);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_EQUAL(actual, expected); },
            [&]() -> std::string { return createCompareDiagnostic(first, second, actual, expected); });
    }

    template <std::integral First, std::integral Second>
    static constexpr auto expectedCompare(First first, Second second) noexcept -> std::strong_ordering {
        if (std::cmp_less(first, second)) {
            return std::strong_ordering::less;
        }
        if (std::cmp_less(second, first)) {
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    template <std::integral T>
    void requireToUnsignedAbsoluteType() {
        using Unsigned = std::make_unsigned_t<T>;
        static_assert(std::same_as<decltype(toUnsignedAbsolute(T{})), Unsigned>);
        static_assert(noexcept(toUnsignedAbsolute(T{})));

        WITH_CONTEXT(requireToUnsignedAbsolute(T{0}, Unsigned{0}));
        WITH_CONTEXT(requireToUnsignedAbsolute(
            std::numeric_limits<T>::max(), static_cast<Unsigned>(std::numeric_limits<T>::max())));
        if constexpr (std::is_signed_v<T>) {
            WITH_CONTEXT(requireToUnsignedAbsolute(T{-1}, Unsigned{1}));
            WITH_CONTEXT(requireToUnsignedAbsolute(std::numeric_limits<T>::min(), minimumAbsolute<T>()));
        }
    }

    template <std::integral T>
    void requireToUnsignedAbsolute(T value, std::make_unsigned_t<T> expected) {
        const auto actual = toUnsignedAbsolute(value);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_EQUAL(actual, expected); },
            [&]() -> std::string { return createToUnsignedAbsoluteDiagnostic(value, actual, expected); });
    }

    template <std::signed_integral T>
    static constexpr auto minimumAbsolute() -> std::make_unsigned_t<T> {
        return std::make_unsigned_t<T>{1} << std::numeric_limits<T>::digits;
    }

    template <std::integral T>
    void requireToIntegerNormalType() {
        static_assert(std::same_as<decltype(toIntegerNormal(T{})), T>);
        static_assert(noexcept(toIntegerNormal(T{})));

        WITH_CONTEXT(requireToIntegerNormal(T{0}, T{0}));
        WITH_CONTEXT(requireToIntegerNormal(T{1}, T{1}));
        WITH_CONTEXT(requireToIntegerNormal(std::numeric_limits<T>::max(), T{1}));
        if constexpr (std::is_signed_v<T>) {
            WITH_CONTEXT(requireToIntegerNormal(T{-1}, T{-1}));
            WITH_CONTEXT(requireToIntegerNormal(std::numeric_limits<T>::min(), T{-1}));
        }
    }

    template <std::integral T>
    void requireToIntegerNormal(T value, T expected) {
        const auto actual = toIntegerNormal(value);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_EQUAL(actual, expected); },
            [&]() -> std::string { return createToIntegerNormalDiagnostic(value, actual, expected); });
    }

    template <std::integral First, std::integral Second>
    static auto createCompareDiagnostic(
        First first, Second second, std::strong_ordering actual, std::strong_ordering expected) -> std::string {
        return std::format(
            "mixedIntegerCompare<{}, {}>({}, {}) returned {}, expected {}.",
            typeName<First>(),
            typeName<Second>(),
            first,
            second,
            orderingName(actual),
            orderingName(expected));
    }

    template <std::integral T>
    static auto createToUnsignedAbsoluteDiagnostic(
        T value, std::make_unsigned_t<T> actual, std::make_unsigned_t<T> expected) -> std::string {
        return std::format(
            "toUnsignedAbsolute<{}>({}) returned {}, expected {}.", typeName<T>(), value, actual, expected);
    }

    template <std::integral T>
    static auto createToIntegerNormalDiagnostic(T value, T actual, T expected) -> std::string {
        return std::format("toIntegerNormal<{}>({}) returned {}, expected {}.", typeName<T>(), value, actual, expected);
    }

    static auto orderingName(std::strong_ordering ordering) -> std::string_view {
        if (ordering == std::strong_ordering::less) {
            return "less";
        }
        if (ordering == std::strong_ordering::greater) {
            return "greater";
        }
        return "equal";
    }

    template <std::integral T>
    static constexpr auto typeName() -> std::string_view {
        if constexpr (std::same_as<T, std::int8_t>) {
            return "std::int8_t";
        } else if constexpr (std::same_as<T, std::uint8_t>) {
            return "std::uint8_t";
        } else if constexpr (std::same_as<T, std::int16_t>) {
            return "std::int16_t";
        } else if constexpr (std::same_as<T, std::uint16_t>) {
            return "std::uint16_t";
        } else if constexpr (std::same_as<T, std::int32_t>) {
            return "std::int32_t";
        } else if constexpr (std::same_as<T, std::uint32_t>) {
            return "std::uint32_t";
        } else if constexpr (std::same_as<T, std::int64_t>) {
            return "std::int64_t";
        } else if constexpr (std::same_as<T, std::uint64_t>) {
            return "std::uint64_t";
        } else {
            return {};
        }
    }
};
