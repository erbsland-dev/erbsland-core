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

namespace {

[[nodiscard]] auto characterBytes(const Char character) noexcept -> std::array<char, 4> {
    auto result = std::array<char, 4>{};
    U8Writer writer{std::span<char>{result}};
    writer.write(character);
    return result;
}

[[nodiscard]] auto characterByteSpan(const Char character, const std::array<char, 4> &bytes) noexcept
    -> std::span<const char> {
    if (!character.isValidUnicode()) {
        return {};
    }
    return std::span<const char>{bytes.data(), utf8::encodedLength(character).toSizeT()};
}

template <typename T>
[[nodiscard]] auto spansOverlap(const std::span<const T> first, const std::span<const T> second) noexcept -> bool {
    if (first.empty() || second.empty()) {
        return false;
    }
    const auto firstBegin = reinterpret_cast<std::uintptr_t>(first.data());
    const auto firstEnd = firstBegin + first.size() * sizeof(T);
    const auto secondBegin = reinterpret_cast<std::uintptr_t>(second.data());
    const auto secondEnd = secondBegin + second.size() * sizeof(T);
    return firstBegin < secondEnd && secondBegin < firstEnd;
}

}

auto U8StringModifyTools::remove(U8StringSharedStorage &storage, const unit::ByteRange range)
    -> U8StringSharedStorage & {
    return replace(storage, range, {});
}

auto U8StringModifyTools::remove(U8StringSharedStorage &storage, const unit::CpRange range) -> U8StringSharedStorage & {
    return remove(storage, byteRangeForCharacterRange(storage.dataView(), range));
}

auto U8StringModifyTools::keep(U8StringSharedStorage &storage, const unit::ByteRange range) -> U8StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }
    if (!range.isValid()) {
        storage.clear();
        return storage;
    }
    const auto keepRange = range.clampedTo(unit::ByteLength::fromSizeT(data.size()));
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

auto U8StringModifyTools::keep(U8StringSharedStorage &storage, const unit::CpRange range) -> U8StringSharedStorage & {
    return keep(storage, byteRangeForCharacterRange(storage.dataView(), range));
}

auto U8StringModifyTools::insert(
    U8StringSharedStorage &storage, const unit::ByteIndex index, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    const auto insertIndex =
        unit::ByteIndex::fromSizeT(std::min(index.toSizeT(), storage.dataView().dataSpan().size()));
    return replace(storage, unit::ByteRange::emptyAt(insertIndex), text);
}

auto U8StringModifyTools::insert(
    U8StringSharedStorage &storage, const unit::CpIndex index, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    if (index.isNoIndex()) {
        return storage;
    }
    auto byteIndex = byteIndexForCharacterIndex(storage.dataView(), index);
    if (byteIndex.isNoIndex()) {
        byteIndex = unit::ByteIndex::end(unit::ByteLength::fromSizeT(storage.dataView().dataSpan().size()));
    }
    return insert(storage, byteIndex, text);
}

auto U8StringModifyTools::replace(
    U8StringSharedStorage &storage, const unit::ByteRange range, const U8StringDataView &text)
    -> U8StringSharedStorage & {
    if (!range.isValid()) {
        return storage;
    }

    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    const auto replacement = text.dataSpan();
    const auto replaceRange = range.clampedTo(unit::ByteLength::fromSizeT(data.size()));
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

auto U8StringModifyTools::replace(
    U8StringSharedStorage &storage, const unit::CpRange range, const U8StringDataView &text)
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

auto U8StringModifyTools::removed(const unit::CpRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U8StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return U8StringSharedStorage::fromBytes(data);
    }

    const auto start = removeRange.index().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U8StringSharedStorage::forSize(newSize);
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
        return U8StringSharedStorage::fromBytes(data);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U8StringModifyTools::removed(const U8StringDataView &text, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    return replacedText(text, {}, compareFn);
}

auto U8StringModifyTools::kept(const unit::CpRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U8StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return {};
    }
    return U8StringSharedStorage::fromBytes(data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()));
}

auto U8StringModifyTools::replacedAll(const CharSet &characters, const Char replacement) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U8StringSharedStorage::fromBytes(data);
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
        return U8StringSharedStorage::fromBytes(data);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U8StringModifyTools::replacedAll(
    const U8StringDataView &text, const U8StringDataView &replacement, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    return replacedText(text, replacement.dataSpan(), compareFn);
}

auto U8StringModifyTools::removed(const unit::ByteRange range) const -> U8StringSharedStorage {
    return replaced(range, {});
}

auto U8StringModifyTools::kept(const unit::ByteRange range) const -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || !range.isValid()) {
        return {};
    }
    const auto keepRange = range.clampedTo(unit::ByteLength::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        return {};
    }
    return U8StringSharedStorage::fromBytes(data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()));
}

