// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::text {

/// Flags for creating safe string representations for logs and diagnostics.
/// @tested{StringTransformTest}
enum class SafeStringFlag : uint8_t {
    None = 0,        ///< No optional safe-string behavior.
    OnlyAscii = 1U,  ///< Escape all non-ASCII characters.
    AutoQuotes = 2U, ///< Add quotes when the escaped text needs them for readability.
    Defaults = AutoQuotes,
    All = OnlyAscii | AutoQuotes,
};

/// A set of safe string flags.
using SafeStringFlags = util::EnumFlags<SafeStringFlag>;

/// Combine two safe string flags.
[[nodiscard]] constexpr auto operator|(SafeStringFlag left, SafeStringFlag right) noexcept -> SafeStringFlags {
    return SafeStringFlags{left} | right;
}

}
