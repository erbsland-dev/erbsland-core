// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../UnicodeCategory.hpp"
#include "../UnicodeCategoryGroup.hpp"

#include "../../unit/Version.hpp"

#include <algorithm>
#include <cstdint>
#include <span>

namespace erbsland::text::impl {

/// One range-compressed Unicode Light entry.
struct UnicodeData final {
    using CodePoint = char32_t;
    using DeltaIndex = uint8_t;

    /// Create an empty Unicode data entry.
    constexpr UnicodeData() noexcept = default;
    /// Create a Unicode data entry for a compressed character range.
    constexpr UnicodeData(
        const CodePoint start,
        const UnicodeCategory category,
        const DeltaIndex deltaIndex = 0U,
        const uint8_t displayWidth = 1U) noexcept :
        start{start},
        categoryValue{static_cast<uint8_t>(category)},
        deltaIndex{deltaIndex},
        displayWidth{displayWidth} {}

    /// Get the category of this Unicode character.
    [[nodiscard]] constexpr auto category() const noexcept -> UnicodeCategory {
        return static_cast<UnicodeCategory>(categoryValue);
    }

    /// Get the category group of this Unicode character.
    [[nodiscard]] constexpr auto categoryGroup() const noexcept -> UnicodeCategoryGroup {
        return static_cast<UnicodeCategoryGroup>(categoryValue >> 4U);
    }

    CodePoint start{0};       ///< The first code point in this compressed range.
    uint8_t categoryValue{0}; ///< The encoded `UnicodeCategory` value.
    DeltaIndex deltaIndex{0}; ///< Index into the simple case-mapping delta table.
    uint8_t displayWidth{1};  ///< Terminal-cell display width.
};

/// One set of simple Unicode case-mapping deltas.
struct UnicodeDelta final {
    int32_t caseFold{0};  ///< Delta for simple case folding.
    int32_t lowercase{0}; ///< Delta for simple lowercase mapping.
    int32_t uppercase{0}; ///< Delta for simple uppercase mapping.
};

/// Access the dense ASCII Unicode Light table.
[[nodiscard]] auto asciiUnicodeDataTable() noexcept -> std::span<const UnicodeData>;

/// Access the range-compressed Unicode Light table for all code points >= 128.
[[nodiscard]] auto unicodeDataMap() noexcept -> std::span<const UnicodeData>;

/// Access the simple case-mapping delta table.
[[nodiscard]] auto unicodeDeltaTable() noexcept -> std::span<const UnicodeDelta>;

/// Access the Unicode Character Database version used for the generated Unicode Light data.
[[nodiscard]] auto unicodeDataVersion() noexcept -> unit::Version;

/// Find Unicode Light data for one code point.
[[nodiscard]] inline auto unicodeDataFor(const UnicodeData::CodePoint codePoint) noexcept -> const UnicodeData & {
    if (codePoint < 128U) {
        return asciiUnicodeDataTable()[static_cast<std::size_t>(codePoint)];
    }
    const auto unicodeData = unicodeDataMap();
    const auto it = std::upper_bound(
        unicodeData.begin(),
        unicodeData.end(),
        codePoint,
        [](const char32_t testedCodePoint, const UnicodeData &data) noexcept -> bool {
            return testedCodePoint < data.start;
        });
    return *(it - 1);
}

/// Get the terminal display width for one Unicode code point.
[[nodiscard]] inline auto unicodeDisplayWidthFor(const UnicodeData::CodePoint codePoint) noexcept -> uint8_t {
    if (codePoint > 0x10FFFFU) {
        return 1U;
    }
    return unicodeDataFor(codePoint).displayWidth;
}

/// Get the case-mapping delta data for one Unicode data entry.
[[nodiscard]] inline auto unicodeDeltaFor(const UnicodeData &data) noexcept -> UnicodeDelta {
    const auto table = unicodeDeltaTable();
    const auto index = static_cast<std::size_t>(data.deltaIndex);
    if (index >= table.size()) {
        return {};
    }
    return table[index];
}

/// Test whether a Unicode range has a selected mapping delta.
template <typename DeltaIndexSelector>
[[nodiscard]] inline auto unicodeContainsMappableCharacters(
    UnicodeData::CodePoint begin, UnicodeData::CodePoint end, DeltaIndexSelector deltaSelector) noexcept -> bool {

    if (begin > end || begin > 0x10FFFFU) {
        return false;
    }
    end = std::min(end, UnicodeData::CodePoint{0x10FFFFU});

    if (begin < 128U) {
        const auto asciiEnd = std::min(end, UnicodeData::CodePoint{127U});
        const auto asciiTable = asciiUnicodeDataTable();
        for (auto codePoint = begin; codePoint <= asciiEnd; ++codePoint) {
            if (deltaSelector(unicodeDeltaFor(asciiTable[static_cast<std::size_t>(codePoint)])) != 0) {
                return true;
            }
        }
        begin = 128U;
        if (begin > end) {
            return false;
        }
    }

    const auto unicodeData = unicodeDataMap();
    if (unicodeData.empty()) {
        return false;
    }

    auto it = std::upper_bound(
        unicodeData.begin(),
        unicodeData.end(),
        begin,
        [](const char32_t testedCodePoint, const UnicodeData &data) noexcept -> bool {
            return testedCodePoint < data.start;
        });
    if (it == unicodeData.begin()) {
        it = unicodeData.begin();
    } else {
        --it;
    }

    while (it != unicodeData.end() && it->start <= end) {
        if (deltaSelector(unicodeDeltaFor(*it)) != 0) {
            return true;
        }
        ++it;
    }
    return false;
}

/// Test if a Unicode scalar range contains characters affected by simple case folding.
[[nodiscard]] inline auto unicodeContainsCaseFoldableCharacters(
    const UnicodeData::CodePoint begin, const UnicodeData::CodePoint end) noexcept -> bool {

    return unicodeContainsMappableCharacters(
        begin, end, [](const UnicodeDelta &delta) noexcept -> int32_t { return delta.caseFold; });
}

/// Test if a Unicode scalar range contains characters affected by simple lowercase mapping.
[[nodiscard]] inline auto unicodeContainsLowercaseMappableCharacters(
    const UnicodeData::CodePoint begin, const UnicodeData::CodePoint end) noexcept -> bool {

    return unicodeContainsMappableCharacters(
        begin, end, [](const UnicodeDelta &delta) noexcept -> int32_t { return delta.lowercase; });
}

/// Test if a Unicode scalar range contains characters affected by simple uppercase mapping.
[[nodiscard]] inline auto unicodeContainsUppercaseMappableCharacters(
    const UnicodeData::CodePoint begin, const UnicodeData::CodePoint end) noexcept -> bool {

    return unicodeContainsMappableCharacters(
        begin, end, [](const UnicodeDelta &delta) noexcept -> int32_t { return delta.uppercase; });
}

}
