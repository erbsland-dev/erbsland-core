// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringModifyTools.hpp"

#include "U8StringCharReadTool.hpp"
#include "U8Writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace erbsland::text::impl {

using namespace unit;

auto U8StringModifyTools::removed(const CpRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U8StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }

    const auto start = removeRange.index().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U8StringSharedStorage::forSize(newSize, _sensitive);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start);
    }
    const auto tailStart = U8StringSharedStorage::checkedAddSize(start, count, "Remove range exceeds string bounds");
    if (tailStart < data.size()) {
        std::memcpy(storage.dataForWrite() + start, data.data() + tailStart, data.size() - tailStart);
    }
    return storage;
}

auto U8StringModifyTools::removedAll(const CharSet &characters) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U8StringModifyTools::removed(const U8StringDataView &text, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    return replacedText(text, {}, compareFn);
}

auto U8StringModifyTools::kept(const CpRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U8StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return {};
    }
    return U8StringSharedStorage::fromBytes(
        data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()), _sensitive);
}

auto U8StringModifyTools::replacedAll(const CharSet &characters, const Char replacement) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }
    const auto bytes = characterBytes(replacement);
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U8StringModifyTools::replacedAll(const CharSet &characters, const U8StringDataView &replacement) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U8StringModifyTools::replacedAll(
    const U8StringDataView &text, const U8StringDataView &replacement, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    return replacedText(text, replacement.dataSpan(), compareFn);
}

auto U8StringModifyTools::removed(const ByteRange range) const -> U8StringSharedStorage {
    return replaced(range, {});
}

auto U8StringModifyTools::kept(const ByteRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || !range.isValid()) {
        return {};
    }
    const auto keepRange = range.clampedTo(ByteLength::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        return {};
    }
    return U8StringSharedStorage::fromBytes(
        data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()), _sensitive);
}

auto U8StringModifyTools::inserted(const ByteIndex index, const U8StringDataView &text) const -> U8StringSharedStorage {
    if (index.isNoIndex()) {
        return U8StringSharedStorage::fromBytes(_data.dataSpan(), _sensitive);
    }
    const auto insertIndex = ByteIndex::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replaced(ByteRange::emptyAt(insertIndex), text);
}

auto U8StringModifyTools::inserted(const CpIndex index, const U8StringDataView &text) const -> U8StringSharedStorage {
    if (index.isNoIndex()) {
        return U8StringSharedStorage::fromBytes(_data.dataSpan(), _sensitive);
    }
    auto byteIndex = byteIndexForCharacterIndex(_data, index);
    if (byteIndex.isNoIndex()) {
        byteIndex = ByteIndex::end(ByteLength::fromSizeT(_data.dataSpan().size()));
    }
    return inserted(byteIndex, text);
}

auto U8StringModifyTools::replaced(const ByteRange range, const U8StringDataView &text) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }
    const auto replaceRange = range.clampedTo(ByteLength::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }

    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(
        data.size() - removeLength, replacement.size(), "Modified string exceeds size bounds");
    auto storage = U8StringSharedStorage::forSize(newSize, _sensitive);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start);
    }
    if (!replacement.empty()) {
        std::memcpy(storage.dataForWrite() + start, replacement.data(), replacement.size());
    }
    const auto tailStart = U8StringSharedStorage::checkedAddSize(start, removeLength, "Replace range exceeds bounds");
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start + replacement.size(), data.data() + tailStart, data.size() - tailStart);
    }
    return storage;
}

auto U8StringModifyTools::replaced(const CpRange range, const U8StringDataView &text) const -> U8StringSharedStorage {
    return replaced(byteRangeForCharacterRange(_data, range), text);
}

auto U8StringModifyTools::removedFirst(const U8StringDataView &text, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    return replacedFirst(text, {}, compareFn);
}

auto U8StringModifyTools::replacedFirst(
    const U8StringDataView &text, const U8StringDataView &replacement, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    const auto range = findFirstTextRange(_data, text, compareFn);
    if (!range.isValid()) {
        return U8StringSharedStorage::fromBytes(_data.dataSpan(), _sensitive);
    }
    return replaced(range, replacement);
}

