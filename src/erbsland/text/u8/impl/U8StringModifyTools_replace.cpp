// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringModifyTools.hpp"

#include "U8StringCharReadTool.hpp"
#include "U8StringComparisonTools.hpp"
#include "U8Writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

namespace erbsland::text::impl {

using namespace unit;

auto U8StringModifyTools::removedIfChanged(const CpRange range) const -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U8StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return std::nullopt;
    }

    const auto start = removeRange.beginIndex().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U8StringSharedStorage::forSize(newSize, _sensitive);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start);
    }
    const auto tailStart = U8StringSharedStorage::checkedAddSize(start, count);
    if (tailStart < data.size()) {
        std::memcpy(storage.dataForWrite() + start, data.data() + tailStart, data.size() - tailStart);
    }
    return storage;
}

auto U8StringModifyTools::removedAllIfChanged(const CharSet &characters) const -> std::optional<U8StringSharedStorage> {
    return replacedAllIfChanged(characters, U8StringDataView{});
}

auto U8StringModifyTools::removedAllIfChanged(const U8StringDataView &text, const CharCompareFn compareFn) const
    -> std::optional<U8StringSharedStorage> {
    return replacedTextIfChanged(text, {}, compareFn);
}

auto U8StringModifyTools::kept(const CpRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U8StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return U8StringSharedStorage::forSize(0U, _sensitive);
    }
    // Keeping a range deliberately prunes the backing storage; do not return a shared subview.
    return U8StringSharedStorage::fromBytes(
        data.subspan(keepRange.beginIndex().toSizeT(), keepRange.length().toSizeT()), _sensitive);
}

auto U8StringModifyTools::replacedAllIfChanged(const CharSet &characters, const Char replacement) const
    -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return std::nullopt;
    }
    const auto bytes = characterBytes(replacement);
    return replacedCharactersIfChanged(
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U8StringModifyTools::replacedAllIfChanged(const CharSet &characters, const U8StringDataView &replacement) const
    -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return std::nullopt;
    }
    return replacedCharactersIfChanged(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U8StringModifyTools::replacedAllIfChanged(
    const U8StringDataView &text, const U8StringDataView &replacement, const CharCompareFn compareFn) const
    -> std::optional<U8StringSharedStorage> {
    return replacedTextIfChanged(text, replacement.dataSpan(), compareFn);
}

auto U8StringModifyTools::removedIfChanged(const ByteRange range) const -> std::optional<U8StringSharedStorage> {
    return replacedIfChanged(range, {});
}

auto U8StringModifyTools::kept(const ByteRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || !range.isValid()) {
        return U8StringSharedStorage::forSize(0U, _sensitive);
    }
    const auto keepRange = range.clampedTo(ByteCount::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        return U8StringSharedStorage::forSize(0U, _sensitive);
    }
    // Keeping a range deliberately prunes the backing storage; do not return a shared subview.
    return U8StringSharedStorage::fromBytes(
        data.subspan(keepRange.beginIndex().toSizeT(), keepRange.length().toSizeT()), _sensitive);
}

auto U8StringModifyTools::insertedIfChanged(const ByteIndex index, const U8StringDataView &text) const
    -> std::optional<U8StringSharedStorage> {
    if (index.isNoIndex() || text.dataSpan().empty()) {
        return std::nullopt;
    }
    const auto insertIndex = ByteIndex::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replacedIfChanged(ByteRange::emptyAt(insertIndex), text);
}

auto U8StringModifyTools::insertedIfChanged(const CpIndex index, const U8StringDataView &text) const
    -> std::optional<U8StringSharedStorage> {
    if (index.isNoIndex() || text.dataSpan().empty()) {
        return std::nullopt;
    }
    auto byteIndex = byteIndexForCharacterIndex(_data, index);
    if (byteIndex.isNoIndex()) {
        byteIndex = ByteIndex::end(ByteCount::fromSizeT(_data.dataSpan().size()));
    }
    return insertedIfChanged(byteIndex, text);
}

auto U8StringModifyTools::replacedIfChanged(const ByteRange range, const U8StringDataView &text) const
    -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return std::nullopt;
    }
    const auto replaceRange = range.clampedTo(ByteCount::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return std::nullopt;
    }

    const auto start = replaceRange.beginIndex().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    if (removeLength == replacement.size() &&
        (removeLength == 0U ||
            std::memcmp(data.data() + start, replacement.data(), removeLength * sizeof(char)) == 0)) {
        return std::nullopt;
    }
    const auto newSize = U8StringSharedStorage::checkedAddSize(data.size() - removeLength, replacement.size());
    auto storage = U8StringSharedStorage::forSize(newSize, _sensitive);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start);
    }
    if (!replacement.empty()) {
        std::memcpy(storage.dataForWrite() + start, replacement.data(), replacement.size());
    }
    const auto tailStart = U8StringSharedStorage::checkedAddSize(start, removeLength);
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start + replacement.size(), data.data() + tailStart, data.size() - tailStart);
    }
    return storage;
}

