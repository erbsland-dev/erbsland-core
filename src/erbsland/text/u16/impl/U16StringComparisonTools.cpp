// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringComparisonTools.hpp"

#include "U16Encoding.hpp"
#include "U16StringReadTools.hpp"

#include "../../impl/DecodedStringSearch.hpp"

namespace erbsland::text::impl {

using unit::ItemCount;
using unit::U16DataIndex;

auto U16StringComparisonTools::containsOneDecodedCharacter(
    const std::span<const char16_t> data, const CharacterSet &characters) -> bool {
    if (characters.isEmpty()) {
        return false;
    }

    auto result = false;
    utf16::forEachDecodedCharacter(data, EncodingMode::Tolerant, [&](const Char character) -> bool {
        result = characters.contains(character);
        return !result;
    });
    return result;
}

auto U16StringComparisonTools::compare(const U16StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return compareDecodedSpans(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U16StringComparisonTools::find(const U16StringDataView &text, const CharCompareFn compareFn) const noexcept
    -> U16DataIndex {
    return find(text, U16DataIndex::zero(), compareFn);
}

auto U16StringComparisonTools::find(
    const U16StringDataView &text, const U16DataIndex start, const CharCompareFn compareFn) const noexcept
    -> U16DataIndex {
    if (start.isNoIndex()) {
        return U16DataIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() > data.size()) {
        return U16DataIndex::noIndex();
    }

    const auto needle = text.dataSpan();
    if (needle.empty()) {
        return start;
    }

    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        try {
            const auto readNeedle = [&needle](U16DataIndex &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf16::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](U16DataIndex &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf16::decodeCharOrReplace(data, position);
            };
            return DecodedStringSearch{needle.size(), U16DataIndex::zero(), readNeedle, compareFn}.find(
                start, readData);
        } catch (...) {
            // Preserve the noexcept search API by falling back to the allocation-free implementation.
        }
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        if (matchesDecodedSpan(data, position, needle, compareFn)) {
            return position;
        }
        utf16::fastAdvanceChar(data, position);
    }
    return U16DataIndex::noIndex();
}

auto U16StringComparisonTools::startsWith(const U16StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return startsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U16StringComparisonTools::startsWith(const Char character) const noexcept -> bool {
    return U16StringReadTools{_data}.charAt(U16DataIndex::zero()) == character;
}

auto U16StringComparisonTools::endsWith(const U16StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return endsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U16StringComparisonTools::endsWith(const Char character) const noexcept -> bool {
    auto index = U16DataIndex::end(U16StringReadTools{_data}.byteLength());
    return U16StringReadTools{_data}.retreat(index) && U16StringReadTools{_data}.charAt(index) == character;
}

auto U16StringComparisonTools::contains(const U16StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return !find(other, compareFn).isNoIndex();
}

auto U16StringComparisonTools::contains(const Char character) const noexcept -> bool {
    auto result = false;
    utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char currentCharacter) -> bool {
        result = currentCharacter == character;
        return !result;
    });
    return result;
}

auto U16StringComparisonTools::count(const U16StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> ItemCount {
    const auto needle = other.dataSpan();
    if (needle.empty()) {
        return {};
    }

    const auto data = _data.dataSpan();
    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        try {
            const auto readNeedle = [&needle](U16DataIndex &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf16::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](U16DataIndex &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf16::decodeCharOrReplace(data, position);
            };
            return DecodedStringSearch{needle.size(), U16DataIndex::zero(), readNeedle, compareFn}.count(
                U16DataIndex::zero(), readData);
        } catch (...) {
            // Preserve the noexcept search API by falling back to the allocation-free implementation.
        }
    }

    auto result = ItemCount{};
    auto position = U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesDecodedSpan(data, position, needle, compareFn)) {
            ++result;
            position = endOfMatch(data, position, needle);
        } else {
            utf16::fastAdvanceChar(data, position);
        }
    }
    return result;
}

auto U16StringComparisonTools::count(const Char character) const noexcept -> ItemCount {
    auto result = ItemCount{};
    utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char currentCharacter) -> bool {
        if (currentCharacter == character) {
            ++result;
        }
        return true;
    });
    return result;
}

