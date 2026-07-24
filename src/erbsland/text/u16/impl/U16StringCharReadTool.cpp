// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringCharReadTool.hpp"

#include "U16Encoding.hpp"

#include <algorithm>

namespace erbsland::text::impl {

using namespace unit;

auto U16StringCharReadTool::charLength() const noexcept -> CpLength {
    const auto data = _data.dataSpan();
    auto position = U16DataIndex::zero();
    auto result = CpLength::zero();
    while (position.toSizeT() < data.size()) {
        utf16::fastAdvanceChar(data, position);
        ++result;
    }
    return result;
}

auto U16StringCharReadTool::charAt(const CpIndex index) const noexcept -> Char {
    if (index.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    auto position = U16DataIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return Char{utf16::decodeCharOrReplace(data, position)};
        }
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? Char::endOfData() : Char::noCodePoint();
}

auto U16StringCharReadTool::byteIndexAt(const CpIndex index) const noexcept -> U16DataIndex {
    if (index.isNoIndex()) {
        return U16DataIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    auto position = U16DataIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return position;
        }
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? U16DataIndex::fromSizeT(data.size()) : U16DataIndex::noIndex();
}

auto U16StringCharReadTool::charIndexAt(const U16DataIndex index) const noexcept -> CpIndex {
    if (index.isNoIndex()) {
        return CpIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    if (index.toSizeT() > data.size()) {
        return CpIndex::noIndex();
    }

    auto position = U16DataIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        utf16::fastAdvanceChar(data, position);
        if (index >= characterStart && index < position) {
            return currentIndex;
        }
        ++currentIndex;
    }
    return currentIndex;
}

auto U16StringCharReadTool::sliceRange(const CpRange range) const noexcept -> U16DataRange {
    if (!_data.range().isValid() || !range.isValid() || range.isEmpty()) {
        return U16DataRange::empty();
    }

    const auto data = _data.dataSpan();
    auto position = U16DataIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size() && currentIndex < range.index()) {
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    if (currentIndex < range.index() || position.toSizeT() >= data.size()) {
        return U16DataRange::empty();
    }

    const auto sliceStart = position;
    auto remainingLength = range.length();
    while (position.toSizeT() < data.size() && (remainingLength.isInfinite() || !remainingLength.isZero())) {
        utf16::fastAdvanceChar(data, position);
        if (!remainingLength.isInfinite()) {
            --remainingLength;
        }
    }

    const auto sliceLength = U16DataLength::fromSizeT(position.toSizeT() - sliceStart.toSizeT());
    return U16DataRange{sliceStart, sliceLength}.withOrigin(_data.range().index());
}

auto U16StringCharReadTool::findFirstOf(const CharSet &characters) const noexcept -> CpIndex {
    return findFirstOf(characters, CpIndex::zero());
}

auto U16StringCharReadTool::findFirstOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U16StringCharReadTool::findFirstNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return findFirstNotOf(characters, CpIndex::zero());
}

auto U16StringCharReadTool::findFirstNotOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U16StringCharReadTool::findLastOf(const CharSet &characters) const noexcept -> CpIndex {
    return findLastOf(characters, CpIndex::end(charLength()));
}

auto U16StringCharReadTool::findLastOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U16StringCharReadTool::findLastNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return findLastNotOf(characters, CpIndex::end(charLength()));
}

auto U16StringCharReadTool::findLastNotOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U16StringCharReadTool::find(const U16StringDataView &text) const noexcept -> CpIndex {
    return find(text, CpIndex::zero());
}

auto U16StringCharReadTool::find(const U16StringDataView &text, const CpIndex start) const noexcept -> CpIndex {
    if (start.isNoIndex()) {
        return CpIndex::noIndex();
    }

    const auto needle = text.dataSpan();
    auto position = byteIndexAt(start);
    if (position.isNoIndex()) {
        return CpIndex::noIndex();
    }
    if (needle.empty()) {
        return start;
    }

    const auto data = _data.dataSpan();
    if (position.toSizeT() >= data.size()) {
        return CpIndex::noIndex();
    }

    auto currentIndex = start;
    while (position.toSizeT() < data.size()) {
        if (matchesCharacterSequence(data, position, needle)) {
            return currentIndex;
        }
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return CpIndex::noIndex();
}

auto U16StringCharReadTool::findFirstOfCharacterSet(
    const CharacterSet &characters, const CpIndex start, const bool isMatching) const noexcept -> CpIndex {
    if (start.isNoIndex()) {
        return CpIndex::noIndex();
    }

    auto position = byteIndexAt(start);
    if (position.isNoIndex()) {
        return CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (position.toSizeT() >= data.size()) {
        return CpIndex::noIndex();
    }

    auto currentIndex = start;
    while (position.toSizeT() < data.size()) {
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
        ++currentIndex;
    }
    return CpIndex::noIndex();
}

auto U16StringCharReadTool::findLastOfCharacterSet(
    const CharacterSet &characters, const CpIndex end, const bool isMatching) const noexcept -> CpIndex {
    if (end.isNoIndex() || end.isZero()) {
        return CpIndex::noIndex();
    }

    auto position = byteIndexAt(end);
    if (position.isNoIndex()) {
        return CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || position.isZero()) {
        return CpIndex::noIndex();
    }

    auto currentIndex = end;
    while (!position.isZero()) {
        utf16::fastRetreatChar(data, position);
        --currentIndex;
        auto readPosition = position;
        const auto character = utf16::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
    }
    return CpIndex::noIndex();
}

auto U16StringCharReadTool::matchesCharacterSequence(
    const std::span<const char16_t> haystack,
    const U16DataIndex candidateStart,
    const std::span<const char16_t> needle) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = U16DataIndex::zero();
    while (needlePosition.toSizeT() < needle.size()) {
        if (haystackPosition.toSizeT() >= haystack.size()) {
            return false;
        }
        const auto haystackCharacter = utf16::decodeCharOrReplace(haystack, haystackPosition);
        const auto needleCharacter = utf16::decodeCharOrReplace(needle, needlePosition);
        if (haystackCharacter != needleCharacter) {
            return false;
        }
    }
    return true;
}

}
