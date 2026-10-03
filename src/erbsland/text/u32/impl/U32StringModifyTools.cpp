// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringModifyTools.hpp"

#include "U32StringComparisonTools.hpp"
#include "U32StringReadTools.hpp"
#include "U32Writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

namespace erbsland::text::impl {

using unit::CpCount;
using unit::CpIndex;
using unit::CpRange;

auto U32StringModifyTools::characterBytes(const Char character) noexcept -> std::array<char32_t, 2> {
    auto result = std::array<char32_t, 2>{};
    U32Writer writer{std::span<char32_t>{result}};
    writer.write(character);
    return result;
}

auto U32StringModifyTools::characterByteSpan(const Char character, const std::array<char32_t, 2> &bytes) noexcept
    -> std::span<const char32_t> {
    if (!character.isValidUnicode()) {
        return {};
    }
    return std::span<const char32_t>{bytes.data(), utf32::encodedLength(character).toSizeT()};
}

template <typename T>
auto U32StringModifyTools::spansOverlap(const std::span<const T> first, const std::span<const T> second) noexcept
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

auto U32StringModifyTools::remove(U32StringSharedStorage &storage, const CpRange range) -> U32StringSharedStorage & {
    return replace(storage, range, {});
}

auto U32StringModifyTools::removeAll(U32StringSharedStorage &storage, const CharSet &characters)
    -> U32StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    return replaceCharactersInStorage(
        storage, [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U32StringModifyTools::remove(
    U32StringSharedStorage &storage, const U32StringDataView &text, const CharCompareFn compareFn)
    -> U32StringSharedStorage & {
    return replaceTextInStorage(storage, text, {}, compareFn);
}

auto U32StringModifyTools::keep(U32StringSharedStorage &storage, const CpRange range) -> U32StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }
    if (!range.isValid()) {
        storage.clear();
        return storage;
    }
    const auto keepRange = range.clampedTo(CpCount::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        storage.clear();
        return storage;
    }
    if (keepRange.beginIndex().isZero() && keepRange.length().toSizeT() == data.size()) {
        return storage;
    }
    const auto start = keepRange.beginIndex().toSizeT();
    const auto count = keepRange.length().toSizeT();
    storage.ensureMutableCapacity(count);
    if (start > 0U) {
        std::memmove(storage.dataForWrite(), storage.dataForWrite() + start, count * sizeof(char32_t));
    }
    storage.resize(count);
    return storage;
}

auto U32StringModifyTools::insert(U32StringSharedStorage &storage, const CpIndex index, const U32StringDataView &text)
    -> U32StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    const auto insertIndex = CpIndex::fromSizeT(std::min(index.toSizeT(), storage.dataView().dataSpan().size()));
    return replace(storage, CpRange::emptyAt(insertIndex), text);
}

auto U32StringModifyTools::replace(U32StringSharedStorage &storage, const CpRange range, const U32StringDataView &text)
    -> U32StringSharedStorage & {
    if (!range.isValid()) {
        return storage;
    }

    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto replacement = text.dataSpan();
    const auto replaceRange = range.clampedTo(CpCount::fromSizeT(data.size()));
    if (replaceRange.isEmpty() && replacement.empty()) {
        return storage;
    }

    auto replacementCopy = std::u32string{};
    auto replacementData = replacement;
    if (spansOverlap(data, replacementData)) {
        replacementCopy.assign(replacementData.begin(), replacementData.end());
        replacementData = std::span<const char32_t>{replacementCopy.data(), replacementCopy.size()};
    }

    const auto oldSize = data.size();
    const auto start = replaceRange.beginIndex().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto replacementLength = replacementData.size();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize - removeLength, replacementLength);
    storage.ensureMutableCapacity(newSize);

    auto *writeData = storage.dataForWrite();
    const auto tailStart = U32StringSharedStorage::checkedAddSize(start, removeLength);
    const auto tailLength = oldSize - tailStart;
    if (replacementLength != removeLength && tailLength > 0U) {
        std::memmove(writeData + start + replacementLength, writeData + tailStart, tailLength * sizeof(char32_t));
    }
    if (!replacementData.empty()) {
        std::memcpy(writeData + start, replacementData.data(), replacementLength * sizeof(char32_t));
    }
    storage.resize(newSize);
    return storage;
}

