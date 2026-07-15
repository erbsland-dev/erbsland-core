// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"

#include "../IntegerParseOptions.hpp"
#include "../StringCharReader.hpp"

namespace erbsland::text::impl {

/// Parse an integer from a decoded string reader.
template <math::AnyIntegerType T>
[[nodiscard]] auto parseInteger(StringCharReader reader, const IntegerParseOptions &options) -> T {
    const T value = reader.readIntegerOrThrow<T>(options);
    if (!options.hasFlag(IntegerParseFlag::IgnoreTrailingChars) && !reader.isAtEnd()) {
        text::impl::throwParseError("Integer text has trailing characters");
    }
    return value;
}

/// Parse an integer from a decoded string reader, returning a default on expected parse failures.
template <math::AnyIntegerType T>
[[nodiscard]] auto parseIntegerOrDefault(
    StringCharReader reader, T defaultValue, const IntegerParseOptions &options) noexcept -> T {
    try {
        return parseInteger<T>(reader, options);
    } catch (...) {
        return defaultValue;
    }
}

}
