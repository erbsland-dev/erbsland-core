// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::text {

/// Flags for byte block text formatting.
/// @tested{ByteFormatTest}
enum class ByteFormatFlag : uint8_t {
    Separator = 1U << 0U,  ///< Insert separators between bytes or byte groups.
    ByteGroups = 1U << 1U, ///< Group multiple bytes between separators.
    Lines = 1U << 2U,      ///< Split output into lines.
    Offset = 1U << 3U,     ///< Add a hexadecimal byte offset in front of each line.
    LineGroups = 1U << 4U, ///< Insert an empty line after each line group.
    All = (1U << 0U) | (1U << 1U) | (1U << 2U) | (1U << 3U) | (1U << 4U),
};

/// A set of byte format flags.
using ByteFormatFlags = util::EnumFlags<ByteFormatFlag>;

/// Combine two byte format flags.
[[nodiscard]] constexpr auto operator|(ByteFormatFlag left, ByteFormatFlag right) noexcept -> ByteFormatFlags {
    return ByteFormatFlags{left} | right;
}

}
