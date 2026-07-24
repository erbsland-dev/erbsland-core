// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringReadTools.hpp"

#include "U16Encoding.hpp"
#include "U16Writer.hpp"

#include "../../../unit/U16DataLength.hpp"
#include "../../u32/impl/U32Writer.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <algorithm>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

using unit::CpLength;
using unit::U16DataIndex;
using unit::U16DataLength;
using unit::U16DataRange;

auto U16StringReadTools::isValidUtf16() const noexcept -> bool {
    return utf16::isValid(_data.dataSpan());
}

auto U16StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return findFirstOf(characters, U16DataIndex::zero());
}

auto U16StringReadTools::findFirstOf(const CharSet &characters, const U16DataIndex start) const noexcept
    -> U16DataIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U16StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return findFirstNotOf(characters, U16DataIndex::zero());
}

auto U16StringReadTools::findFirstNotOf(const CharSet &characters, const U16DataIndex start) const noexcept
    -> U16DataIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U16StringReadTools::findLastOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return findLastOf(characters, U16DataIndex::end(byteLength()));
}

auto U16StringReadTools::findLastOf(const CharSet &characters, const U16DataIndex end) const noexcept -> U16DataIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U16StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> U16DataIndex {
    return findLastNotOf(characters, U16DataIndex::end(byteLength()));
}

auto U16StringReadTools::findLastNotOf(const CharSet &characters, const U16DataIndex end) const noexcept
    -> U16DataIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U16StringReadTools::byteLength() const noexcept -> U16DataLength {
    return U16DataLength::fromSizeT(_data.dataSpan().size());
}

auto U16StringReadTools::displayWidth() const noexcept -> int {
    auto result = 0;
    const auto data = _data.dataSpan();
    auto position = U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        result += utf16::decodeCharOrReplace(data, position).displayWidth();
    }
    return result;
}

auto U16StringReadTools::charAt(const U16DataIndex startIndex) const noexcept -> Char {
    if (startIndex.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    if (startIndex.toSizeT() == data.size()) {
        return Char::endOfData();
    }
    if (startIndex.toSizeT() > data.size()) {
        return Char::noCodePoint();
    }
    auto position = startIndex;
    return Char{utf16::decodeCharOrReplace(data, position)};
}

auto U16StringReadTools::read(U16DataIndex &index) const noexcept -> Char {
    if (index.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    if (index.toSizeT() == data.size()) {
        return Char::endOfData();
    }
    if (index.toSizeT() > data.size()) {
        return Char::noCodePoint();
    }
    return Char{utf16::decodeCharOrReplace(data, index)};
}

auto U16StringReadTools::readAndRetreat(U16DataIndex &index) const noexcept -> Char {
    if (index.isNoIndex()) {
        return Char::noCodePoint();
    }
    const auto data = _data.dataSpan();
    if (index.isZero()) {
        return Char::endOfData();
    }
    if (index.toSizeT() > data.size()) {
        return Char::noCodePoint();
    }
    auto startIndex = index;
    utf16::fastRetreatChar(data, startIndex);
    auto readIndex = startIndex;
    const auto result = Char{utf16::decodeCharOrReplace(data, readIndex)};
    index = startIndex;
    return result;
}

auto U16StringReadTools::advance(U16DataIndex &index, CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = U16DataIndex::fromSizeT(data.size());
    const auto moveToEnd = [&]() -> bool {
        const auto didAdvance = index < endIndex;
        index = endIndex;
        return didAdvance;
    };
    if (index.toSizeT() >= data.size() || count.isInfinite()) {
        return moveToEnd();
    }
    while (!count.isZero() && index < endIndex) {
        utf16::fastAdvanceChar(data, index);
        --count;
    }
    return true;
}

auto U16StringReadTools::retreat(U16DataIndex &index, CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero() || index.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = U16DataIndex::fromSizeT(data.size());
    if (index > endIndex) {
        index = endIndex;
    }
    if (count.isInfinite() || count.toSizeT() >= index.toSizeT() || data.empty()) {
        index = U16DataIndex::zero();
        return true;
    }
    while (!count.isZero() && !index.isZero()) {
        utf16::fastRetreatChar(data, index);
        --count;
    }
    return true;
}

auto U16StringReadTools::sliceRange(const U16DataRange range) const noexcept -> U16DataRange {
    if (!_data.range().isValid()) {
        return U16DataRange::empty();
    }
    return range.clampedTo(byteLength()).withOrigin(_data.range().index());
}

auto U16StringReadTools::toStdString() const noexcept -> std::string {
    return createUtf8String<std::string>(_data.dataSpan());
}

auto U16StringReadTools::toStdU8String() const noexcept -> std::u8string {
    return createUtf8String<std::u8string>(_data.dataSpan());
}

auto U16StringReadTools::toStdU16String() const noexcept -> std::u16string {
    return createUtf16String<std::u16string>(_data.dataSpan());
}

auto U16StringReadTools::toStdU32String() const noexcept -> std::u32string {
    return createUtf32String<std::u32string>(_data.dataSpan());
}

auto U16StringReadTools::toStdWString() const noexcept -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    return createUtf16String<std::wstring>(_data.dataSpan());
#else
    return createUtf32String<std::wstring>(_data.dataSpan());
#endif
}

auto U16StringReadTools::findFirstOfCharacterSet(
    const CharacterSet &characters, const U16DataIndex start, const bool isMatching) const noexcept -> U16DataIndex {
    if (start.isNoIndex()) {
        return U16DataIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return U16DataIndex::noIndex();
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return characterStart;
        }
    }
    return U16DataIndex::noIndex();
}

auto U16StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const U16DataIndex end, const bool isMatching) const noexcept -> U16DataIndex {
    if (end.isNoIndex() || end.isZero()) {
        return U16DataIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return U16DataIndex::noIndex();
    }

    auto position = end;
    while (!position.isZero()) {
        utf16::fastRetreatChar(data, position);
        auto readPosition = position;
        const auto character = utf16::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return position;
        }
    }
    return U16DataIndex::noIndex();
}

}