auto U8StringModifyTools::replacedText(
    const U8StringDataView &text, const std::span<const char> replacement, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return U8StringSharedStorage::fromBytes(data, _sensitive);
    }

    auto newSize = std::size_t{0};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            utf8::fastAdvanceChar(data, position);
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U8StringSharedStorage::validateSize(newSize);
    auto storage = U8StringSharedStorage::forSize(newSize, _sensitive);
    auto writePosition = std::size_t{0};
    position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            if (!replacement.empty()) {
                std::memcpy(storage.dataForWrite() + writePosition, replacement.data(), replacement.size());
                writePosition = U8StringSharedStorage::checkedAddSize(
                    writePosition, replacement.size(), "Modified string write exceeds size bounds");
            }
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            utf8::fastAdvanceChar(data, position);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            std::memcpy(storage.dataForWrite() + writePosition, data.data() + characterStart.toSizeT(), characterSize);
            writePosition = U8StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    return storage;
}

auto U8StringModifyTools::matchesText(
    const std::span<const char> data,
    const ByteIndex start,
    const std::span<const char> text,
    const CharCompareFn compareFn) noexcept -> bool {
    auto dataPosition = start;
    auto textPosition = ByteIndex::zero();
    while (textPosition.toSizeT() < text.size()) {
        if (dataPosition.toSizeT() >= data.size()) {
            return false;
        }
        const auto dataCharacter = utf8::decodeCharOrReplace(data, dataPosition);
        const auto textCharacter = utf8::decodeCharOrReplace(text, textPosition);
        if (!charactersEqual(dataCharacter, textCharacter, compareFn)) {
            return false;
        }
    }
    return true;
}

auto U8StringModifyTools::replaceTextInStorage(
    U8StringSharedStorage &storage,
    const U8StringDataView &text,
    const std::span<const char> replacement,
    const CharCompareFn compareFn) -> U8StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return storage;
    }
    if (!storage.isUniqueFullRange() || !replacement.empty()) {
        storage = U8StringModifyTools{dataView, storage.isSensitive()}.replacedText(text, replacement, compareFn);
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            hasMatch = true;
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            utf8::fastAdvanceChar(data, position);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            if (writePosition != characterStart.toSizeT()) {
                std::memmove(
                    storage.dataForWrite() + writePosition, data.data() + characterStart.toSizeT(), characterSize);
            }
            writePosition = U8StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    if (hasMatch) {
        storage.resize(writePosition);
    }
    return storage;
}

auto U8StringModifyTools::byteRangeForCharacterRange(const U8StringDataView &data, const CpRange range) noexcept
    -> ByteRange {
    if (!range.isValid()) {
        return ByteRange::noRange();
    }
    auto start = byteIndexForCharacterIndex(data, range.index());
    if (start.isNoIndex()) {
        start = ByteIndex::end(ByteLength::fromSizeT(data.dataSpan().size()));
    }
    if (range.length().isZero()) {
        return ByteRange::emptyAt(start);
    }
    if (range.length().isInfinite()) {
        return ByteRange{start, ByteLength::infinite()}.clampedTo(ByteLength::fromSizeT(data.dataSpan().size()));
    }
    auto end = range.endIndex();
    auto endByte = end.isNoIndex() ? ByteIndex::noIndex() : byteIndexForCharacterIndex(data, end);
    if (endByte.isNoIndex()) {
        endByte = ByteIndex::end(ByteLength::fromSizeT(data.dataSpan().size()));
    }
    return ByteRange{start, endByte}.clampedTo(ByteLength::fromSizeT(data.dataSpan().size()));
}

auto U8StringModifyTools::byteIndexForCharacterIndex(const U8StringDataView &data, const CpIndex index) noexcept
    -> ByteIndex {
    return U8StringCharReadTool{data}.byteIndexAt(index);
}

auto U8StringModifyTools::findFirstTextRange(
    const U8StringDataView &dataView, const U8StringDataView &text, const CharCompareFn compareFn) noexcept
    -> ByteRange {
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return ByteRange::noRange();
    }

    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto start = position;
        if (matchesText(data, start, needle, compareFn)) {
            return ByteRange{start, endOfMatch(data, start, needle)};
        }
        utf8::fastAdvanceChar(data, position);
    }
    return ByteRange::noRange();
}

auto U8StringModifyTools::endOfMatch(
    const std::span<const char> data, ByteIndex start, const std::span<const char> text) noexcept -> ByteIndex {
    auto textPosition = ByteIndex::zero();
    while (textPosition.toSizeT() < text.size() && start.toSizeT() < data.size()) {
        utf8::fastAdvanceChar(data, start);
        utf8::fastAdvanceChar(text, textPosition);
    }
    return start;
}

auto U8StringModifyTools::charactersEqual(const Char left, const Char right, const CharCompareFn compareFn) noexcept
    -> bool {
    if (compareFn == nullptr) {
        return left == right;
    }
    return compareFn(left, right) == std::strong_ordering::equal;
}

}
