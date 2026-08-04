// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockCombinationStyle.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cterm::impl {

/// Combines common Unicode box-frame characters through compact stroke attributes.
/// @tested{BufferTest BufferConvenienceTest}
class CommonBoxFrameBlockCombinationStyle final : public BlockCombinationStyle {
private:
    /// Store one encoded stroke-attribute record.
    using AttributeData = uint32_t;
    /// Store one Unicode code point in compact table data.
    using CodePointData = uint16_t;

public: // implement BlockCombinationStyle
    [[nodiscard]] auto combine(const Block &current, const Block &overlay) const noexcept -> Block override;

private:
    /// Map box offsets to character-table indexes.
    [[nodiscard]] static auto boxOffsetToCharacterIndex() noexcept -> std::span<const uint8_t>;
    /// Access the box-frame character table.
    [[nodiscard]] static auto characters() noexcept -> std::span<const CodePointData>;
    /// Access the stroke-attribute table.
    [[nodiscard]] static auto attributes() noexcept -> std::span<const AttributeData>;
    /// Access exact-match stroke attributes.
    [[nodiscard]] static auto exactAttributes() noexcept -> std::span<const AttributeData>;
    /// Access characters for exact stroke matches.
    [[nodiscard]] static auto exactCharacters() noexcept -> std::span<const CodePointData>;
    /// Access the stroke-position weights.
    [[nodiscard]] static auto positionWeights() noexcept -> std::span<const uint8_t>;
    /// Access special box-drawing code points.
    [[nodiscard]] static auto specialCodePoints() noexcept -> std::span<const CodePointData>;
    /// Access indexes for special box-drawing characters.
    [[nodiscard]] static auto specialCharacterIndexes() noexcept -> std::span<const uint8_t>;

private:
    /// Extract the stroke at one `position`.
    [[nodiscard]] static auto attributeStroke(AttributeData attributes, std::size_t position) noexcept -> uint8_t;
    /// Merge current and overlay stroke attributes.
    [[nodiscard]] static auto combineAttributes(AttributeData current, AttributeData overlay) noexcept -> AttributeData;
    /// Test whether an attribute record contains a line.
    [[nodiscard]] static auto hasLines(AttributeData attributes) noexcept -> bool;
    /// Test whether an attribute record contains only a center marker.
    [[nodiscard]] static auto isCenterOnly(AttributeData attributes) noexcept -> bool;
    /// Score how well a candidate represents the ideal stroke attributes.
    [[nodiscard]] static auto scoreCandidate(AttributeData ideal, AttributeData candidate) noexcept -> uint32_t;
    /// Look up the compact index for a box-drawing code point.
    [[nodiscard]] static auto lookupCharacterIndex(text::Char codePoint) noexcept -> uint8_t;
    /// Look up an exact character for stroke attributes.
    [[nodiscard]] static auto lookupExactCharacter(AttributeData attributes) noexcept -> char32_t;
    /// Choose the best compact character index for stroke attributes.
    [[nodiscard]] static auto bestCharacterIndex(AttributeData ideal) noexcept -> std::size_t;
    /// Combine two compact character indexes.
    [[nodiscard]] static auto combineCodePoint(uint8_t currentIndex, uint8_t overlayIndex) noexcept -> char32_t;

private:
    static constexpr auto cUnsupportedIndex = uint8_t{0xFFU};
    static constexpr auto cAttributeMask = AttributeData{0x0FU};
    static constexpr auto cAttributeShift = std::size_t{4U};
    static constexpr auto cMinimumImprovement = uint32_t{100U};
    static constexpr auto cBoxDrawingStart = char32_t{0x2500U};
    static constexpr auto cBoxDrawingEnd = char32_t{0x257FU};
    static constexpr auto cCenterPosition = std::size_t{6U};
};

}
