// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringModifyTools.hpp"

#include "U16StringCharReadTool.hpp"
#include "U16StringComparisonTools.hpp"
#include "U16Writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

namespace erbsland::text::impl {

using namespace unit;

auto U16StringModifyTools::removedIfChanged(const CpRange range) const -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U16StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return std::nullopt;
    }

    const auto start = removeRange.beginIndex().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U16StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char16_t));
    }
    const auto tailStart = U16StringSharedStorage::checkedAddSize(start, count);
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start, data.data() + tailStart, (data.size() - tailStart) * sizeof(char16_t));
    }
    return storage;
}

auto U16StringModifyTools::removedAllIfChanged(const CharSet &characters) const
    -> std::optional<U16StringSharedStorage> {
    return replacedAllIfChanged(characters, U16StringDataView{});
}

auto U16StringModifyTools::removedAllIfChanged(const U16StringDataView &text, const CharCompareFn compareFn) const
    -> std::optional<U16StringSharedStorage> {
    return replacedTextIfChanged(text, {}, compareFn);
}

auto U16StringModifyTools::kept(const CpRange range) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U16StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return {};
    }
    // Keeping a range deliberately prunes the backing storage; do not return a shared subview.
    return U16StringSharedStorage::fromCodeUnits(
        data.subspan(keepRange.beginIndex().toSizeT(), keepRange.length().toSizeT()));
}

auto U16StringModifyTools::replacedAllIfChanged(const CharSet &characters, const Char replacement) const
    -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return std::nullopt;
    }
    const auto bytes = characterBytes(replacement);
    return replacedCharactersIfChanged(
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U16StringModifyTools::replacedAllIfChanged(const CharSet &characters, const U16StringDataView &replacement) const
    -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return std::nullopt;
    }
    return replacedCharactersIfChanged(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U16StringModifyTools::replacedAllIfChanged(
    const U16StringDataView &text, const U16StringDataView &replacement, const CharCompareFn compareFn) const
    -> std::optional<U16StringSharedStorage> {
    return replacedTextIfChanged(text, replacement.dataSpan(), compareFn);
}

auto U16StringModifyTools::removedIfChanged(const U16Range range) const -> std::optional<U16StringSharedStorage> {
    return replacedIfChanged(range, {});
}

auto U16StringModifyTools::kept(const U16Range range) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || !range.isValid()) {
        return {};
    }
    const auto keepRange = range.clampedTo(U16Count::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        return {};
    }
    // Keeping a range deliberately prunes the backing storage; do not return a shared subview.
    return U16StringSharedStorage::fromCodeUnits(
        data.subspan(keepRange.beginIndex().toSizeT(), keepRange.length().toSizeT()));
}

auto U16StringModifyTools::insertedIfChanged(const U16Index index, const U16StringDataView &text) const
    -> std::optional<U16StringSharedStorage> {
    if (index.isNoIndex() || text.dataSpan().empty()) {
        return std::nullopt;
    }
    const auto insertIndex = U16Index::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replacedIfChanged(U16Range::emptyAt(insertIndex), text);
}

auto U16StringModifyTools::insertedIfChanged(const CpIndex index, const U16StringDataView &text) const
    -> std::optional<U16StringSharedStorage> {
    if (index.isNoIndex() || text.dataSpan().empty()) {
        return std::nullopt;
    }
    auto dataIndex = dataIndexForCharacterIndex(_data, index);
    if (dataIndex.isNoIndex()) {
        dataIndex = U16Index::end(U16Count::fromSizeT(_data.dataSpan().size()));
    }
    return insertedIfChanged(dataIndex, text);
}

auto U16StringModifyTools::replacedIfChanged(const U16Range range, const U16StringDataView &text) const
    -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return std::nullopt;
    }
    const auto replaceRange = range.clampedTo(U16Count::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return std::nullopt;
    }

    const auto start = replaceRange.beginIndex().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    if (removeLength == replacement.size() &&
        (removeLength == 0U ||
            std::memcmp(data.data() + start, replacement.data(), removeLength * sizeof(char16_t)) == 0)) {
        return std::nullopt;
    }
    const auto newSize = U16StringSharedStorage::checkedAddSize(data.size() - removeLength, replacement.size());
    auto storage = U16StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char16_t));
    }
    if (!replacement.empty()) {
        std::memcpy(storage.dataForWrite() + start, replacement.data(), replacement.size() * sizeof(char16_t));
    }
    const auto tailStart = U16StringSharedStorage::checkedAddSize(start, removeLength);
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start + replacement.size(),
            data.data() + tailStart,
            (data.size() - tailStart) * sizeof(char16_t));
    }
    return storage;
}

auto U16StringModifyTools::replacedIfChanged(const CpRange range, const U16StringDataView &text) const
    -> std::optional<U16StringSharedStorage> {
    return replacedIfChanged(dataRangeForCharacterRange(_data, range), text);
}

