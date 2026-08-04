// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringComparisonTools.hpp"

#include "U32Encoding.hpp"
#include "U32StringReadTools.hpp"

#include "../../impl/DecodedStringSearch.hpp"

namespace erbsland::text::impl {

using unit::CpIndex;
using unit::ItemCount;

auto U32StringComparisonTools::containsOneDecodedCharacter(
    const std::span<const char32_t> data, const CharacterSet &characters) -> bool {
    if (characters.isEmpty()) {
        return false;
    }

    auto result = false;
    utf32::forEachDecodedCharacter(data, EncodingMode::Tolerant, [&](const Char character) -> bool {
        result = characters.contains(character);
        return !result;
    });
    return result;
}

auto U32StringComparisonTools::compare(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return compareDecodedSpans(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U32StringComparisonTools::find(const U32StringDataView &text, const CharCompareFn compareFn) const noexcept
    -> CpIndex {
    return find(text, CpIndex::zero(), compareFn);
}

auto U32StringComparisonTools::find(
    const U32StringDataView &text, const CpIndex start, const CharCompareFn compareFn) const noexcept -> CpIndex {
    if (start.isNoIndex()) {
        return CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() > data.size()) {
        return CpIndex::noIndex();
    }

    const auto needle = text.dataSpan();
    if (needle.empty()) {
        return start;
    }

    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        try {
            const auto readNeedle = [&needle](CpIndex &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf32::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](CpIndex &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf32::decodeCharOrReplace(data, position);
            };
            return DecodedStringSearch{needle.size(), CpIndex::zero(), readNeedle, compareFn}.find(start, readData);
        } catch (...) {
            // Preserve the noexcept search API by falling back to the allocation-free implementation.
        }
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        if (matchesDecodedSpan(data, position, needle, compareFn)) {
            return position;
        }
        utf32::fastAdvanceChar(data, position);
    }
    return CpIndex::noIndex();
}

auto U32StringComparisonTools::startsWith(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return startsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U32StringComparisonTools::startsWith(const Char character) const noexcept -> bool {
    return U32StringReadTools{_data}.charAt(CpIndex::zero()) == character;
}

auto U32StringComparisonTools::endsWith(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return endsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U32StringComparisonTools::endsWith(const Char character) const noexcept -> bool {
    auto index = CpIndex::end(U32StringReadTools{_data}.length());
    return U32StringReadTools{_data}.retreat(index) && U32StringReadTools{_data}.charAt(index) == character;
}

auto U32StringComparisonTools::contains(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return !find(other, compareFn).isNoIndex();
}

auto U32StringComparisonTools::contains(const Char character) const noexcept -> bool {
    auto result = false;
    utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char currentCharacter) -> bool {
        result = currentCharacter == character;
        return !result;
    });
    return result;
}

auto U32StringComparisonTools::count(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> ItemCount {
    const auto needle = other.dataSpan();
    if (needle.empty()) {
        return {};
    }

    const auto data = _data.dataSpan();
    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        try {
            const auto readNeedle = [&needle](CpIndex &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf32::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](CpIndex &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf32::decodeCharOrReplace(data, position);
            };
            return DecodedStringSearch{needle.size(), CpIndex::zero(), readNeedle, compareFn}.count(
                CpIndex::zero(), readData);
        } catch (...) {
            // Preserve the noexcept search API by falling back to the allocation-free implementation.
        }
    }

    auto result = ItemCount{};
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesDecodedSpan(data, position, needle, compareFn)) {
            ++result;
            position = endOfMatch(data, position, needle);
        } else {
            utf32::fastAdvanceChar(data, position);
        }
    }
    return result;
}

auto U32StringComparisonTools::count(const Char character) const noexcept -> ItemCount {
    auto result = ItemCount{};
    utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char currentCharacter) -> bool {
        if (currentCharacter == character) {
            ++result;
        }
        return true;
    });
    return result;
}

auto U32StringComparisonTools::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return containsOneDecodedCharacter(_data.dataSpan(), characters);
}

auto U32StringComparisonTools::containsOnly(const CharSet &characters) const noexcept -> bool {
    auto result = true;
    utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Tolerant, [&](const Char currentCharacter) -> bool {
        result = characters.contains(currentCharacter);
        return result;
    });
    return result;
}

auto U32StringComparisonTools::compareDecodedSpans(
    const std::span<const char32_t> left, const std::span<const char32_t> right, const CharCompareFn compareFn) noexcept
    -> std::strong_ordering {
    auto leftPosition = CpIndex::zero();
    auto rightPosition = CpIndex::zero();

    while (leftPosition.toSizeT() < left.size() && rightPosition.toSizeT() < right.size()) {
        const auto leftCharacter = utf32::decodeCharOrReplace(left, leftPosition);
        const auto rightCharacter = utf32::decodeCharOrReplace(right, rightPosition);
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

auto U32StringComparisonTools::matchesDecodedSpan(
    const std::span<const char32_t> haystack,
    const CpIndex candidateStart,
    const std::span<const char32_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = CpIndex::zero();
    while (needlePosition.toSizeT() < needle.size()) {
        if (haystackPosition.toSizeT() >= haystack.size()) {
            return false;
        }
        const auto haystackCharacter = utf32::decodeCharOrReplace(haystack, haystackPosition);
        const auto needleCharacter = utf32::decodeCharOrReplace(needle, needlePosition);
        if (!charactersEqual(haystackCharacter, needleCharacter, compareFn)) {
            return false;
        }
    }
    return true;
}

auto U32StringComparisonTools::startsWithDecodedSpan(
    const std::span<const char32_t> haystack,
    const std::span<const char32_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    return needle.empty() || matchesDecodedSpan(haystack, CpIndex::zero(), needle, compareFn);
}

auto U32StringComparisonTools::endsWithDecodedSpan(
    const std::span<const char32_t> haystack,
    const std::span<const char32_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    if (needle.empty()) {
        return true;
    }

    auto haystackPosition = CpIndex::fromSizeT(haystack.size());
    auto needlePosition = CpIndex::fromSizeT(needle.size());
    while (!needlePosition.isZero()) {
        if (haystackPosition.isZero()) {
            return false;
        }
        utf32::fastRetreatChar(haystack, haystackPosition);
        utf32::fastRetreatChar(needle, needlePosition);
        auto haystackReadPosition = haystackPosition;
        auto needleReadPosition = needlePosition;
        if (!charactersEqual(
                utf32::decodeCharOrReplace(haystack, haystackReadPosition),
                utf32::decodeCharOrReplace(needle, needleReadPosition),
                compareFn)) {
            return false;
        }
    }
    return true;
}

auto U32StringComparisonTools::endOfMatch(
    const std::span<const char32_t> haystack, CpIndex start, const std::span<const char32_t> needle) noexcept
    -> CpIndex {
    auto needlePosition = CpIndex::zero();
    while (needlePosition.toSizeT() < needle.size() && start.toSizeT() < haystack.size()) {
        utf32::fastAdvanceChar(haystack, start);
        utf32::fastAdvanceChar(needle, needlePosition);
    }
    return start;
}

auto U32StringComparisonTools::charactersEqual(
    const Char left, const Char right, const CharCompareFn compareFn) noexcept -> bool {
    if (compareFn == nullptr) {
        return left == right;
    }
    return compareFn(left, right) == std::strong_ordering::equal;
}

}
