// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerMath.hpp"
#include "IntegerTraits.hpp"

#include "impl/SaturatingMathHelper.hpp"

#ifdef _MSC_VER
#include <safeint.h>
#endif

namespace erbsland::math {

template <NativeInteger T>
auto saturatingAdd(T first, T second) noexcept -> T {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_add_overflow)
    T result;
    if (__builtin_add_overflow(first, second, &result)) {
        if constexpr (std::signed_integral<T>) {
            return (first < 0) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
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
    if (!msl::utilities::SafeAdd(first, second, result)) {
        if constexpr (std::signed_integral<T>) {
            return (first < 0) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
#else
#error "Missing saturating add implementation for the compiler you use."
#endif
}

template <NativeInteger T>
auto willAddOverflow(T first, T second) noexcept -> bool {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_add_overflow)
    T result;
    return __builtin_add_overflow(first, second, &result);
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    return !msl::utilities::SafeAdd(first, second, result);
#else
#error "Missing saturating add implementation for the compiler you use."
#endif
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingAdd(tFirst first, tSecond second) noexcept -> tFirst {
    if constexpr (SameSignednessNativeIntegers<tFirst, tSecond>) {
        using C = CompatibleNativeIntegerT<tFirst, tSecond>;
        return saturatingCast<tFirst>(saturatingAdd<C>(static_cast<C>(first), static_cast<C>(second)));
    } else if constexpr (std::signed_integral<tFirst>) { // signed target, unsigned operation
        if constexpr (SecondHasGreaterPositiveRange<tFirst, tSecond>) {
            return (willCastOverflow<tFirst>(second))
                ? saturatingAdd<tFirst>(
                      saturatingAdd<tFirst>(
                          saturatingAdd<tFirst>(first, std::numeric_limits<tFirst>::max()), tFirst{1}),
                      saturatingCast<tFirst>(second - (tSecond{1} << (sizeof(tFirst) * 8 - 1))))
                : saturatingAdd<tFirst>(first, saturatingCast<tFirst>(second));
        } else {
            return saturatingAdd<tFirst>(first, saturatingCast<tFirst>(second));
        }
    } else { // unsigned target, signed operation.
        return (second < 0) ? saturatingSubtract<tFirst>(first, saturatingCast<tFirst>(toUnsignedAbsolute(second)))
                            : saturatingAdd<tFirst>(first, saturatingCast<tFirst>(second));
    }
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto willAddOverflow(tFirst first, tSecond second) noexcept -> bool {
    if constexpr (SameSignednessNativeIntegers<tFirst, tSecond>) {
        using C = CompatibleNativeIntegerT<tFirst, tSecond>;
        if (willAddOverflow<C>(static_cast<C>(first), static_cast<C>(second))) {
            return true;
        }
        const C c = saturatingAdd(static_cast<C>(first), static_cast<C>(second));
        return willCastOverflow<tFirst>(c);
    } else if constexpr (std::signed_integral<tFirst>) { // signed target, unsigned operation
        if constexpr (SecondHasGreaterPositiveRange<tFirst, tSecond>) {
            if (willCastOverflow<tFirst>(second)) {
                if (willAddOverflow<tFirst>(first, std::numeric_limits<tFirst>::max())) {
                    return true;
                }
                first = saturatingAdd<tFirst>(first, std::numeric_limits<tFirst>::max());
                if (willAddOverflow<tFirst>(first, tFirst{1})) {
                    return true;
                }
                first = saturatingAdd<tFirst>(first, tFirst{1});
                const auto remaining = second - (static_cast<tSecond>(1) << (sizeof(tFirst) * 8 - 1));
                if (willCastOverflow<tFirst>(remaining)) {
                    return true;
                }
                return willAddOverflow<tFirst>(first, saturatingCast<tFirst>(remaining));
            }
        }
        return willAddOverflow<tFirst>(first, saturatingCast<tFirst>(second));
    } else { // unsigned target, signed operation.
        if (second < 0) {
            if (willCastOverflow<tFirst>(toUnsignedAbsolute(second))) {
                return true;
            }
            return willSubtractOverflow<tFirst>(first, saturatingCast<tFirst>(toUnsignedAbsolute(second)));
        }
        if (willCastOverflow<tFirst>(second)) {
            return true;
        }
        return willAddOverflow<tFirst>(first, saturatingCast<tFirst>(second));
    }
}

}
