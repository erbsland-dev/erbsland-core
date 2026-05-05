// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

#include "impl/SaturatingMathHelper.hpp"

#ifdef _MSC_VER
#include <safeint.h>
#endif

namespace erbsland::math {

template <NativeInteger T>
auto saturatingMultiply(T first, T second) noexcept -> T {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_mul_overflow)
    T result;
    if (__builtin_mul_overflow(first, second, &result)) {
        if constexpr (std::signed_integral<T>) {
            return ((first < 0) ^ (second < 0)) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    if (!msl::utilities::SafeMultiply(first, second, result)) {
        if constexpr (std::signed_integral<T>) {
            return ((first < 0) ^ (second < 0)) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Missing saturating multiply implementation for the compiler you use."
#endif
}

template <NativeInteger T>
auto willMultiplyOverflow(T first, T second) noexcept -> bool {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_mul_overflow)
    T result;
    return __builtin_mul_overflow(first, second, &result);
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    return !msl::utilities::SafeMultiply(first, second, result);
#else
#error "Missing saturating multiply implementation for the compiler you use."
#endif
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingMultiply(tFirst first, tSecond second) noexcept -> tFirst {
    if (first == tFirst{0} || second == tSecond{0}) {
        return tFirst{0};
    }
    if constexpr (std::unsigned_integral<tFirst> && std::signed_integral<tSecond>) {
        // unsigned/signed needs an additional check.
        return (second < 0)
            ? 0
            : (willCastOverflow<tFirst>(second) ? impl::extremeIntFromMultiplication(first, second)
                                                : saturatingMultiply<tFirst>(first, saturatingCast<tFirst>(second)));
    } else {
        return willCastOverflow<tFirst>(second) ? impl::extremeIntFromMultiplication(first, second)
                                                : saturatingMultiply<tFirst>(first, saturatingCast<tFirst>(second));
    }
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto willMultiplyOverflow(tFirst first, tSecond second) noexcept -> bool {
    if (first == 0 || second == 0 || second == 1) {
        return false;
    }
    if constexpr (std::unsigned_integral<tFirst> && std::signed_integral<tSecond>) {
        // unsigned/signed needs an additional check.
        if (second < 0) {
            return true;
        }
    }
    if constexpr (std::signed_integral<tFirst> && SecondHasGreaterPositiveRange<tFirst, tSecond>) {
        // edge condition, when a == -1 and b == a[min]
        if (first == -1 && second == static_cast<tSecond>(toUnsignedAbsolute(std::numeric_limits<tFirst>::min()))) {
            return false;
        }
    }
    if (willCastOverflow<tFirst>(second) && first != 0) {
        return true;
    }
    return willMultiplyOverflow<tFirst>(first, saturatingCast<tFirst>(second));
}

}
