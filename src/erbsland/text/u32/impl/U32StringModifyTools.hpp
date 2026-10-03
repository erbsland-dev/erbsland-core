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
#include <optional>
#include <span>
#include <utility>

namespace erbsland::text::impl {

/// Modification algorithms for UTF-32 strings.
/// Change-aware replacement and removal preserve unchanged storage; kept ranges deliberately create pruned copies.
/// @tested{StringReplacementTest U32StringTest StringUnchangedResultTest}
class U32StringModifyTools final {
public:
    /// Create modification tools for `data`.
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
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedIfChanged(unit::CpRange range) const -> std::optional<U32StringSharedStorage>;
    /// Return storage with all characters from the set removed.
    /// @param characters The characters to remove.
    /// @return Modified storage, or `std::nullopt` if no match is found.
    [[nodiscard]] auto removedAllIfChanged(const CharSet &characters) const -> std::optional<U32StringSharedStorage>;
    /// Return storage with all occurrences of decoded UTF-32 text removed.
    /// @param text The decoded text to remove.
    /// @param compareFn The optional character comparison function.
    /// @return Modified storage, or `std::nullopt` if no match is found.
    [[nodiscard]] auto removedAllIfChanged(const U32StringDataView &text, CharCompareFn compareFn = {}) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U32StringSharedStorage;
    /// Return storage with text inserted at a character index.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto insertedIfChanged(unit::CpIndex index, const U32StringDataView &text) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with a character range replaced by text.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedIfChanged(unit::CpRange range, const U32StringDataView &text) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with the first occurrence of decoded UTF-32 text removed.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto removedFirstIfChanged(const U32StringDataView &text, CharCompareFn compareFn = {}) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with the first occurrence of decoded UTF-32 text replaced.
    /// @return Changed storage, or `std::nullopt` if the native data is unchanged.
    [[nodiscard]] auto replacedFirstIfChanged(
        const U32StringDataView &text, const U32StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with all characters from the set replaced by one character.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(const CharSet &characters, Char replacement) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with all characters from the set replaced by text.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(const CharSet &characters, const U32StringDataView &replacement) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with all occurrences of decoded UTF-32 text replaced.
    /// @param text The decoded text to replace.
    /// @param replacement The replacement character or text.
    /// @param compareFn The optional character comparison function.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedAllIfChanged(
        const U32StringDataView &text, const U32StringDataView &replacement, CharCompareFn compareFn = {}) const
        -> std::optional<U32StringSharedStorage>;

private:
    /// Append native data units without decoding or modifying their representation.
    /// @param storage The destination storage.
    /// @param data The native data units to append.
    static void appendData(U32StringSharedStorage &storage, std::span<const char32_t> data);
    /// Test whether two spans share any storage.
    /// @tparam T The span element type.
    /// @param first The first span.
    /// @param second The second span.
    /// @return `true` if the spans overlap.
    template <typename T>
    [[nodiscard]] static auto spansOverlap(std::span<const T> first, std::span<const T> second) noexcept -> bool;
    /// Encode one character into a fixed UTF-32 buffer.
    /// @param character The character to encode.
    /// @return The buffer containing the encoded character.
    [[nodiscard]] static auto characterBytes(Char character) noexcept -> std::array<char32_t, 2>;
    /// Get the populated part of an encoded character buffer.
    /// @param character The encoded character.
    /// @param bytes The buffer returned by `characterBytes()`.
    /// @return A span over the populated code points.
    [[nodiscard]] static auto characterByteSpan(Char character, const std::array<char32_t, 2> &bytes) noexcept
        -> std::span<const char32_t>;
    /// Return storage with selected characters replaced.
    /// @tparam Predicate The character-selection predicate.
    /// @param predicate The predicate selecting characters to replace.
    /// @param replacement The UTF-32 replacement text.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    template <typename Predicate>
    [[nodiscard]] auto replacedCharactersIfChanged(Predicate predicate, std::span<const char32_t> replacement) const
        -> std::optional<U32StringSharedStorage>;
    /// Return storage with matching text replaced.
    /// @param text The text to replace.
    /// @param replacement The UTF-32 replacement text.
    /// @param compareFn The character comparison function.
    /// @return Modified storage, or `std::nullopt` if the original data is unchanged.
    [[nodiscard]] auto replacedTextIfChanged(
        const U32StringDataView &text, std::span<const char32_t> replacement, CharCompareFn compareFn) const
        -> std::optional<U32StringSharedStorage>;
    /// Replace selected characters in storage, preserving unique storage when possible.
    /// @tparam Predicate The character-selection predicate.
    /// @param storage The storage to modify.
    /// @param predicate The predicate selecting characters to replace.
    /// @param replacement The UTF-32 replacement text.
    /// @return The modified storage.
    template <typename Predicate>
    static auto replaceCharactersInStorage(
        U32StringSharedStorage &storage, Predicate predicate, std::span<const char32_t> replacement)
        -> U32StringSharedStorage &;
    /// Replace matching text in storage, preserving unique storage when possible.
    /// @param storage The storage to modify.
    /// @param text The text to replace.
    /// @param replacement The UTF-32 replacement text.
    /// @param compareFn The character comparison function.
    /// @return The modified storage.
    static auto replaceTextInStorage(
        U32StringSharedStorage &storage,
        const U32StringDataView &text,
        std::span<const char32_t> replacement,
        CharCompareFn compareFn) -> U32StringSharedStorage &;

private:
    U32StringDataView _data;
};

template <typename Predicate>
auto U32StringModifyTools::replacedCharactersIfChanged(
    Predicate predicate, const std::span<const char32_t> replacement) const -> std::optional<U32StringSharedStorage> {
    const auto data = _data.dataSpan();
    auto result = std::optional<U32StringSharedStorage>{};
    auto copyStart = std::size_t{0};
    auto position = unit::CpIndex::zero();
    while (position.toSizeT() < data.size()) {
        const auto begin = position.toSizeT();
        const auto character = utf32::decodeCharOrReplace(data, position);
        if (!predicate(character)) {
            continue;
        }
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
            newSize = U32StringSharedStorage::checkedAddSize(newSize, replacement.size());
        } else {
            newSize = U32StringSharedStorage::checkedAddSize(newSize, characterSize);
        }
    }
    if (!hasMatch) {
        return storage;
    }
    if (!canWriteInPlace) {
        if (auto result = U32StringModifyTools{dataView}.replacedCharactersIfChanged(predicate, replacement)) {
            storage = std::move(result.value());
        }
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
            writePosition = U32StringSharedStorage::checkedAddSize(writePosition, characterSize);
        }
    }
    storage.resize(newSize);
    return storage;
}

}
