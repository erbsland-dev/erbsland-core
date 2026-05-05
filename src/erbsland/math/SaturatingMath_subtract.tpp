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
auto saturatingSubtract(T first, T second) noexcept -> T {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_sub_overflow)
    T result;
    if (__builtin_sub_overflow(first, second, &result)) {
        return (second < 0) ? std::numeric_limits<T>::max() : std::numeric_limits<T>::min();
    }
    return result;
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    if (!msl::utilities::SafeSubtract(first, second, result)) {
        return (second < 0) ? std::numeric_limits<T>::max() : std::numeric_limits<T>::min();
    }
    return result;
#else
#error "Missing saturating subtract implementation for the compiler you use."
#endif
}

template <NativeInteger T>
auto willSubtractOverflow(T first, T second) noexcept -> bool {
    // Use reliable built-in functions or stop compilation.
#if __GNUC__ || __clang__
#if __has_builtin(__builtin_sub_overflow)
    T result;
    return __builtin_sub_overflow(first, second, &result);
#else
#error "Please use a current version of the GCC or clang compiler."
#endif
#elif _MSC_VER
    T result;
    return !msl::utilities::SafeSubtract(first, second, result);
#else
#error "Missing saturating subtract implementation for the compiler you use."
#endif
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingSubtract(tFirst first, tSecond second) noexcept -> tFirst {
    if constexpr (SameSignednessNativeIntegers<tFirst, tSecond>) {
        using C = CompatibleNativeIntegerT<tFirst, tSecond>;
        return saturatingCast<tFirst>(saturatingSubtract<C>(static_cast<C>(first), static_cast<C>(second)));
    } else if constexpr (std::signed_integral<tFirst>) { // signed target, unsigned operation
        if constexpr (SecondHasGreaterPositiveRange<tFirst, tSecond>) {
            return (willCastOverflow<tFirst>(second))
                ? saturatingSubtract<tFirst>(
                      saturatingSubtract<tFirst>(
                          saturatingSubtract<tFirst>(first, std::numeric_limits<tFirst>::max()), tFirst{1}),
                      saturatingCast<tFirst>(second - (tSecond{1} << (sizeof(tFirst) * 8U - 1U))))
                : saturatingSubtract<tFirst>(first, saturatingCast<tFirst>(second));
        } else {
            return saturatingSubtract<tFirst>(first, saturatingCast<tFirst>(second));
        }
    } else { // unsigned target, signed operation.
        return (second < 0) ? saturatingAdd<tFirst>(first, saturatingCast<tFirst>(toUnsignedAbsolute(second)))
                            : saturatingSubtract<tFirst>(first, saturatingCast<tFirst>(second));
    }
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto willSubtractOverflow(tFirst first, tSecond second) noexcept -> bool {
    if constexpr (SameSignednessNativeIntegers<tFirst, tSecond>) {
        using C = CompatibleNativeIntegerT<tFirst, tSecond>;
        if (willSubtractOverflow<C>(static_cast<C>(first), static_cast<C>(second))) {
            return true;
        }
        const C c = saturatingSubtract(static_cast<C>(first), static_cast<C>(second));
        return willCastOverflow<tFirst>(c);
    } else if constexpr (std::signed_integral<tFirst>) { // signed target, unsigned operation
        if constexpr (SecondHasGreaterPositiveRange<tFirst, tSecond>) {
            if (willCastOverflow<tFirst>(second)) {
                if (willSubtractOverflow<tFirst>(first, std::numeric_limits<tFirst>::max())) {
                    return true;
                }
                first = saturatingSubtract<tFirst>(first, std::numeric_limits<tFirst>::max());
                if (willSubtractOverflow<tFirst>(first, 1)) {
                    return true;
                }
                first = saturatingSubtract<tFirst>(first, 1);
                const auto remaining = second - (static_cast<tSecond>(1) << (sizeof(tFirst) * 8 - 1));
                if (willCastOverflow<tFirst>(remaining)) {
                    return true;
                }
                return willSubtractOverflow<tFirst>(first, saturatingCast<tFirst>(remaining));
            }
        }
        return willSubtractOverflow<tFirst>(first, saturatingCast<tFirst>(second));
    } else { // unsigned target, signed operation.
        if (second < 0) {
            if (willCastOverflow<tFirst>(toUnsignedAbsolute(second))) {
                return true;
            }
            return willAddOverflow<tFirst>(first, saturatingCast<tFirst>(toUnsignedAbsolute(second)));
        }
        if (willCastOverflow<tFirst>(second)) {
            return true;
        }
        return willSubtractOverflow<tFirst>(first, saturatingCast<tFirst>(second));
    }
}

}
