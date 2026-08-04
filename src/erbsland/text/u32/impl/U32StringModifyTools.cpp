// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringModifyTools.hpp"

#include "U32StringReadTools.hpp"
#include "U32Writer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace erbsland::text::impl {

using unit::CpIndex;
using unit::CpLength;
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
    const auto keepRange = range.clampedTo(CpLength::fromSizeT(data.size()));
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
    const auto replaceRange = range.clampedTo(CpLength::fromSizeT(data.size()));
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
    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto replacementLength = replacementData.size();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        oldSize - removeLength, replacementLength, "Modified string exceeds size bounds");
    storage.ensureMutableCapacity(newSize);

    auto *writeData = storage.dataForWrite();
    const auto tailStart = U32StringSharedStorage::checkedAddSize(start, removeLength, "Replace range exceeds bounds");
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
    const auto range = findFirstTextRange(storage.dataView(), text, compareFn);
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

auto U32StringModifyTools::removed(const CpRange range) const -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U32StringReadTools{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }

    const auto start = removeRange.index().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U32StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char32_t));
    }
    const auto tailStart = U32StringSharedStorage::checkedAddSize(start, count, "Remove range exceeds string bounds");
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start, data.data() + tailStart, (data.size() - tailStart) * sizeof(char32_t));
    }
    return storage;
}

auto U32StringModifyTools::removedAll(const CharSet &characters) const -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U32StringModifyTools::removed(const U32StringDataView &text, const CharCompareFn compareFn) const
    -> U32StringSharedStorage {
    return replacedText(text, {}, compareFn);
}

auto U32StringModifyTools::kept(const CpRange range) const -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U32StringReadTools{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return {};
    }
    return U32StringSharedStorage::fromCodeUnits(
        data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()));
}

auto U32StringModifyTools::replacedAll(const CharSet &characters, const Char replacement) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    const auto bytes = characterBytes(replacement);
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U32StringModifyTools::replacedAll(const CharSet &characters, const U32StringDataView &replacement) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U32StringModifyTools::replacedAll(
    const U32StringDataView &text, const U32StringDataView &replacement, const CharCompareFn compareFn) const
    -> U32StringSharedStorage {
    return replacedText(text, replacement.dataSpan(), compareFn);
}

auto U32StringModifyTools::inserted(const CpIndex index, const U32StringDataView &text) const
    -> U32StringSharedStorage {
    if (index.isNoIndex()) {
        return U32StringSharedStorage::fromCodeUnits(_data.dataSpan());
    }
    const auto insertIndex = CpIndex::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replaced(CpRange::emptyAt(insertIndex), text);
}

auto U32StringModifyTools::replaced(const CpRange range, const U32StringDataView &text) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }
    const auto replaceRange = range.clampedTo(CpLength::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }

    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto newSize = U32StringSharedStorage::checkedAddSize(
        data.size() - removeLength, replacement.size(), "Modified string exceeds size bounds");
    auto storage = U32StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char32_t));
    }
    if (!replacement.empty()) {
        std::memcpy(storage.dataForWrite() + start, replacement.data(), replacement.size() * sizeof(char32_t));
    }
    const auto tailStart = U32StringSharedStorage::checkedAddSize(start, removeLength, "Replace range exceeds bounds");
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start + replacement.size(),
            data.data() + tailStart,
            (data.size() - tailStart) * sizeof(char32_t));
    }
    return storage;
}

auto U32StringModifyTools::removedFirst(const U32StringDataView &text, const CharCompareFn compareFn) const
    -> U32StringSharedStorage {
    return replacedFirst(text, {}, compareFn);
}

auto U32StringModifyTools::replacedFirst(
    const U32StringDataView &text, const U32StringDataView &replacement, const CharCompareFn compareFn) const
    -> U32StringSharedStorage {
    const auto range = findFirstTextRange(_data, text, compareFn);
    if (!range.isValid()) {
        return U32StringSharedStorage::fromCodeUnits(_data.dataSpan());
    }
    return replaced(range, replacement);
}

auto U32StringModifyTools::replacedText(
    const U32StringDataView &text, const std::span<const char32_t> replacement, const CharCompareFn compareFn) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return U32StringSharedStorage::fromCodeUnits(data);
    }

    auto newSize = std::size_t{0};
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            newSize = U32StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            utf32::fastAdvanceChar(data, position);
            newSize = U32StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U32StringSharedStorage::validateSize(newSize);
    auto storage = U32StringSharedStorage::forSize(newSize);
    auto writePosition = std::size_t{0};
    position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            if (!replacement.empty()) {
                std::memcpy(
                    storage.dataForWrite() + writePosition, replacement.data(), replacement.size() * sizeof(char32_t));
                writePosition = U32StringSharedStorage::checkedAddSize(
                    writePosition, replacement.size(), "Modified string write exceeds size bounds");
            }
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            utf32::fastAdvanceChar(data, position);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            std::memcpy(
                storage.dataForWrite() + writePosition,
                data.data() + characterStart.toSizeT(),
                characterSize * sizeof(char32_t));
            writePosition = U32StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    return storage;
}

auto U32StringModifyTools::matchesText(
    const std::span<const char32_t> data,
    const CpIndex start,
    const std::span<const char32_t> text,
    const CharCompareFn compareFn) noexcept -> bool {
    auto dataPosition = start;
    auto textPosition = CpIndex::zero();
    while (textPosition.toSizeT() < text.size()) {
        if (dataPosition.toSizeT() >= data.size()) {
            return false;
        }
        const auto dataCharacter = utf32::decodeCharOrReplace(data, dataPosition);
        const auto textCharacter = utf32::decodeCharOrReplace(text, textPosition);
        if (!charactersEqual(dataCharacter, textCharacter, compareFn)) {
            return false;
        }
    }
    return true;
}

auto U32StringModifyTools::charactersEqual(const Char left, const Char right, const CharCompareFn compareFn) noexcept
    -> bool {
    if (compareFn != nullptr) {
        return compareFn(left, right) == std::strong_ordering::equal;
    }
    return left == right;
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
        storage = U32StringModifyTools{dataView}.replacedText(text, replacement, compareFn);
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            hasMatch = true;
            position = endOfMatch(data, position, needle);
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
            writePosition = U32StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    if (hasMatch) {
        storage.resize(writePosition);
    }
    return storage;
}

auto U32StringModifyTools::findFirstTextRange(
    const U32StringDataView &dataView, const U32StringDataView &text, const CharCompareFn compareFn) noexcept
    -> CpRange {
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return CpRange::noRange();
    }

    auto position = CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto start = position;
        if (matchesText(data, start, needle, compareFn)) {
            return CpRange{start, endOfMatch(data, start, needle)};
        }
        utf32::fastAdvanceChar(data, position);
    }
    return CpRange::noRange();
}

auto U32StringModifyTools::endOfMatch(
    const std::span<const char32_t> data, CpIndex start, const std::span<const char32_t> text) noexcept -> CpIndex {
    auto textPosition = CpIndex::zero();
    while (textPosition.toSizeT() < text.size() && start.toSizeT() < data.size()) {
        utf32::fastAdvanceChar(data, start);
        utf32::fastAdvanceChar(text, textPosition);
    }
    return start;
}

}
