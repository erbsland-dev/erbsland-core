// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"
#include "SignedMagnitude.hpp"

#include "impl/ConstexprSaturatingMathHelper.hpp"

namespace erbsland::math {

/// Add two same-size native integers and clamp the result to a custom bounded range.
/// @seedoc{/reference/math/saturating_math}
/// @tparam tFirst The first operand type.
/// @tparam tSecond The second operand type.
/// @tparam tResult The result and bounds type.
/// @param first The first operand.
/// @param second The second operand.
/// @param minimum The smallest allowed result value.
/// @param maximum The largest allowed result value.
/// @return The mathematical sum clamped to `[minimum, maximum]`.
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto saturatingAddBounded(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.saturatingAddBounded(secondValue, minimum, maximum);
}

/// Test if adding two same-size native integers would clamp to a custom bounded range.
/// @return `true` if the mathematical sum is outside `[minimum, maximum]`.
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto willAddBoundedSaturate(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.wouldAddBoundedSaturate(secondValue, minimum, maximum);
}

/// Subtract two same-size native integers and clamp the result to a custom bounded range.
/// @return The mathematical difference clamped to `[minimum, maximum]`.
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto saturatingSubtractBounded(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.saturatingAddBounded(secondValue.negated(), minimum, maximum);
}

/// Test if subtracting two same-size native integers would clamp to a custom bounded range.
/// @return `true` if the mathematical difference is outside `[minimum, maximum]`.
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto willSubtractBoundedSaturate(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.wouldAddBoundedSaturate(secondValue.negated(), minimum, maximum);
}

/// Negate a same-size native integer and clamp the result to a custom bounded range.
/// @return The mathematical negation clamped to `[minimum, maximum]`.
template <NativeInteger tValue, NativeInteger tResult>
    requires(sizeof(tValue) == sizeof(tResult))
[[nodiscard]] constexpr auto saturatingNegateBounded(tValue value, tResult minimum, tResult maximum) noexcept
    -> tResult {
    return SignedMagnitude<tResult>::fromValue(value).negated().toSaturatingValue(minimum, maximum);
}

/// Test if negating a same-size native integer would clamp to a custom bounded range.
/// @return `true` if the mathematical negation is outside `[minimum, maximum]`.
template <NativeInteger tValue, NativeInteger tResult>
    requires(sizeof(tValue) == sizeof(tResult))
[[nodiscard]] constexpr auto willNegateBoundedSaturate(tValue value, tResult minimum, tResult maximum) noexcept
    -> bool {
    return SignedMagnitude<tResult>::fromValue(value).negated().wouldSaturate(minimum, maximum);
}

/// Increment a same-size native integer and clamp the result to a custom bounded range.
/// @return The value plus one clamped to `[minimum, maximum]`.
template <NativeInteger tValue, NativeInteger tResult>
    requires(sizeof(tValue) == sizeof(tResult))
[[nodiscard]] constexpr auto saturatingIncrementBounded(tValue value, tResult minimum, tResult maximum) noexcept
    -> tResult {
    return saturatingAddBounded(value, tResult{1}, minimum, maximum);
}

/// Decrement a same-size native integer and clamp the result to a custom bounded range.
/// @return The value minus one clamped to `[minimum, maximum]`.
template <NativeInteger tValue, NativeInteger tResult>
    requires(sizeof(tValue) == sizeof(tResult))
[[nodiscard]] constexpr auto saturatingDecrementBounded(tValue value, tResult minimum, tResult maximum) noexcept
    -> tResult {
    return saturatingSubtractBounded(value, tResult{1}, minimum, maximum);
}

/// Multiply two native integers and clamp the result to a custom bounded range.
/// @return The mathematical product clamped to `[minimum, maximum]`.
/// @tested{ConstexprSaturatingMathTest}
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto saturatingMultiplyBounded(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.saturatingMultiplyBounded(secondValue, minimum, maximum);
}

/// Test if multiplying two native integers would clamp to a custom bounded range.
/// @return `true` if the mathematical product is outside `[minimum, maximum]`.
/// @tested{ConstexprSaturatingMathTest}
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto willMultiplyBoundedSaturate(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.wouldMultiplyBoundedSaturate(secondValue, minimum, maximum);
}

/// Divide two native integers and clamp the result to a custom bounded range.
/// @return The mathematical quotient clamped to `[minimum, maximum]`.
/// @tested{ConstexprSaturatingMathTest}
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto saturatingDivideBounded(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.saturatingDivideBounded(secondValue, minimum, maximum);
}

/// Test if dividing two native integers would clamp to a custom bounded range.
/// @return `true` if the mathematical quotient is outside `[minimum, maximum]`.
/// @tested{ConstexprSaturatingMathTest}
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto willDivideBoundedSaturate(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.wouldDivideBoundedSaturate(secondValue, minimum, maximum);
}

/// Calculate the modulo of two native integers and clamp the result to a custom bounded range.
/// @return The mathematical remainder clamped to `[minimum, maximum]`.
/// @tested{ConstexprSaturatingMathTest}
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto saturatingModuloBounded(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> tResult {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.saturatingModuloBounded(secondValue, minimum, maximum);
}

/// Test if a modulo operation between two native integers would clamp to a custom bounded range.
/// @return `true` if the mathematical remainder is outside `[minimum, maximum]`.
/// @tested{ConstexprSaturatingMathTest}
template <NativeInteger tFirst, NativeInteger tSecond, NativeInteger tResult>
    requires impl::SameSizeBoundedIntegerOperation<tFirst, tSecond, tResult>
[[nodiscard]] constexpr auto willModuloBoundedSaturate(
    tFirst first, tSecond second, tResult minimum, tResult maximum) noexcept -> bool {
    const auto firstValue = SignedMagnitude<tResult>::fromValue(first);
    const auto secondValue = SignedMagnitude<tResult>::fromValue(second);
    return firstValue.wouldModuloBoundedSaturate(secondValue, minimum, maximum);
}

}
