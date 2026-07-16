// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32Encoding.hpp"
#include "U32StringDataView.hpp"
#include "U32StringSharedStorage.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"

#include <cstring>
#include <span>

namespace erbsland::text::impl {

/// Copy-modify algorithms for UTF-32 strings.
/// @tested{U32StringTest}
class U32StringModifyTools final {
public:
    explicit constexpr U32StringModifyTools(const U32StringDataView &data) noexcept : _data{data} {}

public:
    /// Remove a character range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U32StringSharedStorage &storage, unit::CpRange range) -> U32StringSharedStorage &;
    /// Remove all characters from a set from mutable storage, reusing unique full-range storage when possible.
    static auto removeAll(U32StringSharedStorage &storage, const CharSet &characters) -> U32StringSharedStorage &;
    /// Remove all occurrences of decoded UTF-32 text from mutable storage.
    static auto remove(U32StringSharedStorage &storage, const U32StringDataView &text, CharCompareFn compareFn = {})
        -> U32StringSharedStorage &;
    /// Keep a character range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U32StringSharedStorage &storage, unit::CpRange range) -> U32StringSharedStorage &;
    /// Insert text at a character index in mutable storage.
    static auto insert(U32StringSharedStorage &storage, unit::CpIndex index, const U32StringDataView &text)
        -> U32StringSharedStorage &;
    /// Replace a character range in mutable storage.
    static auto replace(U32StringSharedStorage &storage, unit::CpRange range, const U32StringDataView &text)
        -> U32StringSharedStorage &;
    /// Remove the first occurrence of decoded UTF-32 text from mutable storage.
    static auto removeFirst(
        U32StringSharedStorage &storage, const U32StringDataView &text, CharCompareFn compareFn = {})
        -> U32StringSharedStorage &;
    /// Replace the first occurrence of decoded UTF-32 text in mutable storage.
    static auto replaceFirst(
        U32StringSharedStorage &storage,
        const U32StringDataView &text,
        const U32StringDataView &replacement,
        CharCompareFn compareFn = {}) -> U32StringSharedStorage &;
    /// Replace all characters from a set in mutable storage.
    static auto replaceAll(U32StringSharedStorage &storage, const CharSet &characters, Char replacement)
        -> U32StringSharedStorage &;
    /// Replace all characters from a set with text in mutable storage.
    static auto replaceAll(
        U32StringSharedStorage &storage, const CharSet &characters, const U32StringDataView &replacement)
        -> U32StringSharedStorage &;
    /// Replace all occurrences of decoded UTF-32 text in mutable storage.
    static auto replaceAll(
        U32StringSharedStorage &storage,
        const U32StringDataView &text,
        const U32StringDataView &replacement,
        CharCompareFn compareFn = {}) -> U32StringSharedStorage &;

public:
    /// Return storage with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U32StringSharedStorage;
    /// Return storage with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U32StringSharedStorage;
    /// Return storage with all occurrences of decoded UTF-32 text removed.
    [[nodiscard]] auto removed(const U32StringDataView &text, CharCompareFn compareFn = {}) const
        -> U32StringSharedStorage;
    /// Return storage keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U32StringSharedStorage;
    /// Return storage with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U32StringDataView &text) const -> U32StringSharedStorage;
    /// Return storage with a character range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U32StringDataView &text) const -> U32StringSharedStorage;
    /// Return storage with the first occurrence of decoded UTF-32 text removed.
    [[nodiscard]] auto removedFirst(const U32StringDataView &text, CharCompareFn compareFn = {}) const
        -> U32StringSharedStorage;
    /// Return storage with the first occurrence of decoded UTF-32 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U32StringDataView &text, const U32StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> U32StringSharedStorage;
    /// Return storage with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U32StringSharedStorage;
    /// Return storage with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U32StringDataView &replacement) const
        -> U32StringSharedStorage;
    /// Return storage with all occurrences of decoded UTF-32 text replaced.
    [[nodiscard]] auto replacedAll(
        const U32StringDataView &text, const U32StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> U32StringSharedStorage;

private:
    template <typename T>
    [[nodiscard]] static auto spansOverlap(std::span<const T> first, std::span<const T> second) noexcept -> bool;
    [[nodiscard]] static auto characterBytes(Char character) noexcept -> std::array<char32_t, 2>;
    [[nodiscard]] static auto characterByteSpan(Char character, const std::array<char32_t, 2> &bytes) noexcept
        -> std::span<const char32_t>;
    template <typename Predicate>
    [[nodiscard]] auto replacedCharacters(Predicate predicate, std::span<const char32_t> replacement) const
        -> U32StringSharedStorage;
    [[nodiscard]] auto replacedText(
        const U32StringDataView &text, std::span<const char32_t> replacement, CharCompareFn compareFn) const
        -> U32StringSharedStorage;
    template <typename Predicate>
    static auto replaceCharactersInStorage(
        U32StringSharedStorage &storage, Predicate predicate, std::span<const char32_t> replacement)
        -> U32StringSharedStorage &;
    static auto replaceTextInStorage(
        U32StringSharedStorage &storage,
        const U32StringDataView &text,
        std::span<const char32_t> replacement,
        CharCompareFn compareFn) -> U32StringSharedStorage &;
    [[nodiscard]] static auto findFirstTextRange(
        const U32StringDataView &data, const U32StringDataView &text, CharCompareFn compareFn) noexcept
        -> unit::CpRange;
    [[nodiscard]] static auto matchesText(
        std::span<const char32_t> data,
        unit::CpIndex start,
        std::span<const char32_t> text,
        CharCompareFn compareFn) noexcept -> bool;
    [[nodiscard]] static auto charactersEqual(Char left, Char right, CharCompareFn compareFn) noexcept -> bool;
    [[nodiscard]] static auto endOfMatch(
        std::span<const char32_t> data, unit::CpIndex start, std::span<const char32_t> text) noexcept -> unit::CpIndex;

private:
    U32StringDataView _data;
};

template <typename Predicate>
auto U32StringModifyTools::replacedCharacters(Predicate predicate, const std::span<const char32_t> replacement) const
    -> U32StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty()) {
        return {};
    }

