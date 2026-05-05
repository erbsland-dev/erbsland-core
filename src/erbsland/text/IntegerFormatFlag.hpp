// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::text {

/// Flags for integer text formatting.
/// @tested{IntegerConversionTest}
enum class IntegerFormatFlag : uint8_t {
    ZeroFill = 1U << 0U,   ///< Pad the digit field with zeroes.
    Separator = 1U << 1U,  ///< Insert digit group separators.
    BasePrefix = 1U << 2U, ///< Add a base prefix for hexadecimal, binary, and octal output.
    All = (1U << 0U) | (1U << 1U) | (1U << 2U),
};

/// A set of integer format flags.
using IntegerFormatFlags = util::EnumFlags<IntegerFormatFlag>;

/// Combine two integer format flags.
[[nodiscard]] constexpr auto operator|(IntegerFormatFlag left, IntegerFormatFlag right) noexcept -> IntegerFormatFlags {
    return IntegerFormatFlags{left} | right;
}

}
