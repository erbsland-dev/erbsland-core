// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::text::impl {

/// One decoded direct Unicode decomposition mapping.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
struct UnicodeDecompositionMapping final {
    /// Test if this mapping exists.
    [[nodiscard]] constexpr auto isPresent() const noexcept -> bool { return length != 0; }

    std::array<char32_t, 32> codePoints{}; ///< The direct mapped code points.
    uint8_t length{0};                     ///< The number of mapped code points.
    bool compatibility{false};             ///< Whether this is a compatibility mapping.
};

/// Access the sparse decomposition page directory.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] auto unicodeNormalizationDecompositionPages() noexcept -> std::span<const uint32_t>;

/// Access the encoded direct decomposition mappings.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] auto unicodeNormalizationDecompositionData() noexcept -> std::span<const uint8_t>;

/// Access the sparse canonical combining-class page directory.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] auto unicodeNormalizationCombiningClassPages() noexcept -> std::span<const uint32_t>;

/// Access the encoded nonzero canonical combining classes.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] auto unicodeNormalizationCombiningClassData() noexcept -> std::span<const uint8_t>;

/// Access the sparse canonical composition page directory.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] auto unicodeNormalizationCompositionPages() noexcept -> std::span<const uint32_t>;

/// Access the encoded canonical composition pairs.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] auto unicodeNormalizationCompositionData() noexcept -> std::span<const uint8_t>;

/// Decode one unsigned LEB128 value from generated normalization data.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
inline auto unicodeNormalizationReadUnsigned(const std::span<const uint8_t> data, std::size_t &position) noexcept
    -> uint32_t {

    auto result = uint32_t{0};
    auto shift = uint32_t{0};
    while (position < data.size() && shift < 32U) {
        const auto byte = data[position++];
        result |= static_cast<uint32_t>(byte & 0x7FU) << shift;
        if ((byte & 0x80U) == 0) {
            break;
        }
        shift += 7U;
    }
    return result;
}

/// Find the encoded byte range for one Unicode page.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] inline auto unicodeNormalizationPageRange(
    const std::span<const uint32_t> pages, const std::size_t dataSize, const char32_t codePoint) noexcept
    -> std::array<std::size_t, 2> {

    if (codePoint > 0x10FFFFU) {
        return {};
    }
    const auto page = static_cast<uint32_t>(codePoint) >> 12U;
    const auto found =
        std::lower_bound(pages.begin(), pages.end(), page, [](const uint32_t entry, const uint32_t value) {
            return (entry & 0x1FFU) < value;
        });
    if (found == pages.end() || (*found & 0x1FFU) != page) {
        return {};
    }
    const auto index = static_cast<std::size_t>(found - pages.begin());
    const auto begin = static_cast<std::size_t>(*found >> 9U);
    const auto end = index + 1U < pages.size() ? static_cast<std::size_t>(pages[index + 1U] >> 9U) : dataSize;
    return {begin, end};
}

/// Find a direct Unicode decomposition mapping.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] inline auto unicodeDecompositionFor(const char32_t codePoint) noexcept -> UnicodeDecompositionMapping {
    const auto pages = unicodeNormalizationDecompositionPages();
    const auto data = unicodeNormalizationDecompositionData();
    const auto range = unicodeNormalizationPageRange(pages, data.size(), codePoint);
    auto position = range[0];
    auto current = static_cast<uint32_t>(codePoint) & ~0xFFFU;
    while (position < range[1]) {
        current += unicodeNormalizationReadUnsigned(data, position);
        const auto header = data[position++];
        const auto length = static_cast<uint8_t>((header & 0x1FU) + 1U);
        if (current == codePoint) {
            auto result = UnicodeDecompositionMapping{};
            result.length = length;
            result.compatibility = (header & 0x20U) != 0;
            for (auto index = uint8_t{0}; index < length; ++index) {
                result.codePoints[index] = static_cast<char32_t>(unicodeNormalizationReadUnsigned(data, position));
            }
            return result;
        }
        for (auto index = uint8_t{0}; index < length; ++index) {
            unicodeNormalizationReadUnsigned(data, position);
        }
        if (current > codePoint) {
            break;
        }
    }
    return {};
}

/// Get the canonical combining class for one code point.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] inline auto unicodeCombiningClassFor(const char32_t codePoint) noexcept -> uint8_t {
    const auto pages = unicodeNormalizationCombiningClassPages();
    const auto data = unicodeNormalizationCombiningClassData();
    const auto range = unicodeNormalizationPageRange(pages, data.size(), codePoint);
    auto position = range[0];
    auto current = static_cast<uint32_t>(codePoint) & ~0xFFFU;
    while (position < range[1]) {
        current += unicodeNormalizationReadUnsigned(data, position);
        const auto combiningClass = data[position++];
        if (current == codePoint) {
            return combiningClass;
        }
        if (current > codePoint) {
            break;
        }
    }
    return 0;
}

/// Find the canonical composition for a starter and trailing code point.
/// @tested{UnicodeNormalizationTest UnicodeNormalizationConformanceTest}
[[nodiscard]] inline auto unicodeCompositionFor(const char32_t starter, const char32_t trailing) noexcept -> char32_t {
    const auto pages = unicodeNormalizationCompositionPages();
    const auto data = unicodeNormalizationCompositionData();
    const auto range = unicodeNormalizationPageRange(pages, data.size(), starter);
    auto position = range[0];
    auto currentStarter = static_cast<uint32_t>(starter) & ~0xFFFU;
    while (position < range[1]) {
        currentStarter += unicodeNormalizationReadUnsigned(data, position);
        const auto pairCount = unicodeNormalizationReadUnsigned(data, position);
        auto currentTrailing = uint32_t{0};
        for (auto index = uint32_t{0}; index < pairCount; ++index) {
            currentTrailing += unicodeNormalizationReadUnsigned(data, position);
            const auto composite = unicodeNormalizationReadUnsigned(data, position);
            if (currentStarter == starter && currentTrailing == trailing) {
                return static_cast<char32_t>(composite);
            }
        }
        if (currentStarter > starter) {
            break;
        }
    }
    return 0;
}

}
