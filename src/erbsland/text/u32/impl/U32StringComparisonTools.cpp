// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringComparisonTools.hpp"

#include "U32Encoding.hpp"
#include "U32StringReadTools.hpp"

namespace erbsland::text::impl {

auto U32StringComparisonTools::containsOneDecodedCharacter(
    const std::span<const char32_t> data, const CharacterSet &characters) -> bool {
    if (characters.isEmpty()) {
        return false;
    }

    auto result = false;
    utf32::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> bool {
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
    -> unit::CpIndex {
    return find(text, unit::CpIndex::zero(), compareFn);
}

auto U32StringComparisonTools::find(
    const U32StringDataView &text, const unit::CpIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::CpIndex {
    if (start.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() > data.size()) {
        return unit::CpIndex::noIndex();
    }

    const auto needle = text.dataSpan();
    if (needle.empty()) {
        return start;
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        if (matchesDecodedSpan(data, position, needle, compareFn)) {
            return position;
        }
        utf32::fastAdvanceChar(data, position);
    }
    return unit::CpIndex::noIndex();
}

auto U32StringComparisonTools::startsWith(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return startsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U32StringComparisonTools::startsWith(const Char character) const noexcept -> bool {
    return U32StringReadTools{_data}.charAt(unit::CpIndex::zero()) == character;
}

auto U32StringComparisonTools::endsWith(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return endsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U32StringComparisonTools::endsWith(const Char character) const noexcept -> bool {
    auto index = unit::CpIndex::end(U32StringReadTools{_data}.length());
    return U32StringReadTools{_data}.retreat(index) && U32StringReadTools{_data}.charAt(index) == character;
}

auto U32StringComparisonTools::contains(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return !find(other, compareFn).isNoIndex();
}

auto U32StringComparisonTools::contains(const Char character) const noexcept -> bool {
    auto result = false;
    utf32::forEachDecodedCharacter(
        _data.dataSpan(), EncodingErrorMode::Replace, [&](const Char currentCharacter) -> bool {
            result = currentCharacter == character;
            return !result;
        });
    return result;
}

auto U32StringComparisonTools::count(const U32StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> unit::ElementCount {
    const auto needle = other.dataSpan();
    if (needle.empty()) {
        return {};
    }

    const auto data = _data.dataSpan();
    auto result = unit::ElementCount{};
    auto position = unit::CpIndex::zero();
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

auto U32StringComparisonTools::count(const Char character) const noexcept -> unit::ElementCount {
    auto result = unit::ElementCount{};
    utf32::forEachDecodedCharacter(
        _data.dataSpan(), EncodingErrorMode::Replace, [&](const Char currentCharacter) -> bool {
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
    utf32::forEachDecodedCharacter(
        _data.dataSpan(), EncodingErrorMode::Replace, [&](const Char currentCharacter) -> bool {
            result = characters.contains(currentCharacter);
            return result;
        });
    return result;
}

auto U32StringComparisonTools::compareDecodedSpans(
    const std::span<const char32_t> left, const std::span<const char32_t> right, const CharCompareFn compareFn) noexcept
    -> std::strong_ordering {
    auto leftPosition = unit::CpIndex::zero();
    auto rightPosition = unit::CpIndex::zero();

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
    const unit::CpIndex candidateStart,
    const std::span<const char32_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = unit::CpIndex::zero();
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
    return needle.empty() || matchesDecodedSpan(haystack, unit::CpIndex::zero(), needle, compareFn);
}

auto U32StringComparisonTools::endsWithDecodedSpan(
    const std::span<const char32_t> haystack,
    const std::span<const char32_t> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    if (needle.empty()) {
        return true;
    }

    auto haystackPosition = unit::CpIndex::fromSizeT(haystack.size());
    auto needlePosition = unit::CpIndex::fromSizeT(needle.size());
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
    const std::span<const char32_t> haystack, unit::CpIndex start, const std::span<const char32_t> needle) noexcept
    -> unit::CpIndex {
    auto needlePosition = unit::CpIndex::zero();
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
