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

auto U8StringModifyTools::characterBytes(const Char character) noexcept -> std::array<char, 4> {
    auto result = std::array<char, 4>{};
    U8Writer writer{std::span<char>{result}};
    writer.write(character);
    return result;
}

auto U8StringModifyTools::characterByteSpan(const Char character, const std::array<char, 4> &bytes) noexcept
    -> std::span<const char> {
    if (!character.isValidUnicode()) {
        return {};
    }
    return std::span<const char>{bytes.data(), utf8::encodedLength(character).toSizeT()};
}

template <typename T>
auto U8StringModifyTools::spansOverlap(const std::span<const T> first, const std::span<const T> second) noexcept
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

auto U8StringModifyTools::remove(U8StringSharedStorage &storage, const ByteRange range) -> U8StringSharedStorage & {
    return replace(storage, range, {});
}

auto U8StringModifyTools::remove(U8StringSharedStorage &storage, const CpRange range) -> U8StringSharedStorage & {
    return remove(storage, byteRangeForCharacterRange(storage.dataView(), range));
}

auto U8StringModifyTools::keep(U8StringSharedStorage &storage, const ByteRange range) -> U8StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }
    if (!range.isValid()) {
        storage.clear();
        return storage;
    }
    const auto keepRange = range.clampedTo(ByteLength::fromSizeT(data.size()));
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
        std::memmove(storage.dataForWrite(), storage.dataForWrite() + start, count);
    }
    storage.resize(count);
    return storage;
}

auto U8StringModifyTools::keep(U8StringSharedStorage &storage, const CpRange range) -> U8StringSharedStorage & {
    return keep(storage, byteRangeForCharacterRange(storage.dataView(), range));
}

auto U8StringModifyTools::insert(U8StringSharedStorage &storage, const ByteIndex index, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    const auto insertIndex = ByteIndex::fromSizeT(std::min(index.toSizeT(), storage.dataView().dataSpan().size()));
    return replace(storage, ByteRange::emptyAt(insertIndex), text);
}

auto U8StringModifyTools::insert(U8StringSharedStorage &storage, const CpIndex index, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    auto byteIndex = byteIndexForCharacterIndex(storage.dataView(), index);
    if (byteIndex.isNoIndex()) {
        byteIndex = ByteIndex::end(ByteLength::fromSizeT(storage.dataView().dataSpan().size()));
    }
    return insert(storage, byteIndex, text);
}

auto U8StringModifyTools::replace(U8StringSharedStorage &storage, const ByteRange range, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    if (!range.isValid()) {
        return storage;
    }

    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto replacement = text.dataSpan();
    const auto replaceRange = range.clampedTo(ByteLength::fromSizeT(data.size()));
    if (replaceRange.isEmpty() && replacement.empty()) {
        return storage;
    }

    auto replacementCopy = std::string{};
    auto replacementData = replacement;
    if (spansOverlap(data, replacementData)) {
        replacementCopy.assign(replacementData.begin(), replacementData.end());
        replacementData = std::span<const char>{replacementCopy.data(), replacementCopy.size()};
    }

    const auto oldSize = data.size();
    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto replacementLength = replacementData.size();
    const auto newSize = U8StringSharedStorage::checkedAddSize(
        oldSize - removeLength, replacementLength, "Modified string exceeds size bounds");
    storage.ensureMutableCapacity(newSize);

    auto *writeData = storage.dataForWrite();
    const auto tailStart = U8StringSharedStorage::checkedAddSize(start, removeLength, "Replace range exceeds bounds");
    const auto tailLength = oldSize - tailStart;
    if (replacementLength != removeLength && tailLength > 0U) {
        std::memmove(writeData + start + replacementLength, writeData + tailStart, tailLength);
    }
    if (!replacementData.empty()) {
        std::memcpy(writeData + start, replacementData.data(), replacementLength);
    }
    storage.resize(newSize);
    return storage;
}

auto U8StringModifyTools::replace(U8StringSharedStorage &storage, const CpRange range, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    return replace(storage, byteRangeForCharacterRange(storage.dataView(), range), text);
}

auto U8StringModifyTools::removeAll(U8StringSharedStorage &storage, const CharSet &characters)
    -> U8StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    return replaceCharactersInStorage(
        storage, [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U8StringModifyTools::remove(
    U8StringSharedStorage &storage, const U8StringDataView &text, const CharCompareFn compareFn)
    -> U8StringSharedStorage & {
    return replaceTextInStorage(storage, text, {}, compareFn);
}

auto U8StringModifyTools::removeFirst(
    U8StringSharedStorage &storage, const U8StringDataView &text, const CharCompareFn compareFn)
    -> U8StringSharedStorage & {
    return replaceFirst(storage, text, {}, compareFn);
}

auto U8StringModifyTools::replaceFirst(
    U8StringSharedStorage &storage,
    const U8StringDataView &text,
    const U8StringDataView &replacement,
    const CharCompareFn compareFn) -> U8StringSharedStorage & {
    const auto range = findFirstTextRange(storage.dataView(), text, compareFn);
    if (!range.isValid()) {
        return storage;
    }
    return replace(storage, range, replacement);
}

auto U8StringModifyTools::replaceAll(U8StringSharedStorage &storage, const CharSet &characters, const Char replacement)
    -> U8StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    const auto bytes = characterBytes(replacement);
    return replaceCharactersInStorage(
        storage,
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U8StringModifyTools::replaceAll(
    U8StringSharedStorage &storage, const CharSet &characters, const U8StringDataView &replacement)
    -> U8StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    return replaceCharactersInStorage(
        storage,
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        replacement.dataSpan());
}

auto U8StringModifyTools::replaceAll(
    U8StringSharedStorage &storage,
    const U8StringDataView &text,
    const U8StringDataView &replacement,
    const CharCompareFn compareFn) -> U8StringSharedStorage & {
    return replaceTextInStorage(storage, text, replacement.dataSpan(), compareFn);
}

}
