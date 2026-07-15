// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringModifyTools.hpp"

#include "U16StringCharReadTool.hpp"
#include "U16Writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace erbsland::text::impl {

auto U16StringModifyTools::characterBytes(const Char character) noexcept -> std::array<char16_t, 2> {
    auto result = std::array<char16_t, 2>{};
    U16Writer writer{std::span<char16_t>{result}};
    writer.write(character);
    return result;
}

auto U16StringModifyTools::characterByteSpan(const Char character, const std::array<char16_t, 2> &bytes) noexcept
    -> std::span<const char16_t> {
    if (!character.isValidUnicode()) {
        return {};
    }
    return std::span<const char16_t>{bytes.data(), utf16::encodedLength(character).toSizeT()};
}

template <typename T>
auto U16StringModifyTools::spansOverlap(const std::span<const T> first, const std::span<const T> second) noexcept
    -> bool {
    if (first.empty() || second.empty()) {
        return false;
    }
    const auto firstBegin = reinterpret_cast<std::uintptr_t>(first.data());
    const auto firstEnd = firstBegin + first.size() * sizeof(T);
    const auto secondBegin = reinterpret_cast<std::uintptr_t>(second.data());
    const auto secondEnd = secondBegin + second.size() * sizeof(T);
    return firstBegin < secondEnd && secondBegin < firstEnd;
}

auto U16StringModifyTools::remove(U16StringSharedStorage &storage, const unit::U16DataRange range)
    -> U16StringSharedStorage & {
    return replace(storage, range, {});
}

auto U16StringModifyTools::remove(U16StringSharedStorage &storage, const unit::CpRange range)
    -> U16StringSharedStorage & {
    return remove(storage, dataRangeForCharacterRange(storage.dataView(), range));
}

auto U16StringModifyTools::keep(U16StringSharedStorage &storage, const unit::U16DataRange range)
    -> U16StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }
    if (!range.isValid()) {
        storage.clear();
        return storage;
    }
    const auto keepRange = range.clampedTo(unit::U16DataLength::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        storage.clear();
        return storage;
    }
    if (keepRange.index().isZero() && keepRange.length().toSizeT() == data.size()) {
        return storage;
    }
    const auto start = keepRange.index().toSizeT();
    const auto count = keepRange.length().toSizeT();
    storage.ensureMutableCapacity(count);
    if (start > 0U) {
        std::memmove(storage.dataForWrite(), storage.dataForWrite() + start, count * sizeof(char16_t));
    }
    storage.resize(count);
    return storage;
}

auto U16StringModifyTools::keep(U16StringSharedStorage &storage, const unit::CpRange range)
    -> U16StringSharedStorage & {
    return keep(storage, dataRangeForCharacterRange(storage.dataView(), range));
}

auto U16StringModifyTools::insert(
    U16StringSharedStorage &storage, const unit::U16DataIndex index, const U16StringDataView &text)
    -> U16StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    const auto insertIndex =
        unit::U16DataIndex::fromSizeT(std::min(index.toSizeT(), storage.dataView().dataSpan().size()));
    return replace(storage, unit::U16DataRange::emptyAt(insertIndex), text);
}

auto U16StringModifyTools::insert(
    U16StringSharedStorage &storage, const unit::CpIndex index, const U16StringDataView &text)
    -> U16StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    auto dataIndex = dataIndexForCharacterIndex(storage.dataView(), index);
    if (dataIndex.isNoIndex()) {
        dataIndex = unit::U16DataIndex::end(unit::U16DataLength::fromSizeT(storage.dataView().dataSpan().size()));
    }
    return insert(storage, dataIndex, text);
}

auto U16StringModifyTools::replace(
    U16StringSharedStorage &storage, const unit::U16DataRange range, const U16StringDataView &text)
    -> U16StringSharedStorage & {
    if (!range.isValid()) {
        return storage;
    }

    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto replacement = text.dataSpan();
    const auto replaceRange = range.clampedTo(unit::U16DataLength::fromSizeT(data.size()));
    if (replaceRange.isEmpty() && replacement.empty()) {
        return storage;
    }

    auto replacementCopy = std::u16string{};
    auto replacementData = replacement;
    if (spansOverlap(data, replacementData)) {
        replacementCopy.assign(replacementData.begin(), replacementData.end());
        replacementData = std::span<const char16_t>{replacementCopy.data(), replacementCopy.size()};
    }

    const auto oldSize = data.size();
    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto replacementLength = replacementData.size();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        oldSize - removeLength, replacementLength, "Modified string exceeds size bounds");
    storage.ensureMutableCapacity(newSize);

    auto *writeData = storage.dataForWrite();
    const auto tailStart = U16StringSharedStorage::checkedAddSize(start, removeLength, "Replace range exceeds bounds");
    const auto tailLength = oldSize - tailStart;
    if (replacementLength != removeLength && tailLength > 0U) {
        std::memmove(writeData + start + replacementLength, writeData + tailStart, tailLength * sizeof(char16_t));
    }
    if (!replacementData.empty()) {
        std::memcpy(writeData + start, replacementData.data(), replacementLength * sizeof(char16_t));
    }
    storage.resize(newSize);
    return storage;
}

auto U16StringModifyTools::replace(
    U16StringSharedStorage &storage, const unit::CpRange range, const U16StringDataView &text)
    -> U16StringSharedStorage & {
    return replace(storage, dataRangeForCharacterRange(storage.dataView(), range), text);
}

auto U16StringModifyTools::removeAll(U16StringSharedStorage &storage, const CharSet &characters)
    -> U16StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    return replaceCharactersInStorage(
        storage, [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U16StringModifyTools::remove(
    U16StringSharedStorage &storage, const U16StringDataView &text, const CharCompareFn compareFn)
    -> U16StringSharedStorage & {
    return replaceTextInStorage(storage, text, {}, compareFn);
}

auto U16StringModifyTools::removeFirst(
    U16StringSharedStorage &storage, const U16StringDataView &text, const CharCompareFn compareFn)
    -> U16StringSharedStorage & {
    return replaceFirst(storage, text, {}, compareFn);
}

auto U16StringModifyTools::replaceFirst(
    U16StringSharedStorage &storage,
    const U16StringDataView &text,
    const U16StringDataView &replacement,
    const CharCompareFn compareFn) -> U16StringSharedStorage & {
    const auto range = findFirstTextRange(storage.dataView(), text, compareFn);
    if (!range.isValid()) {
        return storage;
    }
    return replace(storage, range, replacement);
}

auto U16StringModifyTools::replaceAll(
    U16StringSharedStorage &storage, const CharSet &characters, const Char replacement) -> U16StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    const auto bytes = characterBytes(replacement);
    return replaceCharactersInStorage(
        storage,
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U16StringModifyTools::replaceAll(
    U16StringSharedStorage &storage, const CharSet &characters, const U16StringDataView &replacement)
    -> U16StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    return replaceCharactersInStorage(
        storage,
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        replacement.dataSpan());
}

auto U16StringModifyTools::replaceAll(
    U16StringSharedStorage &storage,
    const U16StringDataView &text,
    const U16StringDataView &replacement,
    const CharCompareFn compareFn) -> U16StringSharedStorage & {
    return replaceTextInStorage(storage, text, replacement.dataSpan(), compareFn);
}

}
