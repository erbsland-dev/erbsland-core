// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>
#include <cstdint>

namespace erbsland::re::impl::ch {

/// The mask type for the category.
using Mask = uint32_t;

/// Number of bits reserved for a Unicode subcategory.
constexpr static std::size_t cUnicodeSubCategoryBitCount = 7U;
/// Number of bits reserved for a Unicode category.
constexpr static std::size_t cUnicodeCategoryBitCount = 7U;
/// Number of bits reserved for the ASCII restriction.
constexpr static std::size_t cReSubcategoryBitCount = 1U;
/// Number of bits reserved for regular-expression categories.
constexpr static std::size_t cReCategoryBitCount = 8U;
/// Total category mask width.
constexpr static std::size_t cTotalBits =
    cUnicodeSubCategoryBitCount + cUnicodeCategoryBitCount + cReSubcategoryBitCount + cReCategoryBitCount;
static_assert(cTotalBits <= 24U, "The total number of bits for category masks exceeds the maximum allowed (24 bits).");

/// First Unicode subcategory bit.
constexpr static std::size_t cUnicodeSubCategoryBaseBit = 0;
/// First Unicode category bit.
constexpr static std::size_t cUnicodeCategoryBaseBit = cUnicodeSubCategoryBaseBit + cUnicodeSubCategoryBitCount;
/// ASCII restriction bit.
constexpr static std::size_t cReSubCategoryBaseBit = cUnicodeCategoryBaseBit + cUnicodeCategoryBitCount;
/// First regular-expression category bit.
constexpr static std::size_t cReCategoryBaseBit = cReSubCategoryBaseBit + cReSubcategoryBitCount;

/// Create a mask for a one-letter Unicode category.
/// @param categoryIndex The category bit index in the range 0..6.
/// @return The encoded category mask.
/// @tested{CategoryContainsTest CategoryTest}
constexpr auto ucValue(const std::size_t categoryIndex) noexcept -> Mask {
    return static_cast<Mask>(1U) << (categoryIndex + cUnicodeCategoryBaseBit);
}
/// Create a mask for a two-letter Unicode category.
/// @param categoryIndex The category bit index in the range 0..6.
/// @param subcategoryIndex The subcategory bit index in the range 0..6.
/// @return The encoded category mask.
/// @tested{CategoryContainsTest CategoryTest}
constexpr auto ucValue(const std::size_t categoryIndex, const std::size_t subcategoryIndex) noexcept -> Mask {
    return ucValue(categoryIndex) | static_cast<Mask>(1U) << (subcategoryIndex + cUnicodeSubCategoryBaseBit);
}
/// Create a mask for a regular-expression category.
/// @param categoryIndex The category bit index in the range 0..7.
/// @param isAscii Whether to restrict the category to ASCII.
/// @return The encoded category mask.
/// @tested{CategoryContainsTest CategoryTest}
constexpr auto reValue(const std::size_t categoryIndex, bool isAscii) noexcept -> Mask {
    return static_cast<Mask>(1U) << (categoryIndex + cReCategoryBaseBit) |
        static_cast<Mask>(isAscii ? 1U : 0U) << cReSubCategoryBaseBit;
}
/// Create a mask for an ASCII regular-expression category.
/// @param categoryIndex The category bit index in the range 0..7.
/// @return The encoded category mask.
/// @tested{CategoryContainsTest CategoryTest}
constexpr auto reAsciiValue(const std::size_t categoryIndex) noexcept -> Mask {
    return reValue(categoryIndex, true); // Ascii is more limited than Unicode, ASCII flag must be set.
}
/// Create a mask for a Unicode regular-expression category.
/// @param categoryIndex The category bit index in the range 0..7.
/// @return The encoded category mask.
/// @tested{CategoryContainsTest CategoryTest}
constexpr auto reUnicodeValue(const std::size_t categoryIndex) noexcept -> Mask {
    return reValue(categoryIndex, false); // Unicode variant includes all ASCII characters.
}

}
