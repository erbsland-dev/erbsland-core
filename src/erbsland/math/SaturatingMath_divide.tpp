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
auto saturatingDivide(T first, T second) noexcept -> T {
    if (second == 0) {
        std::terminate(); // division by zero.
    }
    // There is one rance condition:
    // - (-0x80 / -0x01) => -0x80 WRONG! should be 0x7f
    // There are no reliable built-in functions to handle these saturation issues
    if constexpr (std::signed_integral<T>) {
        return (first == std::numeric_limits<T>::min() && second == T{-1}) ? std::numeric_limits<T>::max()
                                                                           : first / second;
    } else {
        return first / second;
    }
}

template <NativeInteger T>
auto willDivideOverflow(T first, T second) noexcept -> bool {
    if constexpr (std::signed_integral<T>) {
        return first == std::numeric_limits<T>::min() && second == T{-1};
    } else {
        return false;
    }
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingDivide(tFirst first, tSecond second) noexcept -> tFirst {
    // race conditions to handle (8bit signed/8bit unsigned example):
    // 1. -0x80/0x80 = 1 / After overflow cast -0x80/0x7f = 1 OK
    // 2. -0x80/0x81 = 0 / After overflow cast -0x80/0x7f = 1 WRONG!
    if constexpr (std::unsigned_integral<tFirst> && std::signed_integral<tSecond>) {
        // unsigned/signed needs an additional check.
        if constexpr (SecondHasGreaterPositiveRange<tFirst, tSecond>) {
            return (second < 0 || first < toUnsignedAbsolute(second))
                ? 0
                : saturatingDivide<tFirst>(first, saturatingCast<tFirst>(second));
        } else {
            return (second < 0) ? 0 : saturatingDivide<tFirst>(first, saturatingCast<tFirst>(second));
        }
    } else {
        if constexpr (SecondHasGreaterPositiveRange<tFirst, tSecond>) {
            return (toUnsignedAbsolute(first) < toUnsignedAbsolute(second))
                ? 0
                : saturatingDivide<tFirst>(first, saturatingCast<tFirst>(second));
        } else {
            return saturatingDivide<tFirst>(first, saturatingCast<tFirst>(second));
        }
    }
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto willDivideOverflow(tFirst first, tSecond second) noexcept -> bool {
    if constexpr (std::unsigned_integral<tFirst> && std::signed_integral<tSecond>) {
        if (second >= 0 || first == 0) {
            return false;
        }
        return first >= toUnsignedAbsolute(second);
    } else if constexpr (std::signed_integral<tFirst> && std::signed_integral<tSecond>) {
        return (first == std::numeric_limits<tFirst>::min()) && second == tSecond{-1};
    } else {
        return false;
    }
}

}
