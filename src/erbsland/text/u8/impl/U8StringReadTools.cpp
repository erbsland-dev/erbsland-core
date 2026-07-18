// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringReadTools.hpp"

#include "U8Encoding.hpp"
#include "U8Writer.hpp"

#include "../../../unit/U16DataLength.hpp"
#include "../../impl/ThrowHelper.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16Writer.hpp"
#include "../../u32/impl/U32Writer.hpp"

#include <algorithm>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

using unit::ByteIndex;
using unit::ByteLength;
using unit::ByteRange;
using unit::CpLength;

auto U8StringReadTools::isValidUtf8() const noexcept -> bool {
    return utf8::isValid(_data.dataSpan());
}

auto U8StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findFirstOf(characters, ByteIndex::zero());
}

auto U8StringReadTools::findFirstOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U8StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findFirstNotOf(characters, ByteIndex::zero());
}

auto U8StringReadTools::findFirstNotOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U8StringReadTools::findLastOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findLastOf(characters, ByteIndex::end(byteLength()));
}

auto U8StringReadTools::findLastOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U8StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findLastNotOf(characters, ByteIndex::end(byteLength()));
}

auto U8StringReadTools::findLastNotOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U8StringReadTools::byteLength() const noexcept -> ByteLength {
    return ByteLength::fromSizeT(_data.dataSpan().size());
}

auto U8StringReadTools::displayWidth() const noexcept -> int {
    auto result = 0;
    const auto data = _data.dataSpan();
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        result += utf8::decodeCharOrReplace(data, position).displayWidth();
    }
    return result;
}

auto U8StringReadTools::charAt(const ByteIndex startIndex) const noexcept -> Char {
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
    return Char{utf8::decodeCharOrReplace(data, position)};
}

auto U8StringReadTools::read(ByteIndex &index) const noexcept -> Char {
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
    return Char{utf8::decodeCharOrReplace(data, index)};
}

auto U8StringReadTools::readAndRetreat(ByteIndex &index) const noexcept -> Char {
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
    utf8::fastRetreatChar(data, startIndex);
    auto readIndex = startIndex;
    const auto result = Char{utf8::decodeCharOrReplace(data, readIndex)};
    index = startIndex;
    return result;
}

auto U8StringReadTools::charAtOrThrow(const ByteIndex startIndex) const -> Char {
    const auto data = _data.dataSpan();
    if (startIndex.isNoIndex() || startIndex.toSizeT() >= data.size()) {
        text::impl::throwOutOfRange("Read position out of range");
    }
    auto position = startIndex;
    return utf8::decodeCharOrThrow(data, position);
}

auto U8StringReadTools::readOrThrow(ByteIndex &index) const -> Char {
    const auto data = _data.dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        text::impl::throwOutOfRange("Read position out of range");
    }
    return Char{utf8::decodeCharOrThrow(data, index)};
}

auto U8StringReadTools::advance(ByteIndex &index, CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = ByteIndex::fromSizeT(data.size());
    const auto moveToEnd = [&]() -> bool {
        const auto didAdvance = index < endIndex;
        index = endIndex;
        return didAdvance;
    };
    if (index.toSizeT() >= data.size() || count.isInfinite()) {
        return moveToEnd();
    }
    while (!count.isZero() && index < endIndex) {
        utf8::fastAdvanceChar(data, index);
        --count;
    }
    return true;
}

auto U8StringReadTools::retreat(ByteIndex &index, CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero() || index.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = ByteIndex::fromSizeT(data.size());
    if (index > endIndex) {
        index = endIndex;
    }
    if (count.isInfinite() || count.toSizeT() >= index.toSizeT() || data.empty()) {
        index = ByteIndex::zero();
        return true;
    }
    while (!count.isZero() && !index.isZero()) {
        utf8::fastRetreatChar(data, index);
        --count;
    }
    return true;
}

auto U8StringReadTools::sliceRange(const ByteRange range) const noexcept -> ByteRange {
    if (!_data.range().isValid()) {
        return ByteRange::empty();
    }
    return range.clampedTo(byteLength()).withOrigin(_data.range().index());
}

auto U8StringReadTools::toStdString() const noexcept -> std::string {
    return createUtf8String<std::string>(_data.dataSpan());
}

auto U8StringReadTools::toStdU8String() const noexcept -> std::u8string {
    return createUtf8String<std::u8string>(_data.dataSpan());
}

auto U8StringReadTools::toStdU16String() const noexcept -> std::u16string {
    return createUtf16String<std::u16string>(_data.dataSpan());
}

auto U8StringReadTools::toStdU32String() const noexcept -> std::u32string {
    return createUtf32String<std::u32string>(_data.dataSpan());
}

auto U8StringReadTools::toStdWString() const noexcept -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    return createUtf16String<std::wstring>(_data.dataSpan());
#else
    return createUtf32String<std::wstring>(_data.dataSpan());
#endif
}

auto U8StringReadTools::findFirstOfCharacterSet(
    const CharacterSet &characters, const ByteIndex start, const bool isMatching) const noexcept -> ByteIndex {
    if (start.isNoIndex()) {
        return ByteIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return ByteIndex::noIndex();
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return characterStart;
        }
    }
    return ByteIndex::noIndex();
}

auto U8StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const ByteIndex end, const bool isMatching) const noexcept -> ByteIndex {
    if (end.isNoIndex() || end.isZero()) {
        return ByteIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return ByteIndex::noIndex();
    }

    auto position = end;
    while (!position.isZero()) {
        utf8::fastRetreatChar(data, position);
        auto readPosition = position;
        const auto character = utf8::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return position;
        }
    }
    return ByteIndex::noIndex();
}

}
