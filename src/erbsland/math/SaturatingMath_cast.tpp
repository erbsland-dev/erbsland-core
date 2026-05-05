// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerTraits.hpp"

#include "impl/SaturatingMathHelper.hpp"

#ifdef _MSC_VER
#include <safeint.h>
#endif

namespace erbsland::math {

template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto saturatingCast(tSourceType value) noexcept -> tTargetType {
    if constexpr (std::same_as<tTargetType, tSourceType>) {
        // no cast required.
        return value;
    } else if constexpr (!SameSignednessNativeIntegers<tSourceType, tTargetType>) {
        // same size, but mixed signs.
        return impl::saturatingCast_mixedSign<tTargetType, tSourceType>(value);
    } else if constexpr (sizeof(tTargetType) < sizeof(tSourceType)) {
        // target is smaller than source.
        return impl::saturatingCast_downSize<tTargetType, tSourceType>(value);
    } else {
        // target is larger than source.
        return static_cast<tTargetType>(value);
    }
}

template <NativeInteger tTargetType, NativeInteger tSourceType>
constexpr auto willCastOverflow(tSourceType value) noexcept -> bool {
    if constexpr (!SameSignednessNativeIntegers<tSourceType, tTargetType>) {
        if constexpr (std::unsigned_integral<tTargetType>) { // source is signed, target unsigned.
            if constexpr (sizeof(tSourceType) > sizeof(tTargetType)) {
                return value < 0 || value > static_cast<tSourceType>(std::numeric_limits<tTargetType>::max());
            } else {
                return value < 0;
            }
        } else { // source is unsigned, target signed.
            if constexpr (sizeof(tSourceType) >= sizeof(tTargetType)) {
                return value > static_cast<tSourceType>(std::numeric_limits<tTargetType>::max());
            } else {
                return false; // if target is larger than source, it will never overflow.
            }
        }
    } else {
        if constexpr (sizeof(tSourceType) > sizeof(tTargetType)) {
            if constexpr (std::signed_integral<tTargetType>) {
                return (value < static_cast<tSourceType>(std::numeric_limits<tTargetType>::min())) ||
                    (value > static_cast<tSourceType>(std::numeric_limits<tTargetType>::max()));
            } else {
                return (value > static_cast<tSourceType>(std::numeric_limits<tTargetType>::max()));
            }
        } else {
            return false; // if target is larger or equal than source, it will never overflow.
        }
    }
}

}
