// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"
#include "U8StringDataView.hpp"
#include "U8StringSharedStorage.hpp"

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"

#include <array>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

/// Copy-modify algorithms for UTF-8 strings.
/// @tested{U8StringModifierTest}
class U8StringModifyTools final {
public:
    explicit constexpr U8StringModifyTools(const U8StringDataView &data, const bool sensitive = false) noexcept :
        _data{data}, _sensitive{sensitive} {}

public:
    /// Remove a byte range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U8StringSharedStorage &storage, unit::ByteRange range) -> U8StringSharedStorage &;
    /// Remove a character range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U8StringSharedStorage &storage, unit::CpRange range) -> U8StringSharedStorage &;
    /// Remove all characters from a set from mutable storage, reusing unique full-range storage when possible.
    static auto removeAll(U8StringSharedStorage &storage, const CharSet &characters) -> U8StringSharedStorage &;
    /// Remove all occurrences of decoded UTF-8 text from mutable storage.
    static auto remove(U8StringSharedStorage &storage, const U8StringDataView &text, CharCompareFn compareFn = {})
        -> U8StringSharedStorage &;
    /// Keep a byte range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U8StringSharedStorage &storage, unit::ByteRange range) -> U8StringSharedStorage &;
    /// Keep a character range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U8StringSharedStorage &storage, unit::CpRange range) -> U8StringSharedStorage &;
    /// Insert text at a byte index in mutable storage.
    static auto insert(U8StringSharedStorage &storage, unit::ByteIndex index, const U8StringDataView &text)
        -> U8StringSharedStorage &;
    /// Insert text at a character index in mutable storage.
    static auto insert(U8StringSharedStorage &storage, unit::CpIndex index, const U8StringDataView &text)
        -> U8StringSharedStorage &;
    /// Replace a byte range in mutable storage.
    static auto replace(U8StringSharedStorage &storage, unit::ByteRange range, const U8StringDataView &text)
        -> U8StringSharedStorage &;
    /// Replace a character range in mutable storage.
    static auto replace(U8StringSharedStorage &storage, unit::CpRange range, const U8StringDataView &text)
        -> U8StringSharedStorage &;
    /// Remove the first occurrence of decoded UTF-8 text from mutable storage.
    static auto removeFirst(U8StringSharedStorage &storage, const U8StringDataView &text, CharCompareFn compareFn = {})
        -> U8StringSharedStorage &;
    /// Replace the first occurrence of decoded UTF-8 text in mutable storage.
    static auto replaceFirst(
        U8StringSharedStorage &storage,
        const U8StringDataView &text,
        const U8StringDataView &replacement,
        CharCompareFn compareFn = {}) -> U8StringSharedStorage &;
    /// Replace all characters from a set in mutable storage.
    static auto replaceAll(U8StringSharedStorage &storage, const CharSet &characters, Char replacement)
        -> U8StringSharedStorage &;
    /// Replace all characters from a set with text in mutable storage.
    static auto replaceAll(
        U8StringSharedStorage &storage, const CharSet &characters, const U8StringDataView &replacement)
        -> U8StringSharedStorage &;
    /// Replace all occurrences of decoded UTF-8 text in mutable storage.
    static auto replaceAll(
        U8StringSharedStorage &storage,
        const U8StringDataView &text,
        const U8StringDataView &replacement,
        CharCompareFn compareFn = {}) -> U8StringSharedStorage &;

public:
    /// Return storage with a byte range removed.
    [[nodiscard]] auto removed(unit::ByteRange range) const -> U8StringSharedStorage;
    /// Return storage with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U8StringSharedStorage;
    /// Return storage with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U8StringSharedStorage;
    /// Return storage with all occurrences of decoded UTF-8 text removed.
    [[nodiscard]] auto removed(const U8StringDataView &text, CharCompareFn compareFn = {}) const
        -> U8StringSharedStorage;
    /// Return storage keeping only a byte range.
    [[nodiscard]] auto kept(unit::ByteRange range) const -> U8StringSharedStorage;
    /// Return storage keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U8StringSharedStorage;
    /// Return storage with text inserted at a byte index.
    [[nodiscard]] auto inserted(unit::ByteIndex index, const U8StringDataView &text) const -> U8StringSharedStorage;
    /// Return storage with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U8StringDataView &text) const -> U8StringSharedStorage;
    /// Return storage with a byte range replaced by text.
    [[nodiscard]] auto replaced(unit::ByteRange range, const U8StringDataView &text) const -> U8StringSharedStorage;
    /// Return storage with a character range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U8StringDataView &text) const -> U8StringSharedStorage;
    /// Return storage with the first occurrence of decoded UTF-8 text removed.
    [[nodiscard]] auto removedFirst(const U8StringDataView &text, CharCompareFn compareFn = {}) const
        -> U8StringSharedStorage;
    /// Return storage with the first occurrence of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U8StringDataView &text, const U8StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> U8StringSharedStorage;
    /// Return storage with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U8StringSharedStorage;
    /// Return storage with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U8StringDataView &replacement) const
        -> U8StringSharedStorage;
    /// Return storage with all occurrences of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedAll(
        const U8StringDataView &text, const U8StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> U8StringSharedStorage;

private:
    template <typename T>
    [[nodiscard]] static auto spansOverlap(std::span<const T> first, std::span<const T> second) noexcept -> bool;
    [[nodiscard]] static auto characterBytes(Char character) noexcept -> std::array<char, 4>;
    [[nodiscard]] static auto characterByteSpan(Char character, const std::array<char, 4> &bytes) noexcept
        -> std::span<const char>;

