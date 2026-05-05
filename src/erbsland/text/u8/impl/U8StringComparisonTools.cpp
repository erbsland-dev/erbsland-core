// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringComparisonTools.hpp"

#include "U8Encoding.hpp"
#include "U8StringReadTools.hpp"

namespace erbsland::text::impl {

auto U8StringComparisonTools::containsOneDecodedCharacter(
    const std::span<const char> data, const CharacterSet &characters) -> bool {
    if (characters.isEmpty()) {
        return false;
    }

    auto result = false;
    utf8::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> bool {
        result = characters.contains(character);
        return !result;
    });
    return result;
}

auto U8StringComparisonTools::compare(const U8StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return compareDecodedSpans(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U8StringComparisonTools::find(const U8StringDataView &text, const CharCompareFn compareFn) const noexcept
    -> unit::ByteIndex {
    return find(text, unit::ByteIndex::zero(), compareFn);
}

auto U8StringComparisonTools::find(
    const U8StringDataView &text, const unit::ByteIndex start, const CharCompareFn compareFn) const noexcept
    -> unit::ByteIndex {
    if (start.isNoIndex()) {
        return unit::ByteIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() > data.size()) {
        return unit::ByteIndex::noIndex();
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
        utf8::fastAdvanceChar(data, position);
    }
    return unit::ByteIndex::noIndex();
}

auto U8StringComparisonTools::startsWith(const U8StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return startsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U8StringComparisonTools::startsWith(const Char character) const noexcept -> bool {
    return U8StringReadTools{_data}.charAt(unit::ByteIndex::zero()) == character;
}

auto U8StringComparisonTools::endsWith(const U8StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return endsWithDecodedSpan(_data.dataSpan(), other.dataSpan(), compareFn);
}

auto U8StringComparisonTools::endsWith(const Char character) const noexcept -> bool {
    auto index = unit::ByteIndex::end(U8StringReadTools{_data}.byteLength());
    return U8StringReadTools{_data}.retreat(index) && U8StringReadTools{_data}.charAt(index) == character;
}

auto U8StringComparisonTools::contains(const U8StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> bool {
    return !find(other, compareFn).isNoIndex();
}

auto U8StringComparisonTools::contains(const Char character) const noexcept -> bool {
    auto result = false;
    utf8::forEachDecodedCharacter(
        _data.dataSpan(), EncodingErrorMode::Replace, [&](const Char currentCharacter) -> bool {
            result = currentCharacter == character;
            return !result;
        });
    return result;
}

auto U8StringComparisonTools::count(const U8StringDataView &other, const CharCompareFn compareFn) const noexcept
    -> unit::ElementCount {
    const auto needle = other.dataSpan();
    if (needle.empty()) {
        return {};
    }

    const auto data = _data.dataSpan();
    auto result = unit::ElementCount{};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesDecodedSpan(data, position, needle, compareFn)) {
            ++result;
            position = endOfMatch(data, position, needle);
        } else {
            utf8::fastAdvanceChar(data, position);
        }
    }
    return result;
}

auto U8StringComparisonTools::count(const Char character) const noexcept -> unit::ElementCount {
    auto result = unit::ElementCount{};
    utf8::forEachDecodedCharacter(
        _data.dataSpan(), EncodingErrorMode::Replace, [&](const Char currentCharacter) -> bool {
            if (currentCharacter == character) {
                ++result;
            }
            return true;
        });
    return result;
}

auto U8StringComparisonTools::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return containsOneDecodedCharacter(_data.dataSpan(), characters);
}

auto U8StringComparisonTools::containsOnly(const CharSet &characters) const noexcept -> bool {
    auto result = true;
    utf8::forEachDecodedCharacter(
        _data.dataSpan(), EncodingErrorMode::Replace, [&](const Char currentCharacter) -> bool {
            result = characters.contains(currentCharacter);
            return result;
        });
    return result;
}

auto U8StringComparisonTools::compareDecodedSpans(
    const std::span<const char> left, const std::span<const char> right, const CharCompareFn compareFn) noexcept
    -> std::strong_ordering {
    auto leftPosition = unit::ByteIndex::zero();
    auto rightPosition = unit::ByteIndex::zero();

    while (leftPosition.toSizeT() < left.size() && rightPosition.toSizeT() < right.size()) {
        const auto leftCharacter = utf8::decodeCharOrReplace(left, leftPosition);
        const auto rightCharacter = utf8::decodeCharOrReplace(right, rightPosition);
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

auto U8StringComparisonTools::matchesDecodedSpan(
    const std::span<const char> haystack,
    const unit::ByteIndex candidateStart,
    const std::span<const char> needle,
    const CharCompareFn compareFn) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = unit::ByteIndex::zero();
    while (needlePosition.toSizeT() < needle.size()) {
        if (haystackPosition.toSizeT() >= haystack.size()) {
            return false;
        }
        const auto haystackCharacter = utf8::decodeCharOrReplace(haystack, haystackPosition);
        const auto needleCharacter = utf8::decodeCharOrReplace(needle, needlePosition);
        if (!charactersEqual(haystackCharacter, needleCharacter, compareFn)) {
            return false;
        }
    }
    return true;
}

auto U8StringComparisonTools::startsWithDecodedSpan(
    const std::span<const char> haystack, const std::span<const char> needle, const CharCompareFn compareFn) noexcept
    -> bool {
    return needle.empty() || matchesDecodedSpan(haystack, unit::ByteIndex::zero(), needle, compareFn);
}

auto U8StringComparisonTools::endsWithDecodedSpan(
    const std::span<const char> haystack, const std::span<const char> needle, const CharCompareFn compareFn) noexcept
    -> bool {
    if (needle.empty()) {
        return true;
    }

    auto haystackPosition = unit::ByteIndex::fromSizeT(haystack.size());
    auto needlePosition = unit::ByteIndex::fromSizeT(needle.size());
    while (!needlePosition.isZero()) {
        if (haystackPosition.isZero()) {
            return false;
        }
        utf8::fastRetreatChar(haystack, haystackPosition);
        utf8::fastRetreatChar(needle, needlePosition);
        auto haystackReadPosition = haystackPosition;
        auto needleReadPosition = needlePosition;
        if (!charactersEqual(
                utf8::decodeCharOrReplace(haystack, haystackReadPosition),
                utf8::decodeCharOrReplace(needle, needleReadPosition),
                compareFn)) {
            return false;
        }
    }
    return true;
}

auto U8StringComparisonTools::endOfMatch(
    const std::span<const char> haystack, unit::ByteIndex start, const std::span<const char> needle) noexcept
    -> unit::ByteIndex {
    auto needlePosition = unit::ByteIndex::zero();
    while (needlePosition.toSizeT() < needle.size() && start.toSizeT() < haystack.size()) {
        utf8::fastAdvanceChar(haystack, start);
        utf8::fastAdvanceChar(needle, needlePosition);
    }
    return start;
}

auto U8StringComparisonTools::charactersEqual(const Char left, const Char right, const CharCompareFn compareFn) noexcept
    -> bool {
    if (compareFn == nullptr) {
        return left == right;
    }
    return compareFn(left, right) == std::strong_ordering::equal;
}

}
