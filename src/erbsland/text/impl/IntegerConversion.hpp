// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"

#include "../IntegerParseOptions.hpp"
#include "../StringCharReader.hpp"

#include "../../math/SaturatingMath.hpp"
#include "../../math/SignedMagnitude.hpp"

namespace erbsland::text::impl {

/// Parse an integer from a decoded string reader.
template <math::AnyIntegerType T>
[[nodiscard]] auto parseInteger(StringCharReader reader, const IntegerParseOptions &options) -> T {
    const T value = reader.readIntegerOrThrow<T>(options);
    if (!options.hasFlag(IntegerParseFlag::IgnoreTrailingChars) && !reader.isAtEnd()) {
        throwParseError("Integer text has trailing characters");
    }
    return value;
}

/// Parse an integer from a decoded string reader, returning a default on expected parse failures.
template <math::AnyIntegerType T>
[[nodiscard]] auto parseIntegerOrDefault(
    StringCharReader reader, T defaultValue, const IntegerParseOptions &options) noexcept -> T {
    const auto result = reader.parseInteger(options);
    if (result.status != ReadNumberStatus::Success ||
        (!options.hasFlag(IntegerParseFlag::IgnoreTrailingChars) && !reader.isAtEnd())) {
        return defaultValue;
    }
    if constexpr (std::is_signed_v<T>) {
        const auto magnitude = math::SignedMagnitude<int64_t>{result.isNegative, result.value};
        if (magnitude.wouldSaturate(std::numeric_limits<T>::min(), std::numeric_limits<T>::max())) {
            return defaultValue;
        }
        return static_cast<T>(
            magnitude.toSaturatingValue(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    } else {
        if (result.isNegative || math::willCastOverflow<T>(result.value)) {
            return defaultValue;
        }
        return math::saturatingCast<T>(result.value);
    }
}

}
