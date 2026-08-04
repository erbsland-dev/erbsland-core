// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerTraits.hpp"

namespace erbsland::math::impl {

/// Test if an integer value is negative.
/// This helper is safe to use with unsigned integers, where it always returns `false`.
/// @tparam T Any signed or unsigned integer type.
/// @param value The value to test.
/// @return `true` if the value is signed and below zero.
template <NativeInteger T>
constexpr auto isNegative(T value) noexcept -> bool {
    if constexpr (std::signed_integral<T>) {
        return value < T{0};
    } else {
        return false;
    }
}

/// Get the saturated extreme value for a multiplication overflow.
/// The returned value is the minimum of `A` for a negative signed product, and the maximum of `A` otherwise.
/// @tparam tFirst The target integer type.
/// @tparam tSecond The second operand integer type.
/// @param first The first multiplication operand.
/// @param second The second multiplication operand.
/// @return The saturated extreme value for the product sign.
template <NativeInteger tFirst, NativeInteger tSecond>
constexpr auto extremeIntFromMultiplication(tFirst first, tSecond second) noexcept -> tFirst {
    if constexpr (std::signed_integral<tFirst>) {
        return (isNegative(first) != isNegative(second)) ? std::numeric_limits<tFirst>::min()
                                                         : std::numeric_limits<tFirst>::max();
    } else {
        return std::numeric_limits<tFirst>::max();
    }
}

/// Convert a value when the target's maximum is representable by the source.
template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto saturatingCast_maxIfTargetLarger(tSourceType value) noexcept -> tTargetType {
    return value > static_cast<tSourceType>(std::numeric_limits<tTargetType>::max())
        ? std::numeric_limits<tTargetType>::max()
        : static_cast<tTargetType>(value);
}

/// Convert between signed and unsigned integer types with saturation.
template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto saturatingCast_mixedSign(tSourceType value) noexcept -> tTargetType {
    if constexpr (std::unsigned_integral<tTargetType>) {
        // source is signed, target unsigned.
        if constexpr (sizeof(tTargetType) < sizeof(tSourceType)) {
            return (value < 0) ? 0 : saturatingCast_maxIfTargetLarger<tTargetType, tSourceType>(value);
        } else {
            return (value < 0) ? 0 : static_cast<tTargetType>(value);
        }
    } else {
        // source is unsigned, target signed.
        if constexpr (sizeof(tTargetType) <= sizeof(tSourceType)) {
            return saturatingCast_maxIfTargetLarger<tTargetType, tSourceType>(value);
        } else {
            return static_cast<tTargetType>(value);
        }
    }
}

/// Convert to a narrower integer type with saturation.
template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto saturatingCast_downSize(tSourceType value) noexcept -> tTargetType {
    if constexpr (std::signed_integral<tTargetType>) {
        return (value < static_cast<tSourceType>(std::numeric_limits<tTargetType>::min()))
            ? std::numeric_limits<tTargetType>::min()
            : saturatingCast_maxIfTargetLarger<tTargetType, tSourceType>(value);
    } else {
        return saturatingCast_maxIfTargetLarger<tTargetType, tSourceType>(value);
    }
}

}