auto U16StringModifyTools::removedFirstIfChanged(const U16StringDataView &text, const CharCompareFn compareFn) const
    -> std::optional<U16StringSharedStorage> {
    return replacedFirstIfChanged(text, {}, compareFn);
}

auto U16StringModifyTools::replacedFirstIfChanged(
    const U16StringDataView &text, const U16StringDataView &replacement, const CharCompareFn compareFn) const
    -> std::optional<U16StringSharedStorage> {
    const auto range = U16StringComparisonTools{_data}.findFirstRange(text, compareFn);
    if (!range.isValid()) {
        return std::nullopt;
    }
    return replacedIfChanged(range, replacement);
}

void U16StringModifyTools::appendData(U16StringSharedStorage &storage, const std::span<const char16_t> data) {
    if (data.empty()) {
        return;
    }
    const auto oldSize = storage.range().length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(oldSize, data.size());
    storage.ensureMutableCapacity(newSize);
    std::memcpy(storage.dataForWrite() + oldSize, data.data(), data.size() * sizeof(char16_t));
    storage.resize(newSize);
}

auto U16StringModifyTools::replacedTextIfChanged(
    const U16StringDataView &text, const std::span<const char16_t> replacement, const CharCompareFn compareFn) const
    -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return std::nullopt;
    }

    auto result = std::optional<U16StringSharedStorage>{};
    auto copyStart = std::size_t{0};
    auto position = U16Index::zero();
    while (position.toSizeT() < data.size()) {
        const auto matchEnd = U16StringComparisonTools::endOfTextMatch(data, position, needle, compareFn);
        if (matchEnd.isNoIndex()) {
            utf16::fastAdvanceChar(data, position);
            continue;
        }
        const auto begin = position.toSizeT();
        position = matchEnd;
        const auto matchSize = position.toSizeT() - begin;
        if (matchSize == replacement.size() &&
            std::memcmp(data.data() + begin, replacement.data(), matchSize * sizeof(char16_t)) == 0) {
            continue;
        }
        if (!result) {
            result.emplace();
            const auto initialCapacity =
                U16StringSharedStorage::checkedAddSize(data.size() - matchSize, replacement.size());
            if (initialCapacity > 0U) {
                result->ensureMutableCapacity(initialCapacity);
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

auto U16StringModifyTools::replaceTextInStorage(
    U16StringSharedStorage &storage,
    const U16StringDataView &text,
    const std::span<const char16_t> replacement,
    const CharCompareFn compareFn) -> U16StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return storage;
    }
    if (!storage.isUniqueFullRange() || !replacement.empty()) {
        if (auto result = U16StringModifyTools{dataView}.replacedTextIfChanged(text, replacement, compareFn)) {
            storage = std::move(result.value());
        }
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = U16Index::zero();
    while (position.toSizeT() < data.size()) {
        const auto matchEnd = U16StringComparisonTools::endOfTextMatch(data, position, needle, compareFn);
        if (!matchEnd.isNoIndex()) {
            hasMatch = true;
            position = matchEnd;
        } else {
            const auto characterStart = position;
            utf16::fastAdvanceChar(data, position);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            if (writePosition != characterStart.toSizeT()) {
                std::memmove(
                    storage.dataForWrite() + writePosition,
                    data.data() + characterStart.toSizeT(),
                    characterSize * sizeof(char16_t));
            }
            writePosition = U16StringSharedStorage::checkedAddSize(writePosition, characterSize);
        }
    }
    if (hasMatch) {
        storage.resize(writePosition);
    }
    return storage;
}

auto U16StringModifyTools::dataRangeForCharacterRange(const U16StringDataView &data, const CpRange range) noexcept
    -> U16Range {
    if (!range.isValid()) {
        return U16Range::noRange();
    }
    auto start = dataIndexForCharacterIndex(data, range.beginIndex());
    if (start.isNoIndex()) {
        start = U16Index::end(U16Count::fromSizeT(data.dataSpan().size()));
    }
    if (range.length().isZero()) {
        return U16Range::emptyAt(start);
    }
    if (range.length().isInfinite()) {
        return U16Range{start, U16Count::infinite()}.clampedTo(U16Count::fromSizeT(data.dataSpan().size()));
    }
    auto end = range.endIndex();
    auto endData = end.isNoIndex() ? U16Index::noIndex() : dataIndexForCharacterIndex(data, end);
    if (endData.isNoIndex()) {
        endData = U16Index::end(U16Count::fromSizeT(data.dataSpan().size()));
    }
    return U16Range{start, endData}.clampedTo(U16Count::fromSizeT(data.dataSpan().size()));
}

auto U16StringModifyTools::dataIndexForCharacterIndex(const U16StringDataView &data, const CpIndex index) noexcept
    -> U16Index {
    return U16StringCharReadTool{data}.byteIndexAt(index);
}

}
