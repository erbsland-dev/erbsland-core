// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringCharReadTool.hpp"

#include "U8Encoding.hpp"

#include "../../../err/ThrowHelper.hpp"

#include <algorithm>

namespace erbsland::text::impl {

auto U8StringCharReadTool::charLength() const noexcept -> unit::CpLength {
    const auto data = _data.dataSpan();
    auto position = unit::ByteIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < data.size()) {
        utf8::fastAdvanceChar(data, position);
        ++result;
    }
    return result;
}

auto U8StringCharReadTool::charAt(const unit::CpIndex index) const noexcept -> Char {
    if (index.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    auto position = unit::ByteIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return Char{utf8::decodeCharOrReplace(data, position)};
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? Char::endOfData() : Char::noCodePoint();
}

auto U8StringCharReadTool::charAtOrThrow(const unit::CpIndex index) const -> Char {
    if (index.isNoIndex()) {
        err::throwOutOfRange("Read position out of range");
    }
    const auto data = _data.dataSpan();
    auto position = unit::ByteIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return utf8::decodeCharOrThrow(data, position);
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    err::throwOutOfRange("Read position out of range");
}

auto U8StringCharReadTool::byteIndexAt(const unit::CpIndex index) const noexcept -> unit::ByteIndex {
    if (index.isNoIndex()) {
        return unit::ByteIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    auto position = unit::ByteIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return position;
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? unit::ByteIndex::fromSizeT(data.size()) : unit::ByteIndex::noIndex();
}

auto U8StringCharReadTool::charIndexAt(const unit::ByteIndex index) const noexcept -> unit::CpIndex {
    if (index.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    if (index.toSizeT() > data.size()) {
        return unit::CpIndex::noIndex();
    }

    auto position = unit::ByteIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
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

auto U8StringCharReadTool::sliceRange(const unit::CpRange range) const noexcept -> unit::ByteRange {
    if (!_data.range().isValid() || !range.isValid() || range.isEmpty()) {
        return unit::ByteRange::empty();
    }

    const auto data = _data.dataSpan();
    auto position = unit::ByteIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size() && currentIndex < range.index()) {
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    if (currentIndex < range.index() || position.toSizeT() >= data.size()) {
        return unit::ByteRange::empty();
    }

    const auto sliceStart = position;
    auto remainingLength = range.length();
    while (position.toSizeT() < data.size() && (remainingLength.isInfinite() || !remainingLength.isZero())) {
        utf8::fastAdvanceChar(data, position);
        if (!remainingLength.isInfinite()) {
            --remainingLength;
        }
    }

    const auto sliceLength = unit::ByteLength::fromSizeT(position.toSizeT() - sliceStart.toSizeT());
    return unit::ByteRange{sliceStart, sliceLength}.withOrigin(_data.range().index());
}

auto U8StringCharReadTool::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findFirstOf(characters, unit::CpIndex::zero());
}

auto U8StringCharReadTool::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U8StringCharReadTool::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findFirstNotOf(characters, unit::CpIndex::zero());
}

auto U8StringCharReadTool::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U8StringCharReadTool::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findLastOf(characters, unit::CpIndex::end(charLength()));
}

auto U8StringCharReadTool::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U8StringCharReadTool::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findLastNotOf(characters, unit::CpIndex::end(charLength()));
}

auto U8StringCharReadTool::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U8StringCharReadTool::find(const U8StringDataView &text) const noexcept -> unit::CpIndex {
    return find(text, unit::CpIndex::zero());
}

auto U8StringCharReadTool::find(const U8StringDataView &text, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    if (start.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }

    const auto needle = text.dataSpan();
    auto position = byteIndexAt(start);
    if (position.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }
    if (needle.empty()) {
        return start;
    }

    const auto data = _data.dataSpan();
    if (position.toSizeT() >= data.size()) {
        return unit::CpIndex::noIndex();
    }

    auto currentIndex = start;
    while (position.toSizeT() < data.size()) {
        if (matchesCharacterSequence(data, position, needle)) {
            return currentIndex;
        }
        utf8::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return unit::CpIndex::noIndex();
}

auto U8StringCharReadTool::findFirstOfCharacterSet(
    const CharacterSet &characters, const unit::CpIndex start, const bool isMatching) const noexcept -> unit::CpIndex {
    if (start.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }

    auto position = byteIndexAt(start);
    if (position.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (position.toSizeT() >= data.size()) {
        return unit::CpIndex::noIndex();
    }

    auto currentIndex = start;
    while (position.toSizeT() < data.size()) {
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
        ++currentIndex;
    }
    return unit::CpIndex::noIndex();
}

auto U8StringCharReadTool::findLastOfCharacterSet(
    const CharacterSet &characters, const unit::CpIndex end, const bool isMatching) const noexcept -> unit::CpIndex {
    if (end.isNoIndex() || end.isZero()) {
        return unit::CpIndex::noIndex();
    }

    auto position = byteIndexAt(end);
    if (position.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || position.isZero()) {
        return unit::CpIndex::noIndex();
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
    return unit::CpIndex::noIndex();
}

auto U8StringCharReadTool::matchesCharacterSequence(
    const std::span<const char> haystack,
    const unit::ByteIndex candidateStart,
    const std::span<const char> needle) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = unit::ByteIndex::zero();
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
