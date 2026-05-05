// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::text {

/// Flags for floating point parsing.
/// @tested{FloatConversionTest}
enum class FloatParseFlag : uint8_t {
    IgnoreTrailingChars = 1U << 0U, ///< Stop parsing successfully after the floating point value.
    All = 1U << 0U,
};

/// A set of floating point parse flags.
using FloatParseFlags = util::EnumFlags<FloatParseFlag>;

/// Combine two floating point parse flags.
[[nodiscard]] constexpr auto operator|(FloatParseFlag left, FloatParseFlag right) noexcept -> FloatParseFlags {
    return FloatParseFlags{left} | right;
}

}
