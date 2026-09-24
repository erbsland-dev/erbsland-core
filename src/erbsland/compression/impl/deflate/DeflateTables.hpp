// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstdint>

/// Shared Deflate length and distance code parameters.
namespace erbsland::compression::impl::deflate_tables {

/// Base length for symbols 257 through 285.
inline constexpr auto cLengthBase = std::array<uint16_t, 29U>{
    3U,
    4U,
    5U,
    6U,
    7U,
    8U,
    9U,
    10U,
    11U,
    13U,
    15U,
    17U,
    19U,
    23U,
    27U,
    31U,
    35U,
    43U,
    51U,
    59U,
    67U,
    83U,
    99U,
    115U,
    131U,
    163U,
    195U,
    227U,
    258U};
/// Extra-bit count for each length symbol.
inline constexpr auto cLengthExtra = std::array<uint8_t, 29U>{
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U, 2U, 2U, 2U, 2U, 3U, 3U, 3U, 3U, 4U, 4U, 4U, 4U, 5U, 5U, 5U, 5U, 0U};
/// Base distance for symbols zero through 29.
inline constexpr auto cDistanceBase = std::array<uint16_t, 30U>{
    1U,
    2U,
    3U,
    4U,
    5U,
    7U,
    9U,
    13U,
    17U,
    25U,
    33U,
    49U,
    65U,
    97U,
    129U,
    193U,
    257U,
    385U,
    513U,
    769U,
    1025U,
    1537U,
    2049U,
    3073U,
    4097U,
    6145U,
    8193U,
    12289U,
    16385U,
    24577U};
/// Extra-bit count for each distance symbol.
inline constexpr auto cDistanceExtra = std::array<uint8_t, 30U>{
    0U,
    0U,
    0U,
    0U,
    1U,
    1U,
    2U,
    2U,
    3U,
    3U,
    4U,
    4U,
    5U,
    5U,
    6U,
    6U,
    7U,
    7U,
    8U,
    8U,
    9U,
    9U,
    10U,
    10U,
    11U,
    11U,
    12U,
    12U,
    13U,
    13U};
}
