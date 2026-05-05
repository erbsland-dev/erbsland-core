// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyIntegerTypes.hpp"

#include <concepts>
#include <cstdint>
#include <exception>
#include <limits>
#include <type_traits>

namespace erbsland::math {

/// Check if both native integer types have the same signedness.
/// - signed, signed => true
/// - unsigned, unsigned => true
/// - signed, unsigned => false
/// - unsigned, signed => false
/// @tparam tFirst The first type to test.
/// @tparam tSecond The second type to test.
template <typename tFirst, typename tSecond>
concept SameSignednessNativeIntegers =
    NativeIntegerPair<tFirst, tSecond> && (std::signed_integral<tFirst> == std::signed_integral<tSecond>);

/// Get the wider native integer of both.
/// @tparam tFirst The first integer type.
/// @tparam tSecond The second integer type.
template <NativeInteger tFirst, NativeInteger tSecond>
struct WiderNativeInteger {
    /// The type with the larger storage size.
    using type = std::conditional_t<sizeof(tFirst) >= sizeof(tSecond), tFirst, tSecond>;
};

/// @see WiderNativeInteger
template <NativeInteger tFirst, NativeInteger tSecond>
using WiderNativeIntegerT = WiderNativeInteger<tFirst, tSecond>::type;

/// Check if the second type can represent a greater range of positive values.
/// This is true when both types have the same size and the second is unsigned while the first is signed,
/// or when the second type is strictly wider.
/// @tparam tFirst The first type.
/// @tparam tSecond The second type.
template <typename tFirst, typename tSecond>
concept SecondHasGreaterPositiveRange = NativeIntegerPair<tFirst, tSecond> &&
    ((sizeof(tFirst) == sizeof(tSecond) && std::signed_integral<tFirst> && std::unsigned_integral<tSecond>) ||
        (sizeof(tSecond) > sizeof(tFirst)));

/// Get a compatible native integer for an operation between two integer types.
/// This template provides a convenient way to find a safe common integer type for an integer operation.
/// Compared with `std::common_type` it does not allow mixing of signed and unsigned integers.
/// Compared with `std::common_type` it handles `uint16_t` as 16bit values if the implementation allows it.
/// The result is `type`, which is only a valid integer if both types are integer and are compatible
/// for a safe operation. See `IntegerMath.hpp` for mixed operations and comparisons.
/// @tparam tFirst The first integer type.
/// @tparam tSecond The second integer type.
template <NativeInteger tFirst, NativeInteger tSecond>
struct CompatibleNativeInteger {
    /// The wider compatible integer type, or `void` for mixed signedness.
    using type = std::conditional_t<
        SameSignednessNativeIntegers<tFirst, tSecond>,
        std::remove_const_t<WiderNativeIntegerT<tFirst, tSecond>>,
        void>;
};

/// @see CompatibleNativeInteger
template <NativeInteger tFirst, NativeInteger tSecond>
using CompatibleNativeIntegerT = CompatibleNativeInteger<tFirst, tSecond>::type;

/// Extract the native integer type from a normalized operand type.
template <typename T>
struct NativeIntegerOfImpl {
    /// The native integer type.
    using type = T;
};

/// Extract the wrapped native integer type from a `SaturatingInteger`.
template <NativeInteger T>
struct NativeIntegerOfImpl<SaturatingInteger<T>> {
    /// The wrapped native integer type.
    using type = T;
};

/// Get the native integer type from an integer operand.
template <AnyIntegerType T>
struct NativeIntegerOf {
    /// The native integer type from a native integer or `SaturatingInteger`.
    using type = NativeIntegerOfImpl<std::remove_cvref_t<T>>::type;
};

/// Get the native integer type from an integer operand.
template <AnyIntegerType T>
using NativeIntegerOfT = NativeIntegerOf<T>::type;

// Check if a native integer and an operand have matching signedness.
template <typename tValue, typename T>
concept SignCompatibleIntegerOperand =
    NativeInteger<tValue> && AnyIntegerType<T> && SameSignednessNativeIntegers<tValue, NativeIntegerOfT<T>>;

// Check if a native integer and two operands all have matching signedness.
template <typename tValue, typename tFirst, typename tSecond>
concept SignCompatibleIntegerOperandPair = NativeInteger<tValue> && AnyIntegerPair<tFirst, tSecond> &&
    SignCompatibleIntegerOperand<tValue, tFirst> && SignCompatibleIntegerOperand<tValue, tSecond>;

}
