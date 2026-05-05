// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockCombinationStyle.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cterm {

/// Combines common Unicode box-frame characters through compact stroke attributes.
class CommonBoxFrameBlockCombinationStyle final : public BlockCombinationStyle {
private:
    using AttributeData = uint32_t;
    using CodePointData = uint16_t;

public: // implement BlockCombinationStyle
    [[nodiscard]] auto combine(const Block &current, const Block &overlay) const noexcept -> Block override;

private:
    [[nodiscard]] static auto boxOffsetToCharacterIndex() noexcept -> std::span<const uint8_t>;
    [[nodiscard]] static auto characters() noexcept -> std::span<const CodePointData>;
    [[nodiscard]] static auto attributes() noexcept -> std::span<const AttributeData>;
    [[nodiscard]] static auto exactAttributes() noexcept -> std::span<const AttributeData>;
    [[nodiscard]] static auto exactCharacters() noexcept -> std::span<const CodePointData>;
    [[nodiscard]] static auto positionWeights() noexcept -> std::span<const uint8_t>;
    [[nodiscard]] static auto specialCodePoints() noexcept -> std::span<const CodePointData>;
    [[nodiscard]] static auto specialCharacterIndexes() noexcept -> std::span<const uint8_t>;

private:
    [[nodiscard]] static auto attributeStroke(AttributeData attributes, std::size_t position) noexcept -> uint8_t;
    [[nodiscard]] static auto combineAttributes(AttributeData current, AttributeData overlay) noexcept -> AttributeData;
    [[nodiscard]] static auto hasLines(AttributeData attributes) noexcept -> bool;
    [[nodiscard]] static auto isCenterOnly(AttributeData attributes) noexcept -> bool;
    [[nodiscard]] static auto scoreCandidate(AttributeData ideal, AttributeData candidate) noexcept -> uint32_t;
    [[nodiscard]] static auto lookupCharacterIndex(text::Char codePoint) noexcept -> uint8_t;
    [[nodiscard]] static auto lookupExactCharacter(AttributeData attributes) noexcept -> char32_t;
    [[nodiscard]] static auto bestCharacterIndex(AttributeData ideal) noexcept -> std::size_t;
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