auto U8StringModifyTools::inserted(const unit::ByteIndex index, const U8StringDataView &text) const
    -> U8StringSharedStorage {
    if (index.isNoIndex()) {
        return U8StringSharedStorage::fromBytes(_data.dataSpan());
    }
    const auto insertIndex = unit::ByteIndex::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replaced(unit::ByteRange::emptyAt(insertIndex), text);
}

auto U8StringModifyTools::inserted(const unit::CpIndex index, const U8StringDataView &text) const
    -> U8StringSharedStorage {
    if (index.isNoIndex()) {
        return U8StringSharedStorage::fromBytes(_data.dataSpan());
    }
    auto byteIndex = byteIndexForCharacterIndex(_data, index);
    if (byteIndex.isNoIndex()) {
        byteIndex = unit::ByteIndex::end(unit::ByteLength::fromSizeT(_data.dataSpan().size()));
    }
    return inserted(byteIndex, text);
}

auto U8StringModifyTools::replaced(const unit::ByteRange range, const U8StringDataView &text) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return U8StringSharedStorage::fromBytes(data);
    }
    const auto replaceRange = range.clampedTo(unit::ByteLength::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return U8StringSharedStorage::fromBytes(data);
    }

    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto newSize = U8StringSharedStorage::checkedAddSize(
        data.size() - removeLength, replacement.size(), "Modified string exceeds size bounds");
    auto storage = U8StringSharedStorage::forSize(newSize);
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

auto U8StringModifyTools::replaced(const unit::CpRange range, const U8StringDataView &text) const
    -> U8StringSharedStorage {
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
        return U8StringSharedStorage::fromBytes(_data.dataSpan());
    }
    return replaced(range, replacement);
}

auto U8StringModifyTools::replacedText(
    const U8StringDataView &text, const std::span<const char> replacement, const CharCompareFn compareFn) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return U8StringSharedStorage::fromBytes(data);
    }

    auto newSize = std::size_t{0};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            const auto character = utf8::decodeCharOrReplace(data, position);
            static_cast<void>(character);
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U8StringSharedStorage::validateSize(newSize);
    auto storage = U8StringSharedStorage::forSize(newSize);
    auto writePosition = std::size_t{0};
    position = unit::ByteIndex::zero();
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
            const auto character = utf8::decodeCharOrReplace(data, position);
            static_cast<void>(character);
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
    const unit::ByteIndex start,
    const std::span<const char> text,
    const CharCompareFn compareFn) noexcept -> bool {
    auto dataPosition = start;
    auto textPosition = unit::ByteIndex::zero();
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
        storage = U8StringModifyTools{dataView}.replacedText(text, replacement, compareFn);
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            hasMatch = true;
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            const auto character = utf8::decodeCharOrReplace(data, position);
            static_cast<void>(character);
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

auto U8StringModifyTools::byteRangeForCharacterRange(const U8StringDataView &data, const unit::CpRange range) noexcept
    -> unit::ByteRange {
    if (!range.isValid()) {
        return unit::ByteRange::noRange();
    }
    auto start = byteIndexForCharacterIndex(data, range.index());
    if (start.isNoIndex()) {
        start = unit::ByteIndex::end(unit::ByteLength::fromSizeT(data.dataSpan().size()));
    }
    if (range.length().isZero()) {
        return unit::ByteRange::emptyAt(start);
    }
    if (range.length().isInfinite()) {
        return unit::ByteRange{start, unit::ByteLength::infinite()}.clampedTo(
            unit::ByteLength::fromSizeT(data.dataSpan().size()));
    }
    auto end = range.endIndex();
    auto endByte = end.isNoIndex() ? unit::ByteIndex::noIndex() : byteIndexForCharacterIndex(data, end);
    if (endByte.isNoIndex()) {
        endByte = unit::ByteIndex::end(unit::ByteLength::fromSizeT(data.dataSpan().size()));
    }
    return unit::ByteRange{start, endByte}.clampedTo(unit::ByteLength::fromSizeT(data.dataSpan().size()));
}

auto U8StringModifyTools::byteIndexForCharacterIndex(const U8StringDataView &data, const unit::CpIndex index) noexcept
    -> unit::ByteIndex {
    return U8StringCharReadTool{data}.byteIndexAt(index);
}

auto U8StringModifyTools::findFirstTextRange(
    const U8StringDataView &dataView, const U8StringDataView &text, const CharCompareFn compareFn) noexcept
    -> unit::ByteRange {
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return unit::ByteRange::noRange();
    }

    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto start = position;
        if (matchesText(data, start, needle, compareFn)) {
            return unit::ByteRange{start, endOfMatch(data, start, needle)};
        }
        const auto character = utf8::decodeCharOrReplace(data, position);
        static_cast<void>(character);
    }
    return unit::ByteRange::noRange();
}

auto U8StringModifyTools::endOfMatch(
    const std::span<const char> data, unit::ByteIndex start, const std::span<const char> text) noexcept
    -> unit::ByteIndex {
    auto textPosition = unit::ByteIndex::zero();
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