auto U16StringComparisonTools::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return containsOneDecodedCharacter(_data.dataSpan(), characters);
}

auto U16StringComparisonTools::containsOnly(const CharSet &characters) const noexcept -> bool {
    auto result = true;
    utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char currentCharacter) -> bool {
        result = characters.contains(currentCharacter);
        return result;
    });
    return result;
}

auto U16StringComparisonTools::containsOnly(const AsciiCategory category) const noexcept -> bool {
    auto result = true;
    utf16::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char character) -> bool {
        result = character.isAsciiCategory(category);
        return result;
    });
    return result;
}

auto U16StringComparisonTools::compareDecodedSpans(
    const std::span<const char16_t> left, const std::span<const char16_t> right, const CharCompareFn compareFn) noexcept
    -> std::strong_ordering {
    auto leftPosition = U16DataIndex::zero();
    auto rightPosition = U16DataIndex::zero();

    while (leftPosition.toSizeT() < left.size() && rightPosition.toSizeT() < right.size()) {
        const auto leftCharacter = utf16::decodeCharOrReplace(left, leftPosition);
        const auto rightCharacter = utf16::decodeCharOrReplace(right, rightPosition);
        const auto result =
            compareFn == nullptr ? (leftCharacter <=> rightCharacter) : compareFn(leftCharacter, rightCharacter);
        if (result != std::strong_ordering::equal) {
            return result;
        }
    }
    if (leftPosition.toSizeT() < left.size()) {
        return std::strong_ordering::greater;
    }
    if (rightPosition.toSizeT() < right.size()) {
        return std::strong_ordering::less;
    }
    return std::strong_ordering::equal;
}

auto U16StringComparisonTools::matchesDecodedSpan(
    const std::span<const char16_t> haystack,
    const U16DataIndex candidateStart,
    const std::span<const char16_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = U16DataIndex::zero();
    while (needlePosition.toSizeT() < needle.size()) {
        if (haystackPosition.toSizeT() >= haystack.size()) {
            return false;
        }
        const auto haystackCharacter = utf16::decodeCharOrReplace(haystack, haystackPosition);
        const auto needleCharacter = utf16::decodeCharOrReplace(needle, needlePosition);
        if (!charactersEqual(haystackCharacter, needleCharacter, compareFn)) {
            return false;
        }
    }
    return true;
}

auto U16StringComparisonTools::startsWithDecodedSpan(
    const std::span<const char16_t> haystack,
    const std::span<const char16_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    return needle.empty() || matchesDecodedSpan(haystack, U16DataIndex::zero(), needle, compareFn);
}

auto U16StringComparisonTools::endsWithDecodedSpan(
    const std::span<const char16_t> haystack,
    const std::span<const char16_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    if (needle.empty()) {
        return true;
    }

    auto haystackPosition = U16DataIndex::fromSizeT(haystack.size());
    auto needlePosition = U16DataIndex::fromSizeT(needle.size());
    while (!needlePosition.isZero()) {
        if (haystackPosition.isZero()) {
            return false;
        }
        utf16::fastRetreatChar(haystack, haystackPosition);
        utf16::fastRetreatChar(needle, needlePosition);
        auto haystackReadPosition = haystackPosition;
        auto needleReadPosition = needlePosition;
        if (!charactersEqual(
                utf16::decodeCharOrReplace(haystack, haystackReadPosition),
                utf16::decodeCharOrReplace(needle, needleReadPosition),
                compareFn)) {
            return false;
        }
    }
    return true;
}

auto U16StringComparisonTools::endOfMatch(
    const std::span<const char16_t> haystack, U16DataIndex start, const std::span<const char16_t> needle) noexcept
    -> U16DataIndex {
    auto needlePosition = U16DataIndex::zero();
    while (needlePosition.toSizeT() < needle.size() && start.toSizeT() < haystack.size()) {
        utf16::fastAdvanceChar(haystack, start);
        utf16::fastAdvanceChar(needle, needlePosition);
    }
    return start;
}

auto U16StringComparisonTools::charactersEqual(
    const Char left, const Char right, const CharCompareFn compareFn) noexcept -> bool {
    if (compareFn == nullptr) {
        return left == right;
    }
    return compareFn(left, right) == std::strong_ordering::equal;
}

}
