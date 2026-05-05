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
auto saturatingModulo(T first, T second) noexcept -> T {
    if (second == 0) {
        std::terminate(); // division by zero.
    }
    if constexpr (std::signed_integral<T>) {
        if (first == std::numeric_limits<T>::min() && second == T{-1}) {
            return T{0};
        }
    }
    // Modulo with two equal types is always safe.
    return first % second;
}

template <NativeInteger T>
auto willModuloOverflow([[maybe_unused]] T first, [[maybe_unused]] T second) noexcept -> bool {
    return false;
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingModulo(tFirst first, tSecond second) noexcept -> tFirst {
    if (second == tSecond{0}) {
        std::terminate(); // division by zero.
    }
    // Race conditions to handle (8bit signed/8bit unsigned example):
    // 1. -0x80%0x80 = 0 / After overflow cast -0x80/0x7f = -1 WRONG!
    //
    // Explanation for this implementation:
    // In c++, modulo is defined as remainder of the division, so ((a/b)*b+m)==a.
    // Therefore, the sign of the modulo operator has no influence to the result. The result always has the same
    // sign as `a`.
    //
    // By converting `b` to an absolute unsigned integer, the race condition of [int min]!=[int max] is
    // avoided. This is working, except if `a` is [int min] and `b` is a larger type, abs([int min]) == b.
    // E.g. with `a` = signed 8-bit, -0x80 and `b` = unsigned 0x80. The cast would overflow, `b` limited to 0x7f.
    // `a` would be taken as result and lead to the wrong result.
    //
    // Therefore, this condition is checked, by comparing abs(a)==abs(b) first and return zero.
    //
    using AbsoluteB = std::make_unsigned_t<tSecond>;
    const auto absoluteB = toUnsignedAbsolute(second);
    if constexpr (std::signed_integral<tSecond> || SecondHasGreaterPositiveRange<tFirst, tSecond>) {
        if constexpr (SecondHasGreaterPositiveRange<tFirst, AbsoluteB>) {
            return (toUnsignedAbsolute(first) == absoluteB)
                ? 0
                : (willCastOverflow<tFirst>(absoluteB)
                          ? first
                          : saturatingModulo<tFirst>(first, saturatingCast<tFirst>(absoluteB)));
        } else {
            return saturatingModulo<tFirst>(first, saturatingCast<tFirst>(absoluteB));
        }
    } else {
        return (toUnsignedAbsolute(first) == absoluteB)
            ? 0
            : (willCastOverflow<tFirst>(absoluteB)
                      ? first
                      : saturatingModulo<tFirst>(first, saturatingCast<tFirst>(absoluteB)));
    }
}

template <NativeInteger tFirst, NativeInteger tSecond>
auto willModuloOverflow([[maybe_unused]] tFirst first, [[maybe_unused]] tSecond second) noexcept -> bool {
    return false;
}

}
