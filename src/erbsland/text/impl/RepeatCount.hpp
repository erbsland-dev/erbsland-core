// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"

#include "../../math/IntegerConversion.hpp"
#include "../../math/SaturatingMath.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Convert a bounded repeat count to a native storage size.
/// @tparam T The count type.
/// @param count A finite count representable by the native storage size.
/// @return The native repeat count.
/// @tested{StringStorageBoundsTest}
template <typename T>
[[nodiscard]] auto repeatCountToSize(const T count) -> std::size_t {
    if (count.isInfinite()) {
        throwParameterFailure("Repeated string count must be finite", "count");
    }
    if (math::willCastOverflow<std::size_t>(count.toRawValue())) {
        throwParameterFailure("Repeated string count exceeds native size bounds", "count");
    }
    return static_cast<std::size_t>(count.toRawValue());
}

}
