// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

#include <type_traits>

namespace erbsland::math {

/// Convert an integer operand to its native integer value.
///
/// `SaturatingInteger` unwraps via `toRawValue()`. Native integers pass through unchanged.
/// @tparam T The integer operand type.
/// @param value The value to convert.
/// @return The native integer representation.
/// @tested{IntegerTraitsTest}
template <AnyIntegerType T>
constexpr auto toNativeInteger(T value) noexcept -> NativeIntegerOfT<T> {
    if constexpr (SaturatingIntegerType<T>) {
        return value.toRawValue();
    } else {
        return value;
    }
}

/// Convert an integer operand to a `SaturatingInteger`.
///
/// `SaturatingInteger` values pass through unchanged. Native integers are wrapped in a `SaturatingInteger`.
/// @tparam T The integer operand type.
/// @param value The value to convert.
/// @return The value wrapped in a ``SaturatingInteger``.
/// @tested{IntegerTraitsTest}
template <AnyIntegerType T>
constexpr auto toSaturatingInteger(T value) noexcept -> SaturatingInteger<NativeIntegerOfT<T>> {
    if constexpr (SaturatingIntegerType<T>) {
        return value;
    } else {
        return SaturatingInteger<std::remove_cvref_t<T>>{value};
    }
}

}
