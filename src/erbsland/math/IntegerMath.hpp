// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

namespace erbsland::math {

/// Test if any given integer is negative.
/// Always returns false for unsigned integers.
/// @tparam T Any signed or unsigned integer type.
/// @param value The value to test.
/// @return `true` if the value is negative.
template <NativeInteger T>
[[nodiscard]] constexpr auto isNegativeValue(T value) noexcept -> bool {
    if constexpr (std::signed_integral<T>) {
        return value < T{0};
    } else {
        return false;
    }
}

/// Get the absolute value of an integer as an unsigned integer.
/// This helper avoids undefined behavior for the minimum value of signed integer types.
/// @tparam T Any signed or unsigned integer type.
/// @param value The value to convert.
/// @return The absolute value represented as the matching unsigned integer type.
template <NativeInteger T>
constexpr auto toUnsignedAbsolute(T value) noexcept -> std::make_unsigned_t<T> {
    using UnsignedType = std::make_unsigned_t<T>;
    if constexpr (std::signed_integral<T>) {
        return (value < T{0}) ? (UnsignedType{0} - static_cast<UnsignedType>(value)) : static_cast<UnsignedType>(value);
    } else {
        return value;
    }
}

/// Compare two integer values of mixed native types safely.
/// @tparam tFirst The type of `first` must be an integral type.
/// @tparam tSecond The type of `second` must be an integral type.
/// @param first The first value for comparison.
/// @param second The second value for comparison.
/// @return The result, how `first` compares **to** `second`.
///     `std::strong_ordering::less` means `first` is smaller than `second`.
template <NativeInteger tFirst, NativeInteger tSecond>
[[nodiscard]] constexpr auto mixedIntegerCompare(tFirst first, tSecond second) noexcept -> std::strong_ordering {
    if (std::cmp_less(first, second)) {
        return std::strong_ordering::less;
    }
    if (std::cmp_less(second, first)) {
        return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
}

/// Get the absolute difference between two mixed integers safely.
/// @tparam tFirst The type of `first` must be an integral type.
/// @tparam tSecond The type of `second` must be an integral type.
/// @param first The first value.
/// @param second The second value.
/// @return The absolute difference between the two values.
///   Always an unsigned integer that can represent all absolute-difference-values of both input types.
template <NativeInteger tFirst, NativeInteger tSecond>
[[nodiscard]] constexpr auto integerAbsoluteDifference(tFirst first, tSecond second) noexcept
    -> std::make_unsigned_t<CompatibleNativeIntegerT<tFirst, tSecond>> {
    using Common = CompatibleNativeIntegerT<tFirst, tSecond>;
    using Unsigned = std::make_unsigned_t<Common>;

    const auto commonFirst = static_cast<Common>(first);
    const auto commonSecond = static_cast<Common>(second);
    if constexpr (std::signed_integral<Common>) {
        if ((commonFirst < 0) != (commonSecond < 0)) {
            return static_cast<Unsigned>(toUnsignedAbsolute(commonFirst) + toUnsignedAbsolute(commonSecond));
        }
    }
    return (commonFirst >= commonSecond) ? static_cast<Unsigned>(commonFirst - commonSecond)
                                         : static_cast<Unsigned>(commonSecond - commonFirst);
}

/// Normalize an integer value.
/// If the integer is negative, return -1, if it is zero, return zero if it is positive, return 1.
/// @tparam T The type of the integer.
/// @param value The value to normalize.
/// @return The normalized value, 0, 1, or -1.
template <NativeInteger T>
[[nodiscard]] constexpr auto toIntegerNormal(T value) noexcept -> T {
    if constexpr (std::is_signed_v<T>) {
        if (value == 0) {
            return 0;
        }
        if (value < 0) {
            return static_cast<T>(-1);
        }
        return static_cast<T>(1);
    } else {
        if (value == 0) {
            return 0;
        }
        return static_cast<T>(1);
    }
}

/// Order two bounds in place.
/// If `minimum` is greater than `maximum`, the values are swapped.
/// @tparam T The totally ordered bound type.
/// @param minimum The lower bound after this call.
/// @param maximum The upper bound after this call.
/// @tested{IntegerMathTest}
template <std::totally_ordered T>
constexpr void orderMinimumMaximum(T &minimum, T &maximum) noexcept(noexcept(std::swap(minimum, maximum))) {
    if (minimum > maximum) {
        std::swap(minimum, maximum);
    }
}

}
