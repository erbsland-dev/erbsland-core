// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringReadTools.hpp"

#include "U16Encoding.hpp"
#include "U16Writer.hpp"

#include "../../../err/ThrowHelper.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../u32/impl/U32Writer.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <algorithm>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

auto U16StringReadTools::isValidUtf16() const noexcept -> bool {
    return utf16::isValid(_data.dataSpan());
}

auto U16StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return findFirstOf(characters, unit::U16DataIndex::zero());
}

auto U16StringReadTools::findFirstOf(const CharSet &characters, const unit::U16DataIndex start) const noexcept
    -> unit::U16DataIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U16StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return findFirstNotOf(characters, unit::U16DataIndex::zero());
}

auto U16StringReadTools::findFirstNotOf(const CharSet &characters, const unit::U16DataIndex start) const noexcept
    -> unit::U16DataIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U16StringReadTools::findLastOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return findLastOf(characters, unit::U16DataIndex::end(byteLength()));
}

auto U16StringReadTools::findLastOf(const CharSet &characters, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U16StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex {
    return findLastNotOf(characters, unit::U16DataIndex::end(byteLength()));
}

auto U16StringReadTools::findLastNotOf(const CharSet &characters, const unit::U16DataIndex end) const noexcept
    -> unit::U16DataIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U16StringReadTools::byteLength() const noexcept -> unit::U16DataLength {
    return unit::U16DataLength::fromSizeT(_data.dataSpan().size());
}

auto U16StringReadTools::charAt(const unit::U16DataIndex startIndex) const noexcept -> Char {
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

auto U16StringReadTools::read(unit::U16DataIndex &index) const noexcept -> Char {
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

auto U16StringReadTools::charAtOrThrow(const unit::U16DataIndex startIndex) const -> Char {
    const auto data = _data.dataSpan();
    if (startIndex.isNoIndex() || startIndex.toSizeT() >= data.size()) {
        err::throwOutOfRange("Read position out of range");
    }
    auto position = startIndex;
    return utf16::decodeCharOrThrow(data, position);
}

auto U16StringReadTools::readOrThrow(unit::U16DataIndex &index) const -> Char {
    const auto data = _data.dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        err::throwOutOfRange("Read position out of range");
    }
    return Char{utf16::decodeCharOrThrow(data, index)};
}

auto U16StringReadTools::advance(unit::U16DataIndex &index, unit::CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = unit::U16DataIndex::fromSizeT(data.size());
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

auto U16StringReadTools::retreat(unit::U16DataIndex &index, unit::CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero() || index.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = unit::U16DataIndex::fromSizeT(data.size());
    if (index > endIndex) {
        index = endIndex;
    }
    if (count.isInfinite() || count.toSizeT() >= index.toSizeT() || data.empty()) {
        index = unit::U16DataIndex::zero();
        return true;
    }
    while (!count.isZero() && !index.isZero()) {
        utf16::fastRetreatChar(data, index);
        --count;
    }
    return true;
}

auto U16StringReadTools::sliceRange(const unit::U16DataRange range) const noexcept -> unit::U16DataRange {
    if (!_data.range().isValid()) {
        return unit::U16DataRange::empty();
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
    const CharacterSet &characters, const unit::U16DataIndex start, const bool isMatching) const noexcept
    -> unit::U16DataIndex {
    if (start.isNoIndex()) {
        return unit::U16DataIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return unit::U16DataIndex::noIndex();
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return characterStart;
        }
    }
    return unit::U16DataIndex::noIndex();
}

auto U16StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const unit::U16DataIndex end, const bool isMatching) const noexcept
    -> unit::U16DataIndex {
    if (end.isNoIndex() || end.isZero()) {
        return unit::U16DataIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return unit::U16DataIndex::noIndex();
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
    return unit::U16DataIndex::noIndex();
}

}
