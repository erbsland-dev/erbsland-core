// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringCharReadTool.hpp"

#include "U16Encoding.hpp"

#include "../../../err/ThrowHelper.hpp"

#include <algorithm>

namespace erbsland::text::impl {

auto U16StringCharReadTool::charLength() const noexcept -> unit::CpLength {
    const auto data = _data.dataSpan();
    auto position = unit::U16DataIndex::zero();
    auto result = unit::CpLength::zero();
    while (position.toSizeT() < data.size()) {
        utf16::fastAdvanceChar(data, position);
        ++result;
    }
    return result;
}

auto U16StringCharReadTool::charAt(const unit::CpIndex index) const noexcept -> Char {
    if (index.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    auto position = unit::U16DataIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return Char{utf16::decodeCharOrReplace(data, position)};
        }
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? Char::endOfData() : Char::noCodePoint();
}

auto U16StringCharReadTool::charAtOrThrow(const unit::CpIndex index) const -> Char {
    if (index.isNoIndex()) {
        err::throwOutOfRange("Read position out of range");
    }
    const auto data = _data.dataSpan();
    auto position = unit::U16DataIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return utf16::decodeCharOrThrow(data, position);
        }
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    err::throwOutOfRange("Read position out of range");
}

auto U16StringCharReadTool::byteIndexAt(const unit::CpIndex index) const noexcept -> unit::U16DataIndex {
    if (index.isNoIndex()) {
        return unit::U16DataIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    auto position = unit::U16DataIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (currentIndex == index) {
            return position;
        }
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return currentIndex == index ? unit::U16DataIndex::fromSizeT(data.size()) : unit::U16DataIndex::noIndex();
}

auto U16StringCharReadTool::charIndexAt(const unit::U16DataIndex index) const noexcept -> unit::CpIndex {
    if (index.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }
    const auto data = _data.dataSpan();
    if (index.toSizeT() > data.size()) {
        return unit::CpIndex::noIndex();
    }

    auto position = unit::U16DataIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
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

auto U16StringCharReadTool::sliceRange(const unit::CpRange range) const noexcept -> unit::U16DataRange {
    if (!_data.range().isValid() || !range.isValid() || range.isEmpty()) {
        return unit::U16DataRange::empty();
    }

    const auto data = _data.dataSpan();
    auto position = unit::U16DataIndex::zero();
    auto currentIndex = unit::CpIndex::zero();
    while (position.toSizeT() < data.size() && currentIndex < range.index()) {
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    if (currentIndex < range.index() || position.toSizeT() >= data.size()) {
        return unit::U16DataRange::empty();
    }

    const auto sliceStart = position;
    auto remainingLength = range.length();
    while (position.toSizeT() < data.size() && (remainingLength.isInfinite() || !remainingLength.isZero())) {
        utf16::fastAdvanceChar(data, position);
        if (!remainingLength.isInfinite()) {
            --remainingLength;
        }
    }

    const auto sliceLength = unit::U16DataLength::fromSizeT(position.toSizeT() - sliceStart.toSizeT());
    return unit::U16DataRange{sliceStart, sliceLength}.withOrigin(_data.range().index());
}

auto U16StringCharReadTool::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findFirstOf(characters, unit::CpIndex::zero());
}

auto U16StringCharReadTool::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U16StringCharReadTool::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findFirstNotOf(characters, unit::CpIndex::zero());
}

auto U16StringCharReadTool::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U16StringCharReadTool::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findLastOf(characters, unit::CpIndex::end(charLength()));
}

auto U16StringCharReadTool::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U16StringCharReadTool::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findLastNotOf(characters, unit::CpIndex::end(charLength()));
}

auto U16StringCharReadTool::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U16StringCharReadTool::find(const U16StringDataView &text) const noexcept -> unit::CpIndex {
    return find(text, unit::CpIndex::zero());
}

auto U16StringCharReadTool::find(const U16StringDataView &text, const unit::CpIndex start) const noexcept
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
        utf16::fastAdvanceChar(data, position);
        ++currentIndex;
    }
    return unit::CpIndex::noIndex();
}

auto U16StringCharReadTool::findFirstOfCharacterSet(
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
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
        ++currentIndex;
    }
    return unit::CpIndex::noIndex();
}

auto U16StringCharReadTool::findLastOfCharacterSet(
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
        utf16::fastRetreatChar(data, position);
        --currentIndex;
        auto readPosition = position;
        const auto character = utf16::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return currentIndex;
        }
    }
    return unit::CpIndex::noIndex();
}

auto U16StringCharReadTool::matchesCharacterSequence(
    const std::span<const char16_t> haystack,
    const unit::U16DataIndex candidateStart,
    const std::span<const char16_t> needle) noexcept -> bool {
    auto haystackPosition = candidateStart;
    auto needlePosition = unit::U16DataIndex::zero();
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