auto U8StringModifyTools::replacedIfChanged(const CpRange range, const U8StringDataView &text) const
    -> std::optional<U8StringSharedStorage> {
    return replacedIfChanged(byteRangeForCharacterRange(_data, range), text);
}

auto U8StringModifyTools::removedFirstIfChanged(const U8StringDataView &text, const CharCompareFn compareFn) const
    -> std::optional<U8StringSharedStorage> {
    return replacedFirstIfChanged(text, {}, compareFn);
}

auto U8StringModifyTools::replacedFirstIfChanged(
    const U8StringDataView &text, const U8StringDataView &replacement, const CharCompareFn compareFn) const
    -> std::optional<U8StringSharedStorage> {
    const auto range = U8StringComparisonTools{_data}.findFirstRange(text, compareFn);
    if (!range.isValid()) {
        return std::nullopt;
    }
    return replacedIfChanged(range, replacement);
}

void U8StringModifyTools::appendData(U8StringSharedStorage &storage, const std::span<const char> data) {
    if (data.empty()) {
        return;
    }
    const auto oldSize = storage.range().length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(oldSize, data.size());
    storage.ensureMutableCapacity(newSize);
    std::memcpy(storage.dataForWrite() + oldSize, data.data(), data.size() * sizeof(char));
    storage.resize(newSize);
}

auto U8StringModifyTools::replacedTextIfChanged(
    const U8StringDataView &text, const std::span<const char> replacement, const CharCompareFn compareFn) const
    -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return std::nullopt;
    }

    auto result = std::optional<U8StringSharedStorage>{};
    auto copyStart = std::size_t{0};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto matchEnd = U8StringComparisonTools::endOfTextMatch(data, position, needle, compareFn);
        if (matchEnd.isNoIndex()) {
            utf8::fastAdvanceChar(data, position);
            continue;
        }
        const auto begin = position.toSizeT();
        position = matchEnd;
        const auto matchSize = position.toSizeT() - begin;
        if (matchSize == replacement.size() &&
            std::memcmp(data.data() + begin, replacement.data(), matchSize * sizeof(char)) == 0) {
            continue;
        }
        if (!result) {
            result.emplace();
            const auto initialCapacity =
                U8StringSharedStorage::checkedAddSize(data.size() - matchSize, replacement.size());
            if (initialCapacity > 0U) {
                result->ensureMutableCapacity(initialCapacity);
            }
            if (_sensitive) {
                result->markAsSensitive();
            }
        }
        appendData(result.value(), data.subspan(copyStart, begin - copyStart));
        appendData(result.value(), replacement);
        copyStart = position.toSizeT();
    }
    if (result) {
        appendData(result.value(), data.subspan(copyStart));
    }
    return result;
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
        if (auto result = U8StringModifyTools{dataView, storage.isSensitive()}.replacedTextIfChanged(
                text, replacement, compareFn)) {
            storage = std::move(result.value());
        }
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto matchEnd = U8StringComparisonTools::endOfTextMatch(data, position, needle, compareFn);
        if (!matchEnd.isNoIndex()) {
            hasMatch = true;
            position = matchEnd;
        } else {
            const auto characterStart = position;
            utf8::fastAdvanceChar(data, position);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            if (writePosition != characterStart.toSizeT()) {
                std::memmove(
                    storage.dataForWrite() + writePosition, data.data() + characterStart.toSizeT(), characterSize);
            }
            writePosition = U8StringSharedStorage::checkedAddSize(writePosition, characterSize);
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
    auto start = byteIndexForCharacterIndex(data, range.beginIndex());
    if (start.isNoIndex()) {
        start = ByteIndex::end(ByteCount::fromSizeT(data.dataSpan().size()));
    }
    if (range.length().isZero()) {
        return ByteRange::emptyAt(start);
    }
    if (range.length().isInfinite()) {
        return ByteRange{start, ByteCount::infinite()}.clampedTo(ByteCount::fromSizeT(data.dataSpan().size()));
    }
    auto end = range.endIndex();
    auto endByte = end.isNoIndex() ? ByteIndex::noIndex() : byteIndexForCharacterIndex(data, end);
    if (endByte.isNoIndex()) {
        endByte = ByteIndex::end(ByteCount::fromSizeT(data.dataSpan().size()));
    }
    return ByteRange{start, endByte}.clampedTo(ByteCount::fromSizeT(data.dataSpan().size()));
}

auto U8StringModifyTools::byteIndexForCharacterIndex(const U8StringDataView &data, const CpIndex index) noexcept
    -> ByteIndex {
    return U8StringCharReadTool{data}.byteIndexAt(index);
}

}
