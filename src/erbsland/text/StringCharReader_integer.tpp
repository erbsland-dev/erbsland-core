// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/ThrowHelper.hpp"

#include "../math/SaturatingMath.hpp"

namespace erbsland::text {

template <math::AnyIntegerType T>
auto StringCharReader::readIntegerOrThrow(const IntegerParseOptions &options) -> T {
    const auto startState = save();
    const auto result = scanInteger(options);
    if (result.status != ReadNumberStatus::Success) {
        throwError(result.status, result.position);
    }
    if constexpr (std::is_signed_v<T>) {
        const auto magnitude = math::SignedMagnitude<int64_t>{result.isNegative, result.value};
        if (magnitude.wouldSaturate(std::numeric_limits<T>::min(), std::numeric_limits<T>::max())) {
            restore(startState);
            impl::throwOverflow("Integer number exceeds the supported range");
        }
        return static_cast<T>(
            magnitude.toSaturatingValue(std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    } else {
        if (result.isNegative) {
            restore(startState);
            impl::throwOverflow("Integer number exceeds the supported range");
        }
        if (math::willCastOverflow<T>(result.value)) {
            restore(startState);
            impl::throwOverflow("Integer number exceeds the supported range");
        }
        return math::saturatingCast<T>(result.value);
    }
}

}
