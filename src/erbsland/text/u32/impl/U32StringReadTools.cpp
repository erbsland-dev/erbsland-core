// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringReadTools.hpp"

#include "U32Encoding.hpp"
#include "U32Writer.hpp"

#include "../../../err/ThrowHelper.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16Writer.hpp"
#include "../../u8/impl/U8Writer.hpp"

#include <algorithm>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

auto U32StringReadTools::isValidUtf32() const noexcept -> bool {
    return utf32::isValid(_data.dataSpan());
}

auto U32StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findFirstOf(characters, unit::CpIndex::zero());
}

auto U32StringReadTools::findFirstOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return findFirstOfCharacterSet(characters, start, true);
}

auto U32StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findFirstNotOf(characters, unit::CpIndex::zero());
}

auto U32StringReadTools::findFirstNotOf(const CharSet &characters, const unit::CpIndex start) const noexcept
    -> unit::CpIndex {
    return findFirstOfCharacterSet(characters, start, false);
}

auto U32StringReadTools::findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findLastOf(characters, unit::CpIndex::end(length()));
}

auto U32StringReadTools::findLastOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return findLastOfCharacterSet(characters, end, true);
}

auto U32StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex {
    return findLastNotOf(characters, unit::CpIndex::end(length()));
}

auto U32StringReadTools::findLastNotOf(const CharSet &characters, const unit::CpIndex end) const noexcept
    -> unit::CpIndex {
    return findLastOfCharacterSet(characters, end, false);
}

auto U32StringReadTools::length() const noexcept -> unit::CpLength {
    return unit::CpLength::fromSizeT(_data.dataSpan().size());
}

auto U32StringReadTools::charAt(const unit::CpIndex startIndex) const noexcept -> Char {
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

auto U32StringReadTools::read(unit::CpIndex &index) const noexcept -> Char {
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

auto U32StringReadTools::charAtOrThrow(const unit::CpIndex startIndex) const -> Char {
    const auto data = _data.dataSpan();
    if (startIndex.isNoIndex() || startIndex.toSizeT() >= data.size()) {
        err::throwOutOfRange("Read position out of range");
    }
    auto position = startIndex;
    return utf32::decodeCharOrThrow(data, position);
}

auto U32StringReadTools::readOrThrow(unit::CpIndex &index) const -> Char {
    const auto data = _data.dataSpan();
    if (index.isNoIndex() || index.toSizeT() >= data.size()) {
        err::throwOutOfRange("Read position out of range");
    }
    return Char{utf32::decodeCharOrThrow(data, index)};
}

auto U32StringReadTools::advance(unit::CpIndex &index, unit::CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = unit::CpIndex::fromSizeT(data.size());
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
        --count;
    }
    return true;
}

auto U32StringReadTools::retreat(unit::CpIndex &index, unit::CpLength count) const noexcept -> bool {
    if (index.isNoIndex() || count.isZero() || index.isZero()) {
        return false;
    }
    const auto data = _data.dataSpan();
    const auto endIndex = unit::CpIndex::fromSizeT(data.size());
    if (index > endIndex) {
        index = endIndex;
    }
    if (count.isInfinite() || count.toSizeT() >= index.toSizeT() || data.empty()) {
        index = unit::CpIndex::zero();
        return true;
    }
    while (!count.isZero() && !index.isZero()) {
        utf32::fastRetreatChar(data, index);
        --count;
    }
    return true;
}

auto U32StringReadTools::sliceRange(const unit::CpRange range) const noexcept -> unit::CpRange {
    if (!_data.range().isValid()) {
        return unit::CpRange::empty();
    }
    return range.clampedTo(length()).withOrigin(_data.range().index());
}

auto U32StringReadTools::toStdString() const noexcept -> std::string {
    return createUtf8String<std::string>(_data.dataSpan());
}

auto U32StringReadTools::toStdU8String() const noexcept -> std::u8string {
    return createUtf8String<std::u8string>(_data.dataSpan());
}

auto U32StringReadTools::toStdU16String() const noexcept -> std::u16string {
    const auto data = _data.dataSpan();
    auto reservedSize = unit::U16DataLength::zero();
    utf32::forEachDecodedCharacter(data, EncodingErrorMode::Replace, [&](const Char character) -> void {
        reservedSize += utf16::encodedLength(character);
    });
    auto result = std::u16string{};
    result.resize(reservedSize.toSizeT());
    auto writer = U16Writer{std::span{result.data(), result.size()}};
    utf32::forEachDecodedCharacter(
        data, EncodingErrorMode::Replace, [&](const Char character) -> void { writer.write(character); });
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
    const CharacterSet &characters, const unit::CpIndex start, const bool isMatching) const noexcept -> unit::CpIndex {
    if (start.isNoIndex()) {
        return unit::CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return unit::CpIndex::noIndex();
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return characterStart;
        }
    }
    return unit::CpIndex::noIndex();
}

auto U32StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const unit::CpIndex end, const bool isMatching) const noexcept -> unit::CpIndex {
    if (end.isNoIndex() || end.isZero()) {
        return unit::CpIndex::noIndex();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return unit::CpIndex::noIndex();
    }

    auto position = end;
    while (!position.isZero()) {
        utf32::fastRetreatChar(data, position);
        auto readPosition = position;
        const auto character = utf32::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return position;
        }
    }
    return unit::CpIndex::noIndex();
}

}