auto U32StringModifyTools::removeFirst(
    U32StringSharedStorage &storage, const U32StringDataView &text, const CharCompareFn compareFn)
    -> U32StringSharedStorage & {
    return replaceFirst(storage, text, {}, compareFn);
}

auto U32StringModifyTools::replaceFirst(
    U32StringSharedStorage &storage,
    const U32StringDataView &text,
    const U32StringDataView &replacement,
    const CharCompareFn compareFn) -> U32StringSharedStorage & {
    const auto range = U32StringComparisonTools{storage.dataView()}.findFirstRange(text, compareFn);
    if (!range.isValid()) {
        return storage;
    }
    return replace(storage, range, replacement);
}

auto U32StringModifyTools::replaceAll(
    U32StringSharedStorage &storage, const CharSet &characters, const Char replacement) -> U32StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    const auto bytes = characterBytes(replacement);
    return replaceCharactersInStorage(
        storage,
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U32StringModifyTools::replaceAll(
    U32StringSharedStorage &storage, const CharSet &characters, const U32StringDataView &replacement)
    -> U32StringSharedStorage & {
    if (characters.isEmpty()) {
        return storage;
    }
    return replaceCharactersInStorage(
        storage,
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        replacement.dataSpan());
}

auto U32StringModifyTools::replaceAll(
    U32StringSharedStorage &storage,
    const U32StringDataView &text,
    const U32StringDataView &replacement,
    const CharCompareFn compareFn) -> U32StringSharedStorage & {
    return replaceTextInStorage(storage, text, replacement.dataSpan(), compareFn);
}

auto U32StringModifyTools::removedIfChanged(const CpRange range) const -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U32StringReadTools{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return std::nullopt;
    }

    const auto start = removeRange.beginIndex().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U32StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char32_t));
    }
    const auto tailStart = U32StringSharedStorage::checkedAddSize(start, count);
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start, data.data() + tailStart, (data.size() - tailStart) * sizeof(char32_t));
    }
    return storage;
}

auto U32StringModifyTools::removedAllIfChanged(const CharSet &characters) const
    -> std::optional<U32StringSharedStorage> {
    return replacedAllIfChanged(characters, U32StringDataView{});
}

auto U32StringModifyTools::removedAllIfChanged(const U32StringDataView &text, const CharCompareFn compareFn) const
    -> std::optional<U32StringSharedStorage> {
    return replacedTextIfChanged(text, {}, compareFn);
}

auto U32StringModifyTools::kept(const CpRange range) const -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U32StringReadTools{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return {};
    }
    // Keeping a range deliberately prunes the backing storage; do not return a shared subview.
    return U32StringSharedStorage::fromCodeUnits(
        data.subspan(keepRange.beginIndex().toSizeT(), keepRange.length().toSizeT()));
}

auto U32StringModifyTools::replacedAllIfChanged(const CharSet &characters, const Char replacement) const
    -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return std::nullopt;
    }
    const auto bytes = characterBytes(replacement);
    return replacedCharactersIfChanged(
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U32StringModifyTools::replacedAllIfChanged(const CharSet &characters, const U32StringDataView &replacement) const
    -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return std::nullopt;
    }
    return replacedCharactersIfChanged(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U32StringModifyTools::replacedAllIfChanged(
    const U32StringDataView &text, const U32StringDataView &replacement, const CharCompareFn compareFn) const
    -> std::optional<U32StringSharedStorage> {
    return replacedTextIfChanged(text, replacement.dataSpan(), compareFn);
}

auto U32StringModifyTools::insertedIfChanged(const CpIndex index, const U32StringDataView &text) const
    -> std::optional<U32StringSharedStorage> {
    if (index.isNoIndex() || text.dataSpan().empty()) {
        return std::nullopt;
    }
    const auto insertIndex = CpIndex::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replacedIfChanged(CpRange::emptyAt(insertIndex), text);
}

auto U32StringModifyTools::replacedIfChanged(const CpRange range, const U32StringDataView &text) const
    -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return std::nullopt;
    }
    const auto replaceRange = range.clampedTo(CpCount::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return std::nullopt;
    }

    const auto start = replaceRange.beginIndex().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    if (removeLength == replacement.size() &&
        (removeLength == 0U ||
            std::memcmp(data.data() + start, replacement.data(), removeLength * sizeof(char32_t)) == 0)) {
        return std::nullopt;
    }
    const auto newSize = U32StringSharedStorage::checkedAddSize(data.size() - removeLength, replacement.size());
    auto storage = U32StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char32_t));
    }
    if (!replacement.empty()) {
        std::memcpy(storage.dataForWrite() + start, replacement.data(), replacement.size() * sizeof(char32_t));
    }
    const auto tailStart = U32StringSharedStorage::checkedAddSize(start, removeLength);
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start + replacement.size(),
            data.data() + tailStart,
            (data.size() - tailStart) * sizeof(char32_t));
    }
    return storage;
}