    template <typename Predicate>
    [[nodiscard]] auto replacedCharacters(Predicate predicate, std::span<const char> replacement) const
        -> U8StringSharedStorage;
    [[nodiscard]] auto replacedText(
        const U8StringDataView &text, std::span<const char> replacement, CharCompareFn compareFn) const
        -> U8StringSharedStorage;
    template <typename Predicate>
    static auto replaceCharactersInStorage(
        U8StringSharedStorage &storage, Predicate predicate, std::span<const char> replacement)
        -> U8StringSharedStorage &;
    static auto replaceTextInStorage(
        U8StringSharedStorage &storage,
        const U8StringDataView &text,
        std::span<const char> replacement,
        CharCompareFn compareFn) -> U8StringSharedStorage &;
    [[nodiscard]] static auto byteRangeForCharacterRange(const U8StringDataView &data, unit::CpRange range) noexcept
        -> unit::ByteRange;
    [[nodiscard]] static auto byteIndexForCharacterIndex(const U8StringDataView &data, unit::CpIndex index) noexcept
        -> unit::ByteIndex;
    [[nodiscard]] static auto findFirstTextRange(
        const U8StringDataView &data, const U8StringDataView &text, CharCompareFn compareFn) noexcept
        -> unit::ByteRange;
    [[nodiscard]] static auto matchesText(
        std::span<const char> data, unit::ByteIndex start, std::span<const char> text, CharCompareFn compareFn) noexcept
        -> bool;
    [[nodiscard]] static auto endOfMatch(
        std::span<const char> data, unit::ByteIndex start, std::span<const char> text) noexcept -> unit::ByteIndex;
    [[nodiscard]] static auto charactersEqual(Char left, Char right, CharCompareFn compareFn) noexcept -> bool;

private:
    U8StringDataView _data;
    bool _sensitive{false};
};

template <typename Predicate>
auto U8StringModifyTools::replacedCharacters(Predicate predicate, const std::span<const char> replacement) const
    -> U8StringSharedStorage {
    const auto data = _data.dataSpan();
    if (data.empty()) {
        return {};
    }

    auto newSize = std::size_t{0};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (predicate(character)) {
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
        } else {
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, position.toSizeT() - characterStart.toSizeT(), "Modified string exceeds size bounds");
        }
    }
    U8StringSharedStorage::validateSize(newSize);
    auto storage = U8StringSharedStorage::forSize(newSize, _sensitive);
    auto writePosition = std::size_t{0};
    position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (predicate(character)) {
            if (!replacement.empty()) {
                std::memcpy(storage.dataForWrite() + writePosition, replacement.data(), replacement.size());
                writePosition = U8StringSharedStorage::checkedAddSize(
                    writePosition, replacement.size(), "Modified string write exceeds size bounds");
            }
        } else {
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            std::memcpy(storage.dataForWrite() + writePosition, data.data() + characterStart.toSizeT(), characterSize);
            writePosition = U8StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    return storage;
}

template <typename Predicate>
auto U8StringModifyTools::replaceCharactersInStorage(
    U8StringSharedStorage &storage, Predicate predicate, const std::span<const char> replacement)
    -> U8StringSharedStorage & {
    const auto dataView = storage.dataView();
    const auto data = dataView.dataSpan();
    if (data.empty()) {
        return storage;
    }

    auto newSize = std::size_t{0};
    auto hasMatch = false;
    auto canWriteInPlace = storage.isUniqueFullRange() && replacement.empty();
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf8::decodeCharOrReplace(data, position);
        const auto characterSize = position.toSizeT() - characterStart.toSizeT();
        if (predicate(character)) {
            hasMatch = true;
            newSize = U8StringSharedStorage::checkedAddSize(
                newSize, replacement.size(), "Modified string exceeds size bounds");
        } else {
            newSize =
                U8StringSharedStorage::checkedAddSize(newSize, characterSize, "Modified string exceeds size bounds");
        }
    }
    if (!hasMatch) {
        return storage;
    }
    if (!canWriteInPlace) {
        storage = U8StringModifyTools{dataView, storage.isSensitive()}.replacedCharacters(predicate, replacement);
        return storage;
    }

    auto writePosition = std::size_t{0};
    position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (!predicate(character)) {
            const auto characterSize = position.toSizeT() - characterStart.toSizeT();
            if (writePosition != characterStart.toSizeT()) {
                std::memmove(
                    storage.dataForWrite() + writePosition, data.data() + characterStart.toSizeT(), characterSize);
            }
            writePosition = U8StringSharedStorage::checkedAddSize(
                writePosition, characterSize, "Modified string write exceeds size bounds");
        }
    }
    storage.resize(newSize);
    return storage;
}

}
