// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::text {

/// Flags for integer parsing.
enum class IntegerParseFlag : uint8_t {
    AllowSeparator = 1U << 0U,      ///< Accept digit group separators.
    IgnoreTrailingChars = 1U << 1U, ///< Stop parsing at the first trailing non-digit character.
    AcceptMinusSign = 1U << 2U,     ///< Accept a leading minus sign.
    IgnorePlusSign = 1U << 3U,      ///< Accept and ignore a leading plus sign.
    StopAtMaximum = 1U << 4U,       ///< Stop reading digits when the maximum digit count is reached.
    All = (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U) | (1U << 4U),
};

/// A set of integer parse flags.
using IntegerParseFlags = util::EnumFlags<IntegerParseFlag>;

/// Combine two integer parse flags.
[[nodiscard]] constexpr auto operator|(IntegerParseFlag left, IntegerParseFlag right) noexcept -> IntegerParseFlags {
    return IntegerParseFlags{left} | right;
}

}