auto U32StringModifyTools::removedFirstIfChanged(const U32StringDataView &text, const CharCompareFn compareFn) const
    -> std::optional<U32StringSharedStorage> {
    return replacedFirstIfChanged(text, {}, compareFn);
}

auto U32StringModifyTools::replacedFirstIfChanged(
    const U32StringDataView &text, const U32StringDataView &replacement, const CharCompareFn compareFn) const
    -> std::optional<U32StringSharedStorage> {
    const auto range = U32StringComparisonTools{_data}.findFirstRange(text, compareFn);
    if (!range.isValid()) {
        return std::nullopt;
    }
    return replacedIfChanged(range, replacement);
}

void U32StringModifyTools::appendData(U32StringSharedStorage &storage, const std::span<const char32_t> data) {
    if (data.empty()) {
        return;
    }
    const auto oldSize = storage.range().length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(oldSize, data.size());
    storage.ensureMutableCapacity(newSize);
    std::memcpy(storage.dataForWrite() + oldSize, data.data(), data.size() * sizeof(char32_t));
    storage.resize(newSize);
}

auto U32StringModifyTools::replacedTextIfChanged(
    const U32StringDataView &text, const std::span<const char32_t> replacement, const CharCompareFn compareFn) const
    -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return std::nullopt;
    }

    auto result = std::optional<U32StringSharedStorage>{};
    auto copyStart = std::size_t{0};
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto matchEnd = U32StringComparisonTools::endOfTextMatch(data, position, needle, compareFn);
        if (matchEnd.isNoIndex()) {
            utf32::fastAdvanceChar(data, position);
            continue;
        }
        const auto begin = position.toSizeT();
        position = matchEnd;
        const auto matchSize = position.toSizeT() - begin;
        if (matchSize == replacement.size() &&
            std::memcmp(data.data() + begin, replacement.data(), matchSize * sizeof(char32_t)) == 0) {
            continue;
        }
        if (!result) {
            result.emplace();
            const auto initialCapacity =
                U32StringSharedStorage::checkedAddSize(data.size() - matchSize, replacement.size());
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

auto U32StringModifyTools::replaceTextInStorage(
    U32StringSharedStorage &storage,
    const U32StringDataView &text,
    const std::span<const char32_t> replacement,
    const CharCompareFn compareFn) -> U32StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return storage;
    }
    if (!storage.isUniqueFullRange() || !replacement.empty()) {
        if (auto result = U32StringModifyTools{dataView}.replacedTextIfChanged(text, replacement, compareFn)) {
            storage = std::move(result.value());
        }
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto matchEnd = U32StringComparisonTools::endOfTextMatch(data, position, needle, compareFn);
        if (!matchEnd.isNoIndex()) {
            hasMatch = true;
            position = matchEnd;
        } else {
            const auto characterStart = position;
            utf32::fastAdvanceChar(data, position);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            if (writePosition != characterStart.toSizeT()) {
                std::memmove(
                    storage.dataForWrite() + writePosition,
                    data.data() + characterStart.toSizeT(),
                    characterSize * sizeof(char32_t));
            }
            writePosition = U32StringSharedStorage::checkedAddSize(writePosition, characterSize);
        }
    }
    if (hasMatch) {
        storage.resize(writePosition);
    }
    return storage;
}

}
