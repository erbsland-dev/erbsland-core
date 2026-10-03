// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringReadTools.hpp"

#include "U8Encoding.hpp"
#include "U8Writer.hpp"

#include "../../../unit/U16Count.hpp"
#include "../../u16/impl/U16Encoding.hpp"
#include "../../u16/impl/U16Writer.hpp"
#include "../../u32/impl/U32Writer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>

namespace erbsland::text::impl {

using unit::ByteCount;
using unit::ByteIndex;
using unit::ByteRange;
using unit::CpCount;

auto U8StringReadTools::isEncodingValid() const noexcept -> bool {
    return utf8::isValid(_data.dataSpan());
}

void U8StringReadTools::encodingValidOrThrow() const {
    utf8::forEachDecodedCharacter<EncodingMode::Strict>(_data.dataSpan(), [](const Char) -> void {});
}

auto U8StringReadTools::findFirstOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findFirstOf(characters, ByteIndex::zero());
}

auto U8StringReadTools::findFirstOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return findFirstOfCharacterSet(characters, start, true).beginIndex();
}

auto U8StringReadTools::findFirstNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findFirstNotOf(characters, ByteIndex::zero());
}

auto U8StringReadTools::findFirstNotOf(const CharSet &characters, const ByteIndex start) const noexcept -> ByteIndex {
    return findFirstOfCharacterSet(characters, start, false).beginIndex();
}

auto U8StringReadTools::findLastOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findLastOf(characters, ByteIndex::end(byteLength()));
}

auto U8StringReadTools::findLastOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return findLastOfCharacterSet(characters, end, true).beginIndex();
}

auto U8StringReadTools::findLastNotOf(const CharSet &characters) const noexcept -> ByteIndex {
    return findLastNotOf(characters, ByteIndex::end(byteLength()));
}

auto U8StringReadTools::findLastNotOf(const CharSet &characters, const ByteIndex end) const noexcept -> ByteIndex {
    return findLastOfCharacterSet(characters, end, false).beginIndex();
}

auto U8StringReadTools::byteLength() const noexcept -> ByteCount {
    return ByteCount::fromSizeT(_data.dataSpan().size());
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

auto U8StringReadTools::advance(ByteIndex &index, CpCount count) const noexcept -> bool {
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
        count.uncheckedDecrement();
    }
    return true;
}

auto U8StringReadTools::retreat(ByteIndex &index, CpCount count) const noexcept -> bool {
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
        count.uncheckedDecrement();
    }
    return true;
}

auto U8StringReadTools::sliceRange(const ByteRange range) const noexcept -> ByteRange {
    if (!_data.range().isValid()) {
        return ByteRange::empty();
    }
    return range.clampedTo(byteLength()).toAbsolute(_data.range().beginIndex());
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
    const CharacterSet &characters, const ByteIndex start, const bool isMatching) const noexcept -> ByteRange {
    if (start.isNoIndex()) {
        return ByteRange::noRange();
    }

    const auto data = _data.dataSpan();
    if (start.toSizeT() >= data.size()) {
        return ByteRange::noRange();
    }

    if (isMatching) {
        if (const auto separator = singleAsciiCharacter(characters)) {
            const auto match =
                std::find(data.begin() + static_cast<std::ptrdiff_t>(start.toSizeT()), data.end(), *separator);
            if (match == data.end()) {
                return ByteRange::noRange();
            }
            const auto index = ByteIndex::fromSizeT(static_cast<std::size_t>(std::distance(data.begin(), match)));
            return ByteRange{index, index.incremented()};
        }
    }

    auto position = start;
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (characters.contains(character) == isMatching) {
            return ByteRange{characterStart, position};
        }
    }
    return ByteRange::noRange();
}

auto U8StringReadTools::findLastOfCharacterSet(
    const CharacterSet &characters, const ByteIndex end, const bool isMatching) const noexcept -> ByteRange {
    if (end.isNoIndex() || end.isZero()) {
        return ByteRange::noRange();
    }

    const auto data = _data.dataSpan();
    if (data.empty() || end.toSizeT() > data.size()) {
        return ByteRange::noRange();
    }

    if (isMatching) {
        if (const auto separator = singleAsciiCharacter(characters)) {
            auto offset = end.toSizeT();
            while (offset > 0U) {
                --offset;
                if (data[offset] == *separator) {
                    const auto index = ByteIndex::fromSizeT(offset);
                    return ByteRange{index, index.incremented()};
                }
            }
            return ByteRange::noRange();
        }
    }

    auto position = end;
    while (!position.isZero()) {
        utf8::fastRetreatChar(data, position);
        auto readPosition = position;
        const auto character = utf8::decodeCharOrReplace(data, readPosition);
        if (characters.contains(character) == isMatching) {
            return ByteRange{position, readPosition};
        }
    }
    return ByteRange::noRange();
}

auto U8StringReadTools::findFirstRange(const CharSet &characters) const noexcept -> ByteRange {
    if (characters.isEmpty()) {
        return ByteRange::noRange();
    }
    return findFirstOfCharacterSet(characters, ByteIndex::zero(), true);
}

auto U8StringReadTools::findLastRange(const CharSet &characters) const noexcept -> ByteRange {
    if (characters.isEmpty()) {
        return ByteRange::noRange();
    }
    return findLastOfCharacterSet(characters, ByteIndex::fromSizeT(_data.dataSpan().size()), true);
}

auto U8StringReadTools::singleAsciiCharacter(const CharSet &characters) noexcept -> std::optional<char> {
    auto firstRange = std::optional<CharRange>{};
    auto hasMultipleRanges = false;
    characters.forEach([&](const CharRange range) noexcept -> util::LoopStatus {
        if (firstRange.has_value()) {
            hasMultipleRanges = true;
            return util::LoopStatus::Stop;
        }
        firstRange = range;
        return util::LoopStatus::Continue;
    });
    if (!hasMultipleRanges && firstRange.has_value() && firstRange->isSingleChar() &&
        firstRange->from().toRawValue() <= 0x7FU) {
        return static_cast<char>(firstRange->from().toRawValue());
    }
    return std::nullopt;
}

}
