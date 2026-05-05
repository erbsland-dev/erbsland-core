// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

#ifdef _MSC_VER
#include <safeint.h>
#endif

namespace erbsland::math {

/// Add two integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type
/// @param first The first value.
/// @param second The second value.
/// @return The addition result, limited to the used type.
template <NativeInteger T>
auto saturatingAdd(T first, T second) noexcept -> T;

/// Subtract two integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type
/// @param first The first value.
/// @param second The second value.
/// @return The subtraction result, limited to the used type.
template <NativeInteger T>
auto saturatingSubtract(T first, T second) noexcept -> T;

/// Multiply two integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type.
/// @param first The first multiplier.
/// @param second The second multiplier.
/// @return The product, limited to the used type.
template <NativeInteger T>
auto saturatingMultiply(T first, T second) noexcept -> T;

/// Divide integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type.
/// @param first dividend.
/// @param second divisor.
/// @return The result of the division, limited to the used type.
template <NativeInteger T>
auto saturatingDivide(T first, T second) noexcept -> T;

/// Get the remainder of a division between integers but limit the result to the maximum possible values.
/// @tparam T Any signed or unsigned integer type.
/// @param first dividend.
/// @param second divisor.
/// @return The result of the division, limited to the used type.
template <NativeInteger T>
auto saturatingModulo(T first, T second) noexcept -> T;

/// Test if an addition will overflow.
/// @tparam T The type for the operands.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger T>
auto willAddOverflow(T first, T second) noexcept -> bool;

/// Test if a subtraction will overflow.
/// @tparam T The type for the operands.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger T>
auto willSubtractOverflow(T first, T second) noexcept -> bool;

/// Test if a multiplication will overflow.
/// @tparam T The type for the operands.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger T>
auto willMultiplyOverflow(T first, T second) noexcept -> bool;

/// Test if a division will overflow.
/// @tparam T The type for the operands.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger T>
auto willDivideOverflow(T first, T second) noexcept -> bool;

/// Test if a modulo will overflow.
/// @tparam T The type for the operands.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger T>
auto willModuloOverflow(T first, T second) noexcept -> bool;

/// Convert an integer type into another one, but make sure the result will not overflow.
/// @seeref{saturating-math-casts}
/// @tparam tTargetType The target type for the cast.
/// @tparam tSourceType The source type to cast.
/// @param value The source value to cast.
/// @return A value as target type.
template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto saturatingCast(tSourceType value) noexcept -> tTargetType;

/// Check if a native cast would overflow.
/// @tparam tTargetType The target type for the cast.
/// @tparam tSourceType The source type to cast.
/// @param value The source value to check for an overflow.
/// @return `true` if a native cast cannot represent `value` in the target type without clipping.
template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto willCastOverflow(tSourceType value) noexcept -> bool;

/// Increment a value but never overflow.
/// @tparam T The type of the value.
/// @param value The value to increment.
template <NativeInteger T>
void saturatingIncrement(T &value) noexcept {
    if (value < std::numeric_limits<T>::max()) {
        ++value;
    }
}

/// Increment a value but never overflow.
/// @tparam T The type of the value.
/// @param value The value to decrement.
template <NativeInteger T>
void saturatingDecrement(T &value) noexcept {
    if (value > std::numeric_limits<T>::min()) {
        --value;
    }
}

/// Saturated add with any integer type.
/// @seeref{saturating-math-mixed-types}
/// @tparam tFirst The target type for the operation.
/// @tparam tSecond The type of the second summand.
/// @param first The value to change.
/// @param second The value to add.
/// @return The result.
template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingAdd(tFirst first, tSecond second) noexcept -> tFirst;

/// Saturated subtract with any integer type.
/// @seeref{saturating-math-mixed-types}
/// @tparam tFirst The target type for the operation.
/// @tparam tSecond The type of the subtrahend.
/// @param first The value to change.
/// @param second The value to subtract.
/// @return The result.
template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingSubtract(tFirst first, tSecond second) noexcept -> tFirst;

/// Saturated multiply with any compatible integer type.
/// @seeref{saturating-math-mixed-types}
/// @tparam tFirst The target type for the operation.
/// @tparam tSecond The type of the factor.
/// @param first The value to change.
/// @param second The factor.
/// @return The result.
template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingMultiply(tFirst first, tSecond second) noexcept -> tFirst;

/// Saturated division with any compatible integer type.
/// @seeref{saturating-math-mixed-types}
/// @tparam tFirst The target type for the operation.
/// @tparam tSecond The type of the divisor.
/// @param first The value to change.
/// @param second The divisor.
/// @return The result.
template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingDivide(tFirst first, tSecond second) noexcept -> tFirst;

/// Saturated modulo with any compatible integer type.
/// @seeref{saturating-math-mixed-types}
/// @tparam tFirst The target type for the operation.
/// @tparam tSecond The type of the divisor.
/// @param first The value to change.
/// @param second The divisor.
/// @return The result.
template <NativeInteger tFirst, NativeInteger tSecond>
auto saturatingModulo(tFirst first, tSecond second) noexcept -> tFirst;

/// Test if an addition will overflow.
/// @tparam tFirst The target type of the operation to test for the overflow.
/// @tparam tSecond Operator type with no influence to the result type.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger tFirst, NativeInteger tSecond>
auto willAddOverflow(tFirst first, tSecond second) noexcept -> bool;

/// Test if a subtraction will overflow.
/// @tparam tFirst The target type of the operation to test for the overflow.
/// @tparam tSecond Operator type with no influence to the result type.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger tFirst, NativeInteger tSecond>
auto willSubtractOverflow(tFirst first, tSecond second) noexcept -> bool;

/// Test if a multiplication will overflow.
/// @tparam tFirst The target type of the operation to test for the overflow.
/// @tparam tSecond Operator type with no influence to the result type.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger tFirst, NativeInteger tSecond>
auto willMultiplyOverflow(tFirst first, tSecond second) noexcept -> bool;

/// Test if a division will overflow.
/// @tparam tFirst The target type of the operation to test for the overflow.
/// @tparam tSecond Operator type with no influence to the result type.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger tFirst, NativeInteger tSecond>
auto willDivideOverflow(tFirst first, tSecond second) noexcept -> bool;

/// Test if a modulo operation will overflow.
/// @tparam tFirst The target type of the operation to test for the overflow.
/// @tparam tSecond Operator type with no influence to the result type.
/// @param first The first value of the operation to test.
/// @param second The second value of the operation to test.
/// @return `true` if an overflow will occur, `false` otherwise.
template <NativeInteger tFirst, NativeInteger tSecond>
auto willModuloOverflow(tFirst first, tSecond second) noexcept -> bool;

}

#include "SaturatingMath_add.tpp"
#include "SaturatingMath_cast.tpp"
#include "SaturatingMath_divide.tpp"
#include "SaturatingMath_modulo.tpp"
#include "SaturatingMath_multiply.tpp"
#include "SaturatingMath_subtract.tpp"
