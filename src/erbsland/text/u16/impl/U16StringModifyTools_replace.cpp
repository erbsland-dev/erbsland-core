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

auto U16StringModifyTools::removed(const unit::CpRange range) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto removeRange = _data.relativeRangeForAbsolute(U16StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || removeRange.isEmpty()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }

    const auto start = removeRange.index().toSizeT();
    const auto count = removeRange.length().toSizeT();
    const auto newSize = data.size() - count;
    auto storage = U16StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char16_t));
    }
    const auto tailStart = U16StringSharedStorage::checkedAddSize(start, count, "Remove range exceeds string bounds");
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start, data.data() + tailStart, (data.size() - tailStart) * sizeof(char16_t));
    }
    return storage;
}

auto U16StringModifyTools::removedAll(const CharSet &characters) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, {});
}

auto U16StringModifyTools::removed(const U16StringDataView &text, const CharCompareFn compareFn) const
    -> U16StringSharedStorage {
    return replacedText(text, {}, compareFn);
}

auto U16StringModifyTools::kept(const unit::CpRange range) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto keepRange = _data.relativeRangeForAbsolute(U16StringCharReadTool{_data}.sliceRange(range));
    if (data.empty() || keepRange.isEmpty()) {
        return {};
    }
    return U16StringSharedStorage::fromCodeUnits(
        data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()));
}

auto U16StringModifyTools::replacedAll(const CharSet &characters, const Char replacement) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }
    const auto bytes = characterBytes(replacement);
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); },
        characterByteSpan(replacement, bytes));
}

auto U16StringModifyTools::replacedAll(const CharSet &characters, const U16StringDataView &replacement) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || characters.isEmpty()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }
    return replacedCharacters(
        [&](const Char character) noexcept -> bool { return characters.contains(character); }, replacement.dataSpan());
}

auto U16StringModifyTools::replacedAll(
    const U16StringDataView &text, const U16StringDataView &replacement, const CharCompareFn compareFn) const
    -> U16StringSharedStorage {
    return replacedText(text, replacement.dataSpan(), compareFn);
}

auto U16StringModifyTools::removed(const unit::U16DataRange range) const -> U16StringSharedStorage {
    return replaced(range, {});
}

auto U16StringModifyTools::kept(const unit::U16DataRange range) const -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty() || !range.isValid()) {
        return {};
    }
    const auto keepRange = range.clampedTo(unit::U16DataLength::fromSizeT(data.size()));
    if (keepRange.isEmpty()) {
        return {};
    }
    return U16StringSharedStorage::fromCodeUnits(
        data.subspan(keepRange.index().toSizeT(), keepRange.length().toSizeT()));
}

auto U16StringModifyTools::inserted(const unit::U16DataIndex index, const U16StringDataView &text) const
    -> U16StringSharedStorage {
    if (index.isNoIndex()) {
        return U16StringSharedStorage::fromCodeUnits(_data.dataSpan());
    }
    const auto insertIndex = unit::U16DataIndex::fromSizeT(std::min(index.toSizeT(), _data.dataSpan().size()));
    return replaced(unit::U16DataRange::emptyAt(insertIndex), text);
}

auto U16StringModifyTools::inserted(const unit::CpIndex index, const U16StringDataView &text) const
    -> U16StringSharedStorage {
    if (index.isNoIndex()) {
        return U16StringSharedStorage::fromCodeUnits(_data.dataSpan());
    }
    auto dataIndex = dataIndexForCharacterIndex(_data, index);
    if (dataIndex.isNoIndex()) {
        dataIndex = unit::U16DataIndex::end(unit::U16DataLength::fromSizeT(_data.dataSpan().size()));
    }
    return inserted(dataIndex, text);
}

auto U16StringModifyTools::replaced(const unit::U16DataRange range, const U16StringDataView &text) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (!range.isValid()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }
    const auto replaceRange = range.clampedTo(unit::U16DataLength::fromSizeT(data.size()));
    const auto replacement = text.dataSpan();
    if (replaceRange.isEmpty() && replacement.empty()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }

    const auto start = replaceRange.index().toSizeT();
    const auto removeLength = replaceRange.length().toSizeT();
    const auto newSize = U16StringSharedStorage::checkedAddSize(
        data.size() - removeLength, replacement.size(), "Modified string exceeds size bounds");
    auto storage = U16StringSharedStorage::forSize(newSize);
    if (start > 0U) {
        std::memcpy(storage.dataForWrite(), data.data(), start * sizeof(char16_t));
    }
    if (!replacement.empty()) {
        std::memcpy(storage.dataForWrite() + start, replacement.data(), replacement.size() * sizeof(char16_t));
    }
    const auto tailStart = U16StringSharedStorage::checkedAddSize(start, removeLength, "Replace range exceeds bounds");
    if (tailStart < data.size()) {
        std::memcpy(
            storage.dataForWrite() + start + replacement.size(),
            data.data() + tailStart,
            (data.size() - tailStart) * sizeof(char16_t));
    }
    return storage;
}

auto U16StringModifyTools::replaced(const unit::CpRange range, const U16StringDataView &text) const
    -> U16StringSharedStorage {
    return replaced(dataRangeForCharacterRange(_data, range), text);
}

auto U16StringModifyTools::removedFirst(const U16StringDataView &text, const CharCompareFn compareFn) const
    -> U16StringSharedStorage {
    return replacedFirst(text, {}, compareFn);
}

auto U16StringModifyTools::replacedFirst(
    const U16StringDataView &text, const U16StringDataView &replacement, const CharCompareFn compareFn) const
    -> U16StringSharedStorage {
    const auto range = findFirstTextRange(_data, text, compareFn);
    if (!range.isValid()) {
        return U16StringSharedStorage::fromCodeUnits(_data.dataSpan());
    }
    return replaced(range, replacement);
}

