// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16Encoding.hpp"
#include "U16StringDataView.hpp"
#include "U16StringSharedStorage.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../../unit/U16Index.hpp"
#include "../../../unit/U16Range.hpp"
#include "../../CharCompareFn.hpp"
#include "../../CharSet.hpp"

#include <array>
#include <cstring>
#include <optional>
#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Modification algorithms for UTF-16 strings.
/// Change-aware replacement and removal preserve unchanged storage; kept ranges deliberately create pruned copies.
/// @tested{StringReplacementTest U16StringTest StringUnchangedResultTest}
class U16StringModifyTools final {
public:
    /// Create modification tools for the given UTF-16 data.
    explicit constexpr U16StringModifyTools(const U16StringDataView &data) noexcept : _data{data} {}

public:
    /// Remove a UTF-16 data range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U16StringSharedStorage &storage, unit::U16Range range) -> U16StringSharedStorage &;
    /// Remove a character range from mutable storage, reusing unique full-range storage when possible.
    static auto remove(U16StringSharedStorage &storage, unit::CpRange range) -> U16StringSharedStorage &;
    /// Remove all characters from a set from mutable storage, reusing unique full-range storage when possible.
    static auto removeAll(U16StringSharedStorage &storage, const CharSet &characters) -> U16StringSharedStorage &;
    /// Remove all occurrences of decoded UTF-16 text from mutable storage.
    static auto remove(U16StringSharedStorage &storage, const U16StringDataView &text, CharCompareFn compareFn = {})
        -> U16StringSharedStorage &;
    /// Keep a UTF-16 data range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U16StringSharedStorage &storage, unit::U16Range range) -> U16StringSharedStorage &;
    /// Keep a character range in mutable storage, reusing unique full-range storage when possible.
    static auto keep(U16StringSharedStorage &storage, unit::CpRange range) -> U16StringSharedStorage &;
    /// Insert text at a UTF-16 data index in mutable storage.
    static auto insert(U16StringSharedStorage &storage, unit::U16Index index, const U16StringDataView &text)
        -> U16StringSharedStorage &;
    /// Insert text at a character index in mutable storage.
    static auto insert(U16StringSharedStorage &storage, unit::CpIndex index, const U16StringDataView &text)
        -> U16StringSharedStorage &;
    /// Replace a UTF-16 data range in mutable storage.
    static auto replace(U16StringSharedStorage &storage, unit::U16Range range, const U16StringDataView &text)
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
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedIfChanged(unit::U16Range range) const -> std::optional<U16StringSharedStorage>;
    /// Return storage with a character-based range removed.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedIfChanged(unit::CpRange range) const -> std::optional<U16StringSharedStorage>;
    /// Return storage with all characters from the set removed.
    /// @param characters The characters to remove.
    /// @return Modified storage, or `std::nullopt` if no match is found.
    [[nodiscard]] auto removedAllIfChanged(const CharSet &characters) const -> std::optional<U16StringSharedStorage>;
    /// Return storage with all occurrences of decoded UTF-16 text removed.
    /// @param text The decoded text to remove.
    /// @param compareFn The optional character comparison function.
    /// @return Modified storage, or `std::nullopt` if no match is found.
    [[nodiscard]] auto removedAllIfChanged(const U16StringDataView &text, CharCompareFn compareFn = {}) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage keeping only a UTF-16 data range.
    [[nodiscard]] auto kept(unit::U16Range range) const -> U16StringSharedStorage;
    /// Return storage keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U16StringSharedStorage;
    /// Return storage with text inserted at a UTF-16 data index.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto insertedIfChanged(unit::U16Index index, const U16StringDataView &text) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with text inserted at a character index.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto insertedIfChanged(unit::CpIndex index, const U16StringDataView &text) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with a UTF-16 data range replaced by text.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedIfChanged(unit::U16Range range, const U16StringDataView &text) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with a character range replaced by text.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedIfChanged(unit::CpRange range, const U16StringDataView &text) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with the first occurrence of decoded UTF-16 text removed.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedFirstIfChanged(const U16StringDataView &text, CharCompareFn compareFn = {}) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with the first occurrence of decoded UTF-16 text replaced.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedFirstIfChanged(
        const U16StringDataView &text, const U16StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with all characters from the set replaced by one character.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(const CharSet &characters, Char replacement) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with all characters from the set replaced by text.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(const CharSet &characters, const U16StringDataView &replacement) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with all occurrences of decoded UTF-16 text replaced.
    /// @param text The decoded text to replace.
    /// @param replacement The replacement character or text.
    /// @param compareFn The optional character comparison function.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(
        const U16StringDataView &text, const U16StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> std::optional<U16StringSharedStorage>;

private:
    /// Append native data units without decoding or modifying their representation.
    /// @param storage The destination storage.
    /// @param data The native data units to append.
    static void appendData(U16StringSharedStorage &storage, std::span<const char16_t> data);
    template <typename T>
    /// Test whether two spans overlap in memory.
    [[nodiscard]] static auto spansOverlap(std::span<const T> first, std::span<const T> second) noexcept -> bool;
    /// Encode one character into UTF-16 code units.
    [[nodiscard]] static auto characterBytes(Char character) noexcept -> std::array<char16_t, 2>;
    /// Get the encoded UTF-16 code units for one character.
    [[nodiscard]] static auto characterByteSpan(Char character, const std::array<char16_t, 2> &bytes) noexcept
        -> std::span<const char16_t>;