    auto newSize = std::size_t{0};
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (predicate(character)) {
            newSize = U32StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
        } else {
            newSize = U32StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U32StringSharedStorage::validateSize(newSize);
    auto storage = U32StringSharedStorage::forSize(newSize);
    auto writePosition = std::size_t{0};
    position = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (predicate(character)) {
            if (!replacement.empty()) {
                std::memcpy(
                    storage.dataForWrite() + writePosition, replacement.data(), replacement.size() * sizeof(char32_t));
                writePosition = U32StringSharedStorage::checkedAddSize(
                    writePosition, replacement.size(), "Modified string write exceeds size bounds");
            }
        } else {
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

template <typename Predicate>
auto U32StringModifyTools::replaceCharactersInStorage(
    U32StringSharedStorage &storage, Predicate predicate, const std::span<const char32_t> replacement)
    -> U32StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }

    auto newSize = std::size_t{0};
    auto hasMatch = false;
    auto canWriteInPlace = storage.isUniqueFullRange() && replacement.empty();
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf32::decodeCharOrReplace(data, position);
        const auto characterSize = position.toSizeT() - characterStart.toSizeT();
        if (predicate(character)) {
            hasMatch = true;
            newSize = U32StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
        } else {
            newSize =
                U32StringSharedStorage::checkedAddSize(newSize, characterSize, "Modified string exceeds size bounds");
        }
    }
    if (!hasMatch) {
        return storage;
    }
    if (!canWriteInPlace) {
        storage = U32StringModifyTools{dataView}.replacedCharacters(predicate, replacement);
        return storage;
    }

    auto writePosition = std::size_t{0};
    position = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (!predicate(character)) {
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
    storage.resize(newSize);
    return storage;
}

}
