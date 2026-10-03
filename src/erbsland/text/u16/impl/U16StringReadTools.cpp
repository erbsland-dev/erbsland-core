// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringReadTools.hpp"

#include "U16Encoding.hpp"
#include "U16Writer.hpp"

#include "../../../unit/U16Count.hpp"
#include "../../u32/impl/U32Writer.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <algorithm>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

using unit::CpCount;
using unit::U16Count;
using unit::U16Index;
using unit::U16Range;

auto U16StringReadTools::isEncodingValid() const noexcept -> bool {
    return utf16::isValid(_data.dataSpan());
}

void U16StringReadTools::encodingValidOrThrow() const {
    utf16::forEachDecodedCharacter<EncodingMode::Strict>(_data.dataSpan(), [](const Char) -> void {});
}

auto U16StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> U16Index {
    return findFirstOf(characters, U16Index::zero());
}

auto U16StringReadTools::findFirstOf(const CharSet &characters, const U16Index start) const noexcept -> U16Index {
    return findFirstOfCharacterSet(characters, start, true).beginIndex();
}

auto U16StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> U16Index {
    return findFirstNotOf(characters, U16Index::zero());
}

auto U16StringReadTools::findFirstNotOf(const CharSet &characters, const U16Index start) const noexcept -> U16Index {
    return findFirstOfCharacterSet(characters, start, false).beginIndex();
}

auto U16StringReadTools::findLastOf(const CharSet &characters) const noexcept -> U16Index {
    return findLastOf(characters, U16Index::end(byteLength()));
}

auto U16StringReadTools::findLastOf(const CharSet &characters, const U16Index end) const noexcept -> U16Index {
    return findLastOfCharacterSet(characters, end, true).beginIndex();
}

auto U16StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> U16Index {
    return findLastNotOf(characters, U16Index::end(byteLength()));
}

auto U16StringReadTools::findLastNotOf(const CharSet &characters, const U16Index end) const noexcept -> U16Index {
    return findLastOfCharacterSet(characters, end, false).beginIndex();
}

auto U16StringReadTools::byteLength() const noexcept -> U16Count {
    return U16Count::fromSizeT(_data.dataSpan().size());
}

auto U16StringReadTools::displayWidth() const noexcept -> int {
    auto result = 0;
    const auto data = _data.dataSpan();
    auto position = U16Index::zero();
    while (position.toSizeT() < data.size()) {
        result += utf16::decodeCharOrReplace(data, position).displayWidth();
    }
    return result;
}

auto U16StringReadTools::charAt(const U16Index startIndex) const noexcept -> Char {
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

auto U16StringReadTools::read(U16Index &index) const noexcept -> Char {
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

auto U16StringReadTools::readAndRetreat(U16Index &index) const noexcept -> Char {
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

auto U16StringReadTools::advance(U16Index &index, CpCount count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = U16Index::fromSizeT(data.size());
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
        count.uncheckedDecrement();
    }
    return true;
}

auto U16StringReadTools::retreat(U16Index &index, CpCount count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero() || index.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = U16Index::fromSizeT(data.size());
    if (index > endIndex) {
        index = endIndex;
    }
    if (count.isInfinite() || count.toSizeT() >= index.toSizeT() || data.empty()) {
        index = U16Index::zero();
        return true;
    }
    while (!count.isZero() && !index.isZero()) {
        utf16::fastRetreatChar(data, index);
        count.uncheckedDecrement();
    }
    return true;
}

auto U16StringReadTools::sliceRange(const U16Range range) const noexcept -> U16Range {
    if (!_data.range().isValid()) {
        return U16Range::empty();
    }
    return range.clampedTo(byteLength()).toAbsolute(_data.range().beginIndex());
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
    const CharacterSet &characters, const U16Index start, const bool isMatching) const noexcept -> U16Range {
    if (start.isNoIndex()) {
        return U16Range::noRange();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return U16Range::noRange();
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return U16Range{characterStart, position};
        }
    }
    return U16Range::noRange();
}

auto U16StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const U16Index end, const bool isMatching) const noexcept -> U16Range {
    if (end.isNoIndex() || end.isZero()) {
        return U16Range::noRange();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return U16Range::noRange();
    }

    auto position = end;
    while (!position.isZero()) {
        utf16::fastRetreatChar(data, position);
        auto readPosition = position;
        const auto character = utf16::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return U16Range{position, readPosition};
        }
    }
    return U16Range::noRange();
}

auto U16StringReadTools::findFirstRange(const CharSet &characters) const noexcept -> U16Range {
    if (characters.isEmpty()) {
        return U16Range::noRange();
    }
    return findFirstOfCharacterSet(characters, U16Index::zero(), true);
}

auto U16StringReadTools::findLastRange(const CharSet &characters) const noexcept -> U16Range {
    if (characters.isEmpty()) {
        return U16Range::noRange();
    }
    return findLastOfCharacterSet(characters, U16Index::fromSizeT(_data.dataSpan().size()), true);
}

}
