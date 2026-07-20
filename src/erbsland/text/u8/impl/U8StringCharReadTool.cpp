// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringCharReadTool.hpp"

#include "U8Encoding.hpp"

#include "../../impl/ThrowHelper.hpp"

#include <algorithm>

namespace erbsland::text::impl {

using namespace unit;

auto U8StringCharReadTool::charLength() const noexcept -> CpLength {
    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    auto result = CpLength::zero();
    while (position.toSizeT() < data.size()) {
        utf8::fastAdvanceChar(data, position);
        ++result;
    }
    return result;
}

auto U8StringCharReadTool::charAt(const CpIndex index) const noexcept -> Char {
    if (index.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return Char{utf8::decodeCharOrReplace(data, position)};
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? Char::endOfData() : Char::noCodePoint();
}

auto U8StringCharReadTool::charAtOrThrow(const CpIndex index) const -> Char {
    if (index.isNoIndex()) {
        throwOutOfRange("Read position out of range");
    }
    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return utf8::decodeCharOrThrow(data, position);
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    throwOutOfRange("Read position out of range");
}

auto U8StringCharReadTool::byteIndexAt(const CpIndex index) const noexcept -> ByteIndex {
    if (index.isNoIndex()) {
        return ByteIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return position;
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? ByteIndex::fromSizeT(data.size()) : ByteIndex::noIndex();
}

auto U8StringCharReadTool::charIndexAt(const ByteIndex index) const noexcept -> CpIndex {
    if (index.isNoIndex()) {
        return CpIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    if (index.toSizeT() > data.size()) {
        return CpIndex::noIndex();
    }

    auto position = ByteIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        utf8::fastAdvanceChar(data, position);
        if (index >= characterStart && index < position) {
            return currentIndex;
        }
        ++currentIndex;
    }
    return currentIndex;
}

auto U8StringCharReadTool::sliceRange(const CpRange range) const noexcept -> ByteRange {
    if (!_data.range().isValid() || !range.isValid() || range.isEmpty()) {
        return ByteRange::empty();
    }

    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    auto currentIndex = CpIndex::zero();
    while (position.toSizeT() < data.size() && currentIndex < range.index()) {
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    if (currentIndex < range.index() || position.toSizeT() >= data.size()) {
        return ByteRange::empty();
    }

    const auto sliceStart = position;
    auto remainingLength = range.length();
    while (position.toSizeT() < data.size() && (remainingLength.isInfinite() || !remainingLength.isZero())) {
        utf8::fastAdvanceChar(data, position);
        if (!remainingLength.isInfinite()) {
            --remainingLength;
        }
    }

    const auto sliceLength = ByteLength::fromSizeT(position.toSizeT() - sliceStart.toSizeT());
    return ByteRange{sliceStart, sliceLength}.withOrigin(_data.range().index());
}

auto U8StringCharReadTool::findFirstOf(const CharSet &characters) const noexcept -> CpIndex {
    return findFirstOf(characters, CpIndex::zero());
}

auto U8StringCharReadTool::findFirstOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U8StringCharReadTool::findFirstNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return findFirstNotOf(characters, CpIndex::zero());
}

auto U8StringCharReadTool::findFirstNotOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U8StringCharReadTool::findLastOf(const CharSet &characters) const noexcept -> CpIndex {
    return findLastOf(characters, CpIndex::end(charLength()));
}

auto U8StringCharReadTool::findLastOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U8StringCharReadTool::findLastNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return findLastNotOf(characters, CpIndex::end(charLength()));
}

auto U8StringCharReadTool::findLastNotOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U8StringCharReadTool::find(const U8StringDataView &text) const noexcept -> CpIndex {
    return find(text, CpIndex::zero());
}

auto U8StringCharReadTool::find(const U8StringDataView &text, const CpIndex start) const noexcept -> CpIndex {
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
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return CpIndex::noIndex();
}

auto U8StringCharReadTool::findFirstOfCharacterSet(
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
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
        ++currentIndex;
    }
    return CpIndex::noIndex();
}

auto U8StringCharReadTool::findLastOfCharacterSet(
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
        utf8::fastRetreatChar(data, position);
        --currentIndex;
        auto readPosition = position;
        const auto character = utf8::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
    }
    return CpIndex::noIndex();
}

auto U8StringCharReadTool::matchesCharacterSequence(
    const std::span<const char> haystack, const ByteIndex candidateStart, const std::span<const char> needle) noexcept
    -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = ByteIndex::zero();
    while (needlePosition.toSizeT() < needle.size()) {
        if (haystackPosition.toSizeT() >= haystack.size()) {
            return false;
        }
        const auto haystackCharacter = utf8::decodeCharOrReplace(haystack, haystackPosition);
        const auto needleCharacter = utf8::decodeCharOrReplace(needle, needlePosition);
        if (haystackCharacter != needleCharacter) {
            return false;
        }
    }
    return true;
}

}
