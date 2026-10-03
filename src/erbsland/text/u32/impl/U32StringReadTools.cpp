// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringReadTools.hpp"

#include "U32Encoding.hpp"
#include "U32Writer.hpp"

#include "../../../unit/CpCount.hpp"
#include "../../../unit/U16Count.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16Writer.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <algorithm>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

using unit::CpCount;
using unit::CpIndex;
using unit::CpRange;
using unit::U16Count;

auto U32StringReadTools::isEncodingValid() const noexcept -> bool {
    return utf32::isValid(_data.dataSpan());
}

void U32StringReadTools::encodingValidOrThrow() const {
    utf32::forEachDecodedCharacter(_data.dataSpan(), EncodingMode::Strict, [](const Char) -> void {});
}

auto U32StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> CpIndex {
    return findFirstOf(characters, CpIndex::zero());
}

auto U32StringReadTools::findFirstOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return findFirstOfCharacterSet(characters, start, true).beginIndex();
}

auto U32StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return findFirstNotOf(characters, CpIndex::zero());
}

auto U32StringReadTools::findFirstNotOf(const CharSet &characters, const CpIndex start) const noexcept -> CpIndex {
    return findFirstOfCharacterSet(characters, start, false).beginIndex();
}

auto U32StringReadTools::findLastOf(const CharSet &characters) const noexcept -> CpIndex {
    return findLastOf(characters, CpIndex::end(length()));
}

auto U32StringReadTools::findLastOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return findLastOfCharacterSet(characters, end, true).beginIndex();
}

auto U32StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> CpIndex {
    return findLastNotOf(characters, CpIndex::end(length()));
}

auto U32StringReadTools::findLastNotOf(const CharSet &characters, const CpIndex end) const noexcept -> CpIndex {
    return findLastOfCharacterSet(characters, end, false).beginIndex();
}

auto U32StringReadTools::length() const noexcept -> CpCount {
    return CpCount::fromSizeT(_data.dataSpan().size());
}

auto U32StringReadTools::displayWidth() const noexcept -> int {
    auto result = 0;
    const auto data = _data.dataSpan();
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        result += utf32::decodeCharOrReplace(data, position).displayWidth();
    }
    return result;
}

auto U32StringReadTools::charAt(const CpIndex startIndex) const noexcept -> Char {
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
    return Char{utf32::decodeCharOrReplace(data, position)};
}

auto U32StringReadTools::read(CpIndex &index) const noexcept -> Char {
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
    return Char{utf32::decodeCharOrReplace(data, index)};
}

auto U32StringReadTools::readAndRetreat(CpIndex &index) const noexcept -> Char {
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
    utf32::fastRetreatChar(data, startIndex);
    auto readIndex = startIndex;
    const auto result = Char{utf32::decodeCharOrReplace(data, readIndex)};
    index = startIndex;
    return result;
}

auto U32StringReadTools::advance(CpIndex &index, CpCount count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = CpIndex::fromSizeT(data.size());
    const auto moveToEnd = [&]() -> bool {
        const auto didAdvance = index < endIndex;
        index = endIndex;
        return didAdvance;
    };
    if (index.toSizeT() >= data.size() || count.isInfinite()) {
        return moveToEnd();
    }
    while (!count.isZero() && index < endIndex) {
        utf32::fastAdvanceChar(data, index);
        count.uncheckedDecrement();
    }
    return true;
}

auto U32StringReadTools::retreat(CpIndex &index, CpCount count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero() || index.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = CpIndex::fromSizeT(data.size());
    if (index > endIndex) {
        index = endIndex;
    }
    if (count.isInfinite() || count.toSizeT() >= index.toSizeT() || data.empty()) {
        index = CpIndex::zero();
        return true;
    }
    while (!count.isZero() && !index.isZero()) {
        utf32::fastRetreatChar(data, index);
        count.uncheckedDecrement();
    }
    return true;
}

auto U32StringReadTools::sliceRange(const CpRange range) const noexcept -> CpRange {
    if (!_data.range().isValid()) {
        return CpRange::empty();
    }
    return range.clampedTo(length()).toAbsolute(_data.range().beginIndex());
}

auto U32StringReadTools::toStdString() const noexcept -> std::string {
    return createUtf8String<std::string>(_data.dataSpan());
}

auto U32StringReadTools::toStdU8String() const noexcept -> std::u8string {
    return createUtf8String<std::u8string>(_data.dataSpan());
}

auto U32StringReadTools::toStdU16String() const noexcept -> std::u16string {
    const auto data = _data.dataSpan();
    auto reservedSize = U16Count::zero();
    utf32::forEachDecodedCharacter(data, EncodingMode::Tolerant, [&](const Char character) -> void {
        reservedSize += utf16::encodedLength(character);
    });
    auto result = std::u16string{};
    result.resize(reservedSize.toSizeT());
    auto writer = U16Writer{std::span{result.data(), result.size()}};
    utf32::forEachDecodedCharacter(
        data, EncodingMode::Tolerant, [&](const Char character) -> void { writer.write(character); });
    return result;
}

auto U32StringReadTools::toStdU32String() const noexcept -> std::u32string {
    return createUtf32String<std::u32string>(_data.dataSpan());
}

auto U32StringReadTools::toStdWString() const noexcept -> std::wstring {
#ifdef ERBSLAND_WCHAR_16BIT
    const auto text = toStdU16String();
    return std::wstring{text.begin(), text.end()};
#else
    return createUtf32String<std::wstring>(_data.dataSpan());
#endif
}

auto U32StringReadTools::findFirstOfCharacterSet(
    const CharacterSet &characters, const CpIndex start, const bool isMatching) const noexcept -> CpRange {
    if (start.isNoIndex()) {
        return CpRange::noRange();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return CpRange::noRange();
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return CpRange{characterStart, position};
        }
    }
    return CpRange::noRange();
}

auto U32StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const CpIndex end, const bool isMatching) const noexcept -> CpRange {
    if (end.isNoIndex() || end.isZero()) {
        return CpRange::noRange();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return CpRange::noRange();
    }

    auto position = end;
    while (!position.isZero()) {
        utf32::fastRetreatChar(data, position);
        auto readPosition = position;
        const auto character = utf32::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return CpRange{position, readPosition};
        }
    }
    return CpRange::noRange();
}

auto U32StringReadTools::findFirstRange(const CharSet &characters) const noexcept -> CpRange {
    if (characters.isEmpty()) {
        return CpRange::noRange();
    }
    return findFirstOfCharacterSet(characters, CpIndex::zero(), true);
}

auto U32StringReadTools::findLastRange(const CharSet &characters) const noexcept -> CpRange {
    if (characters.isEmpty()) {
        return CpRange::noRange();
    }
    return findLastOfCharacterSet(characters, CpIndex::fromSizeT(_data.dataSpan().size()), true);
}

}