auto U16StringModifyTools::replacedText(
    const U16StringDataView &text, const std::span<const char16_t> replacement, const CharCompareFn compareFn) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return U16StringSharedStorage::fromCodeUnits(data);
    }

    auto newSize = std::size_t{0};
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            newSize = U16StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            const auto character = utf16::decodeCharOrReplace(data, position);
            static_cast<void>(character);
            newSize = U16StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U16StringSharedStorage::validateSize(newSize);
    auto storage = U16StringSharedStorage::forSize(newSize);
    auto writePosition = std::size_t{0};
    position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            if (!replacement.empty()) {
                std::memcpy(
                    storage.dataForWrite() + writePosition, replacement.data(), replacement.size() * sizeof(char16_t));
                writePosition = U16StringSharedStorage::checkedAddSize(
                    writePosition, replacement.size(), "Modified string write exceeds size bounds");
            }
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            const auto character = utf16::decodeCharOrReplace(data, position);
            static_cast<void>(character);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            std::memcpy(
                storage.dataForWrite() + writePosition,
                data.data() + characterStart.toSizeT(),
                characterSize * sizeof(char16_t));
            writePosition = U16StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    return storage;
}

auto U16StringModifyTools::matchesText(
    const std::span<const char16_t> data,
    const unit::U16DataIndex start,
    const std::span<const char16_t> text,
    const CharCompareFn compareFn) noexcept -> bool {
    auto dataPosition = start;
    auto textPosition = unit::U16DataIndex::zero();
    while (textPosition.toSizeT() < text.size()) {
        if (dataPosition.toSizeT() >= data.size()) {
            return false;
        }
        const auto dataCharacter = utf16::decodeCharOrReplace(data, dataPosition);
        const auto textCharacter = utf16::decodeCharOrReplace(text, textPosition);
        if (!charactersEqual(dataCharacter, textCharacter, compareFn)) {
            return false;
        }
    }
    return true;
}

auto U16StringModifyTools::charactersEqual(const Char left, const Char right, const CharCompareFn compareFn) noexcept
    -> bool {
    if (compareFn != nullptr) {
        return compareFn(left, right) == std::strong_ordering::equal;
    }
    return left == right;
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
        storage = U16StringModifyTools{dataView}.replacedText(text, replacement, compareFn);
        return storage;
    }

    auto hasMatch = false;
    auto writePosition = std::size_t{0};
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        if (matchesText(data, position, needle, compareFn)) {
            hasMatch = true;
            position = endOfMatch(data, position, needle);
        } else {
            const auto characterStart = position;
            const auto character = utf16::decodeCharOrReplace(data, position);
            static_cast<void>(character);
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            if (writePosition != characterStart.toSizeT()) {
                std::memmove(
                    storage.dataForWrite() + writePosition,
                    data.data() + characterStart.toSizeT(),
                    characterSize * sizeof(char16_t));
            }
            writePosition = U16StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    if (hasMatch) {
        storage.resize(writePosition);
    }
    return storage;
}

auto U16StringModifyTools::dataRangeForCharacterRange(const U16StringDataView &data, const unit::CpRange range) noexcept
    -> unit::U16DataRange {
    if (!range.isValid()) {
        return unit::U16DataRange::noRange();
    }
    auto start = dataIndexForCharacterIndex(data, range.index());
    if (start.isNoIndex()) {
        start = unit::U16DataIndex::end(unit::U16DataLength::fromSizeT(data.dataSpan().size()));
    }
    if (range.length().isZero()) {
        return unit::U16DataRange::emptyAt(start);
    }
    if (range.length().isInfinite()) {
        return unit::U16DataRange{start, unit::U16DataLength::infinite()}.clampedTo(
            unit::U16DataLength::fromSizeT(data.dataSpan().size()));
    }
    auto end = range.endIndex();
    auto endData = end.isNoIndex() ? unit::U16DataIndex::noIndex() : dataIndexForCharacterIndex(data, end);
    if (endData.isNoIndex()) {
        endData = unit::U16DataIndex::end(unit::U16DataLength::fromSizeT(data.dataSpan().size()));
    }
    return unit::U16DataRange{start, endData}.clampedTo(unit::U16DataLength::fromSizeT(data.dataSpan().size()));
}

auto U16StringModifyTools::dataIndexForCharacterIndex(const U16StringDataView &data, const unit::CpIndex index) noexcept
    -> unit::U16DataIndex {
    return U16StringCharReadTool{data}.byteIndexAt(index);
}

auto U16StringModifyTools::findFirstTextRange(
    const U16StringDataView &dataView, const U16StringDataView &text, const CharCompareFn compareFn) noexcept
    -> unit::U16DataRange {
    const auto data = dataView.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return unit::U16DataRange::noRange();
    }

    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto start = position;
        if (matchesText(data, start, needle, compareFn)) {
            return unit::U16DataRange{start, endOfMatch(data, start, needle)};
        }
        const auto character = utf16::decodeCharOrReplace(data, position);
        static_cast<void>(character);
    }
    return unit::U16DataRange::noRange();
}

auto U16StringModifyTools::endOfMatch(
    const std::span<const char16_t> data, unit::U16DataIndex start, const std::span<const char16_t> text) noexcept
    -> unit::U16DataIndex {
    auto textPosition = unit::U16DataIndex::zero();
    while (textPosition.toSizeT() < text.size() && start.toSizeT() < data.size()) {
        utf16::fastAdvanceChar(data, start);
        utf16::fastAdvanceChar(text, textPosition);
    }
    return start;
}

}
