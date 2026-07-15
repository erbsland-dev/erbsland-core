// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CommonBoxFrameBlockCombinationStyle.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::cterm {

auto CommonBoxFrameBlockCombinationStyle::combine(const Block &current, const Block &overlay) const noexcept -> Block {
    auto result = overlay;
    const auto currentIndex = lookupCharacterIndex(current.first());
    const auto overlayIndex = lookupCharacterIndex(overlay.first());
    if (currentIndex != cUnsupportedIndex && overlayIndex != cUnsupportedIndex) {
        result = Block{combineCodePoint(currentIndex, overlayIndex)};
    }
    result.setStyle(result.style().withBase(current.style()).withOverlay(overlay.style()));
    return result;
}

auto CommonBoxFrameBlockCombinationStyle::attributeStroke(
    const AttributeData attributes, const std::size_t position) noexcept -> uint8_t {
    return static_cast<uint8_t>((attributes >> (position * cAttributeShift)) & cAttributeMask);
}

auto CommonBoxFrameBlockCombinationStyle::combineAttributes(
    const AttributeData current, const AttributeData overlay) noexcept -> AttributeData {
    auto result = AttributeData{0U};
    for (auto position = std::size_t{0U}; position < positionWeights().size(); ++position) {
        const auto currentStroke = attributeStroke(current, position);
        const auto overlayStroke = attributeStroke(overlay, position);
        const auto stroke = std::max(currentStroke, overlayStroke);
        result |= AttributeData{stroke} << (position * cAttributeShift);
    }
    return result;
}

auto CommonBoxFrameBlockCombinationStyle::hasLines(const AttributeData attributes) noexcept -> bool {
    for (auto position = std::size_t{0U}; position < cCenterPosition; ++position) {
        if (attributeStroke(attributes, position) != 0U) {
            return true;
        }
    }
    return false;
}

auto CommonBoxFrameBlockCombinationStyle::isCenterOnly(const AttributeData attributes) noexcept -> bool {
    return attributeStroke(attributes, cCenterPosition) != 0U && !hasLines(attributes);
}

auto CommonBoxFrameBlockCombinationStyle::scoreCandidate(
    const AttributeData ideal, const AttributeData candidate) noexcept -> uint32_t {
    auto missingScore = uint32_t{0U};
    auto distanceScore = uint32_t{0U};
    auto extraScore = uint32_t{0U};
    const auto weights = positionWeights();
    for (auto position = std::size_t{0U}; position < weights.size(); ++position) {
        const auto idealStroke = attributeStroke(ideal, position);
        const auto candidateStroke = attributeStroke(candidate, position);
        const auto weight = uint32_t{weights[position]};
        if (idealStroke != 0U && candidateStroke == 0U) {
            missingScore += weight;
        } else if (idealStroke == 0U && candidateStroke != 0U) {
            extraScore += weight * (2U + candidateStroke);
        } else if (idealStroke != 0U && candidateStroke != 0U) {
            const auto distance =
                idealStroke > candidateStroke ? idealStroke - candidateStroke : candidateStroke - idealStroke;
            distanceScore += weight * static_cast<uint32_t>(distance);
        }
    }
    return missingScore * 10'000U + distanceScore * 100U + extraScore;
}

auto CommonBoxFrameBlockCombinationStyle::lookupCharacterIndex(const text::Char codePoint) noexcept -> uint8_t {
    const auto rawCodePoint = codePoint.toRawValue();
    if (rawCodePoint >= cBoxDrawingStart && rawCodePoint <= cBoxDrawingEnd) {
        const auto offset = static_cast<std::size_t>(rawCodePoint - cBoxDrawingStart);
        return boxOffsetToCharacterIndex()[offset];
    }

    const auto codePoints = specialCodePoints();
    const auto indexes = specialCharacterIndexes();
    for (auto index = std::size_t{0U}; index < codePoints.size(); ++index) {
        if (rawCodePoint == char32_t{codePoints[index]}) {
            return indexes[index];
        }
    }
    return cUnsupportedIndex;
}

auto CommonBoxFrameBlockCombinationStyle::lookupExactCharacter(const AttributeData attributes) noexcept -> char32_t {
    const auto attributeTable = exactAttributes();
    auto lower = std::size_t{0U};
    auto upper = attributeTable.size();
    while (lower < upper) {
        const auto middle = lower + (upper - lower) / 2U;
        if (attributeTable[middle] < attributes) {
            lower = middle + 1U;
        } else {
            upper = middle;
        }
    }
    if (lower < attributeTable.size() && attributeTable[lower] == attributes) {
        return char32_t{exactCharacters()[lower]};
    }
    return U'\0';
}

auto CommonBoxFrameBlockCombinationStyle::bestCharacterIndex(const AttributeData ideal) noexcept -> std::size_t {
    auto bestIndex = std::size_t{0U};
    auto bestScore = std::numeric_limits<uint32_t>::max();
    const auto attributeTable = attributes();
    for (auto index = std::size_t{0U}; index < attributeTable.size(); ++index) {
        const auto score = scoreCandidate(ideal, attributeTable[index]);
        if (score < bestScore) {
            bestIndex = index;
            bestScore = score;
        }
    }
    return bestIndex;
}

auto CommonBoxFrameBlockCombinationStyle::combineCodePoint(
    const uint8_t currentIndex, const uint8_t overlayIndex) noexcept -> char32_t {
    const auto attributeTable = attributes();
    const auto characterTable = characters();
    const auto currentAttributes = attributeTable[currentIndex];
    const auto overlayAttributes = attributeTable[overlayIndex];
    if (isCenterOnly(currentAttributes) && hasLines(overlayAttributes)) {
        return char32_t{characterTable[overlayIndex]};
    }
    if (hasLines(currentAttributes) && isCenterOnly(overlayAttributes)) {
        return char32_t{characterTable[currentIndex]};
    }

    const auto ideal = combineAttributes(currentAttributes, overlayAttributes);
    if (const auto exactCharacter = lookupExactCharacter(ideal); exactCharacter != U'\0') {
        return exactCharacter;
    }

    const auto bestIndex = bestCharacterIndex(ideal);
    const auto bestScore = scoreCandidate(ideal, attributeTable[bestIndex]);
    const auto overlayScore = scoreCandidate(ideal, overlayAttributes);
    if (bestScore + cMinimumImprovement < overlayScore) {
        return char32_t{characterTable[bestIndex]};
    }
    return char32_t{characterTable[overlayIndex]};
}

}