    template <typename Predicate>
    /// Return storage with characters selected by a predicate replaced with UTF-16 text.
    [[nodiscard]] auto replacedCharactersIfChanged(Predicate predicate, std::span<const char16_t> replacement) const
        -> std::optional<U16StringSharedStorage>;
    /// Return storage with matching text replaced.
    [[nodiscard]] auto replacedTextIfChanged(
        const U16StringDataView &text, std::span<const char16_t> replacement, CharCompareFn compareFn) const
        -> std::optional<U16StringSharedStorage>;
    template <typename Predicate>
    /// Replace characters selected by a predicate directly in the shared storage with UTF-16 text.
    static auto replaceCharactersInStorage(
        U16StringSharedStorage &storage, Predicate predicate, std::span<const char16_t> replacement)
        -> U16StringSharedStorage &;
    /// Replace matching text in existing storage.
    static auto replaceTextInStorage(
        U16StringSharedStorage &storage,
        const U16StringDataView &text,
        std::span<const char16_t> replacement,
        CharCompareFn compareFn) -> U16StringSharedStorage &;
    /// Convert a character range into a UTF-16 data range.
    [[nodiscard]] static auto dataRangeForCharacterRange(const U16StringDataView &data, unit::CpRange range) noexcept
        -> unit::U16Range;
    /// Convert a character index into a UTF-16 data index.
    [[nodiscard]] static auto dataIndexForCharacterIndex(const U16StringDataView &data, unit::CpIndex index) noexcept
        -> unit::U16Index;

private:
    U16StringDataView _data;
};

template <typename Predicate>
auto U16StringModifyTools::replacedCharactersIfChanged(
    Predicate predicate, const std::span<const char16_t> replacement) const -> std::optional<U16StringSharedStorage> {
    const auto data = _data.dataSpan();
    auto result = std::optional<U16StringSharedStorage>{};
    auto copyStart = std::size_t{0};
    auto position = unit::U16Index::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf16::decodeCharOrReplace(data, position);
        if (!predicate(character)) {
            continue;
        }
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
    auto position = unit::U16Index::zero();
    while (position.toSizeT() < data.size()) {
        const auto characterStart = position;
        const auto character = utf16::decodeCharOrReplace(data, position);
        const auto characterSize = position.toSizeT() - characterStart.toSizeT();
        if (predicate(character)) {
            hasMatch = true;
            newSize = U16StringSharedStorage::checkedAddSize(newSize, replacement.size());
        } else {
            newSize = U16StringSharedStorage::checkedAddSize(newSize, characterSize);
        }
    }
    if (!hasMatch) {
        return storage;
    }
    if (!canWriteInPlace) {
        if (auto result = U16StringModifyTools{dataView}.replacedCharactersIfChanged(predicate, replacement)) {
            storage = std::move(result.value());
        }
        return storage;
    }

    auto writePosition = std::size_t{0};
    position = unit::U16Index::zero();
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
            writePosition = U16StringSharedStorage::checkedAddSize(writePosition, characterSize);
        }
    }
    storage.resize(newSize);
    return storage;
}

}
