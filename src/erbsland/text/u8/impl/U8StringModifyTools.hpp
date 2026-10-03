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
#include <optional>
#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Modification algorithms for UTF-8 strings.
/// Change-aware replacement and removal preserve unchanged storage; kept ranges deliberately create pruned copies.
/// @tested{StringReplacementTest U8StringModifierTest StringUnchangedResultTest}
class U8StringModifyTools final {
public:
    /// Create modification tools for the given UTF-8 data.
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
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedIfChanged(unit::ByteRange range) const -> std::optional<U8StringSharedStorage>;
    /// Return storage with a character-based range removed.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedIfChanged(unit::CpRange range) const -> std::optional<U8StringSharedStorage>;
    /// Return storage with all characters from the set removed.
    /// @param characters The characters to remove.
    /// @return Modified storage, or `std::nullopt` if no match is found.
    [[nodiscard]] auto removedAllIfChanged(const CharSet &characters) const -> std::optional<U8StringSharedStorage>;
    /// Return storage with all occurrences of decoded UTF-8 text removed.
    /// @param text The decoded text to remove.
    /// @param compareFn The optional character comparison function.
    /// @return Modified storage, or `std::nullopt` if no match is found.
    [[nodiscard]] auto removedAllIfChanged(const U8StringDataView &text, CharCompareFn compareFn = {}) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage keeping only a byte range.
    [[nodiscard]] auto kept(unit::ByteRange range) const -> U8StringSharedStorage;
    /// Return storage keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U8StringSharedStorage;
    /// Return storage with text inserted at a byte index.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto insertedIfChanged(unit::ByteIndex index, const U8StringDataView &text) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with text inserted at a character index.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto insertedIfChanged(unit::CpIndex index, const U8StringDataView &text) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with a byte range replaced by text.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedIfChanged(unit::ByteRange range, const U8StringDataView &text) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with a character range replaced by text.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedIfChanged(unit::CpRange range, const U8StringDataView &text) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with the first occurrence of decoded UTF-8 text removed.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedFirstIfChanged(const U8StringDataView &text, CharCompareFn compareFn = {}) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with the first occurrence of decoded UTF-8 text replaced.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedFirstIfChanged(
        const U8StringDataView &text, const U8StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with all characters from the set replaced by one character.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(const CharSet &characters, Char replacement) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with all characters from the set replaced by text.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(const CharSet &characters, const U8StringDataView &replacement) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with all occurrences of decoded UTF-8 text replaced.
    /// @param text The decoded text to replace.
    /// @param replacement The replacement character or text.
    /// @param compareFn The optional character comparison function.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(
        const U8StringDataView &text, const U8StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> std::optional<U8StringSharedStorage>;

private:
    /// Append native data units without decoding or modifying their representation.
    /// @param storage The destination storage.
    /// @param data The native data units to append.
    static void appendData(U8StringSharedStorage &storage, std::span<const char> data);
    /// Test whether two spans overlap in memory.
    template <typename T>
    [[nodiscard]] static auto spansOverlap(std::span<const T> first, std::span<const T> second) noexcept -> bool;
    /// Encode one character into UTF-8 code units.
    [[nodiscard]] static auto characterBytes(Char character) noexcept -> std::array<char, 4>;
    /// Get the encoded UTF-8 code units for one character.
    [[nodiscard]] static auto characterByteSpan(Char character, const std::array<char, 4> &bytes) noexcept
        -> std::span<const char>;
    /// Return storage with characters selected by a predicate replaced with UTF-8 text.
    template <typename Predicate>
    [[nodiscard]] auto replacedCharactersIfChanged(Predicate predicate, std::span<const char> replacement) const
        -> std::optional<U8StringSharedStorage>;
    /// Return storage with matching text replaced.
    [[nodiscard]] auto replacedTextIfChanged(
        const U8StringDataView &text, std::span<const char> replacement, CharCompareFn compareFn) const
        -> std::optional<U8StringSharedStorage>;
    /// Replace characters selected by a predicate directly in the shared storage with UTF-8 text.
    template <typename Predicate>
    static auto replaceCharactersInStorage(
        U8StringSharedStorage &storage, Predicate predicate, std::span<const char> replacement)
        -> U8StringSharedStorage &;
    /// Replace matching text in existing storage.
    static auto replaceTextInStorage(
        U8StringSharedStorage &storage,
        const U8StringDataView &text,
        std::span<const char> replacement,
        CharCompareFn compareFn) -> U8StringSharedStorage &;
    /// Convert a character range into a byte range.
    [[nodiscard]] static auto byteRangeForCharacterRange(const U8StringDataView &data, unit::CpRange range) noexcept
        -> unit::ByteRange;
    /// Convert a character index into a byte index.
    [[nodiscard]] static auto byteIndexForCharacterIndex(const U8StringDataView &data, unit::CpIndex index) noexcept
        -> unit::ByteIndex;

private:
    U8StringDataView _data;
    bool _sensitive{false};
};

template <typename Predicate>
auto U8StringModifyTools::replacedCharactersIfChanged(
    Predicate predicate, const std::span<const char> replacement) const -> std::optional<U8StringSharedStorage> {
    const auto data = _data.dataSpan();
    auto result = std::optional<U8StringSharedStorage>{};
    auto copyStart = std::size_t{0};
    auto position = unit::ByteIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf8::decodeCharOrReplace(data, position);
        if (!predicate(character)) {
            continue;
        }
        const auto matchSize = position.toSizeT() - begin;
        if (matchSize == replacement.size() &&
            std::memcmp(data.data() + begin, replacement.data(), matchSize * sizeof(char)) == 0) {
            continue;
        }
        if (!result) {
            result.emplace();
            const auto initialCapacity =
                U8StringSharedStorage::checkedAddSize(data.size() - matchSize, replacement.size());
            if (initialCapacity > 0U) {
                result->ensureMutableCapacity(initialCapacity);
            }
            if (_sensitive) {
                result->markAsSensitive();
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
            newSize = U8StringSharedStorage::checkedAddSize(newSize, replacement.size());
        } else {
            newSize = U8StringSharedStorage::checkedAddSize(newSize, characterSize);
        }
    }
    if (!hasMatch) {
        return storage;
    }
    if (!canWriteInPlace) {
        if (auto result = U8StringModifyTools{dataView, storage.isSensitive()}.replacedCharactersIfChanged(
                predicate, replacement)) {
            storage = std::move(result.value());
        }
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
            writePosition = U8StringSharedStorage::checkedAddSize(writePosition, characterSize);
        }
    }
    storage.resize(newSize);
    return storage;
}

}
