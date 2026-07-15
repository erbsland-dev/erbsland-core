// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16Encoding.hpp"
#include "U16StringDataView.hpp"
#include "U16StringSharedStorage.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../../unit/U16DataIndex.hpp"
#include "../../../unit/U16DataRange.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"

#include <array>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

/// Copy-modify algorithms for UTF-16 strings.
/// @tested{U16StringTest}
class U16StringModifyTools final {
public:
    explicit constexpr U16StringModifyTools(const U16StringDataView &data) noexcept : _data{data} {}

public:
    /// Remove a UTF-16 data range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U16StringSharedStorage &storage, unit::U16DataRange range) -> U16StringSharedStorage &;
    /// Remove a character range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U16StringSharedStorage &storage, unit::CpRange range) -> U16StringSharedStorage &;
    /// Remove all characters from a set from mutable storage, reusing unique full-range storage when possible.
    static auto removeAll(U16StringSharedStorage &storage, const CharSet &characters) -> U16StringSharedStorage &;
    /// Remove all occurrences of decoded UTF-16 text from mutable storage.
    static auto remove(U16StringSharedStorage &storage, const U16StringDataView &text, CharCompareFn compareFn = {})
        -> U16StringSharedStorage &;
    /// Keep a UTF-16 data range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U16StringSharedStorage &storage, unit::U16DataRange range) -> U16StringSharedStorage &;
    /// Keep a character range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U16StringSharedStorage &storage, unit::CpRange range) -> U16StringSharedStorage &;
    /// Insert text at a UTF-16 data index in mutable storage.
    static auto insert(U16StringSharedStorage &storage, unit::U16DataIndex index, const U16StringDataView &text)
        -> U16StringSharedStorage &;
    /// Insert text at a character index in mutable storage.
    static auto insert(U16StringSharedStorage &storage, unit::CpIndex index, const U16StringDataView &text)
        -> U16StringSharedStorage &;
    /// Replace a UTF-16 data range in mutable storage.
    static auto replace(U16StringSharedStorage &storage, unit::U16DataRange range, const U16StringDataView &text)
        -> U16StringSharedStorage &;
    /// Replace a character range in mutable storage.
    static auto replace(U16StringSharedStorage &storage, unit::CpRange range, const U16StringDataView &text)
        -> U16StringSharedStorage &;
    /// Remove the first occurrence of decoded UTF-16 text from mutable storage.
    static auto removeFirst(
        U16StringSharedStorage &storage, const U16StringDataView &text, CharCompareFn compareFn = {})
        -> U16StringSharedStorage &;
    /// Replace the first occurrence of decoded UTF-16 text in mutable storage.
    static auto replaceFirst(
        U16StringSharedStorage &storage,
        const U16StringDataView &text,
        const U16StringDataView &replacement,
        CharCompareFn compareFn = {}) -> U16StringSharedStorage &;
    /// Replace all characters from a set in mutable storage.
    static auto replaceAll(U16StringSharedStorage &storage, const CharSet &characters, Char replacement)
        -> U16StringSharedStorage &;
    /// Replace all characters from a set with text in mutable storage.
    static auto replaceAll(
        U16StringSharedStorage &storage, const CharSet &characters, const U16StringDataView &replacement)
        -> U16StringSharedStorage &;
    /// Replace all occurrences of decoded UTF-16 text in mutable storage.
    static auto replaceAll(
        U16StringSharedStorage &storage,
        const U16StringDataView &text,
        const U16StringDataView &replacement,
        CharCompareFn compareFn = {}) -> U16StringSharedStorage &;

public:
    /// Return storage with a UTF-16 data range removed.
    [[nodiscard]] auto removed(unit::U16DataRange range) const -> U16StringSharedStorage;
    /// Return storage with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U16StringSharedStorage;
    /// Return storage with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U16StringSharedStorage;
    /// Return storage with all occurrences of decoded UTF-16 text removed.
    [[nodiscard]] auto removed(const U16StringDataView &text, CharCompareFn compareFn = {}) const
        -> U16StringSharedStorage;
    /// Return storage keeping only a UTF-16 data range.
    [[nodiscard]] auto kept(unit::U16DataRange range) const -> U16StringSharedStorage;
    /// Return storage keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U16StringSharedStorage;
    /// Return storage with text inserted at a UTF-16 data index.
    [[nodiscard]] auto inserted(unit::U16DataIndex index, const U16StringDataView &text) const
        -> U16StringSharedStorage;
    /// Return storage with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U16StringDataView &text) const -> U16StringSharedStorage;
    /// Return storage with a UTF-16 data range replaced by text.
    [[nodiscard]] auto replaced(unit::U16DataRange range, const U16StringDataView &text) const
        -> U16StringSharedStorage;
    /// Return storage with a character range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U16StringDataView &text) const -> U16StringSharedStorage;
    /// Return storage with the first occurrence of decoded UTF-16 text removed.
    [[nodiscard]] auto removedFirst(const U16StringDataView &text, CharCompareFn compareFn = {}) const
        -> U16StringSharedStorage;
    /// Return storage with the first occurrence of decoded UTF-16 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U16StringDataView &text, const U16StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> U16StringSharedStorage;
    /// Return storage with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U16StringSharedStorage;
    /// Return storage with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U16StringDataView &replacement) const
        -> U16StringSharedStorage;
    /// Return storage with all occurrences of decoded UTF-16 text replaced.
    [[nodiscard]] auto replacedAll(
        const U16StringDataView &text, const U16StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> U16StringSharedStorage;

private:
    template <typename T>
    [[nodiscard]] static auto spansOverlap(std::span<const T> first, std::span<const T> second) noexcept -> bool;
    [[nodiscard]] static auto characterBytes(Char character) noexcept -> std::array<char16_t, 2>;
    [[nodiscard]] static auto characterByteSpan(Char character, const std::array<char16_t, 2> &bytes) noexcept
        -> std::span<const char16_t>;

    template <typename Predicate>
    [[nodiscard]] auto replacedCharacters(Predicate predicate, std::span<const char16_t> replacement) const
        -> U16StringSharedStorage;
    [[nodiscard]] auto replacedText(
        const U16StringDataView &text, std::span<const char16_t> replacement, CharCompareFn compareFn) const
        -> U16StringSharedStorage;
    template <typename Predicate>
    static auto replaceCharactersInStorage(
        U16StringSharedStorage &storage, Predicate predicate, std::span<const char16_t> replacement)
        -> U16StringSharedStorage &;
    static auto replaceTextInStorage(
        U16StringSharedStorage &storage,
        const U16StringDataView &text,
        std::span<const char16_t> replacement,
        CharCompareFn compareFn) -> U16StringSharedStorage &;
    [[nodiscard]] static auto dataRangeForCharacterRange(const U16StringDataView &data, unit::CpRange range) noexcept
        -> unit::U16DataRange;
    [[nodiscard]] static auto dataIndexForCharacterIndex(const U16StringDataView &data, unit::CpIndex index) noexcept
        -> unit::U16DataIndex;
    [[nodiscard]] static auto findFirstTextRange(
        const U16StringDataView &data, const U16StringDataView &text, CharCompareFn compareFn) noexcept
        -> unit::U16DataRange;
    [[nodiscard]] static auto matchesText(
        std::span<const char16_t> data,
        unit::U16DataIndex start,
        std::span<const char16_t> text,
        CharCompareFn compareFn) noexcept -> bool;
    [[nodiscard]] static auto charactersEqual(Char left, Char right, CharCompareFn compareFn) noexcept -> bool;
    [[nodiscard]] static auto endOfMatch(
        std::span<const char16_t> data, unit::U16DataIndex start, std::span<const char16_t> text) noexcept
        -> unit::U16DataIndex;

private:
    U16StringDataView _data;
};

template <typename Predicate>
auto U16StringModifyTools::replacedCharacters(Predicate predicate, const std::span<const char16_t> replacement) const
    -> U16StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty()) {
        return {};
    }

    auto newSize = std::size_t{0};
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (predicate(character)) {
            newSize = U16StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
        } else {
            newSize = U16StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U16StringSharedStorage::validateSize(newSize);
    auto storage = U16StringSharedStorage::forSize(newSize);
    auto writePosition = std::size_t{0};
    position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (predicate(character)) {
            if (!replacement.empty()) {
                std::memcpy(
                    storage.dataForWrite() + writePosition, replacement.data(), replacement.size() * sizeof(char16_t));
                writePosition = U16StringSharedStorage::checkedAddSize(
                    writePosition, replacement.size(), "Modified string write exceeds size bounds");
            }
        } else {
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

template <typename Predicate>
auto U16StringModifyTools::replaceCharactersInStorage(
    U16StringSharedStorage &storage, Predicate predicate, const std::span<const char16_t> replacement)
    -> U16StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }

    auto newSize = std::size_t{0};
    auto hasMatch = false;
    auto canWriteInPlace = storage.isUniqueFullRange() && replacement.empty();
    auto position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        const auto characterSize = position.toSizeT() - characterStart.toSizeT();
        if (predicate(character)) {
            hasMatch = true;
            newSize = U16StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
        } else {
            newSize =
                U16StringSharedStorage::checkedAddSize(newSize, characterSize, "Modified string exceeds size bounds");
        }
    }
    if (!hasMatch) {
        return storage;
    }
    if (!canWriteInPlace) {
        storage = U16StringModifyTools{dataView}.replacedCharacters(predicate, replacement);
        return storage;
    }

    auto writePosition = std::size_t{0};
    position = unit::U16DataIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (!predicate(character)) {
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
    storage.resize(newSize);
    return storage;
}

}
