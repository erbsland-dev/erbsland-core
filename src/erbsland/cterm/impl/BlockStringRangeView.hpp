// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStringData.hpp"

#include "../BlockIndex.hpp"
#include "../BlockRange.hpp"

#include "../../bgeo/BlockSize.hpp"
#include "../../text/CharSet.hpp"

#include <string_view>
#include <vector>

namespace erbsland::cterm::impl {

/// A non-owning read-only view over one range in `BlockStringData`.
///
/// This type keeps shared range algorithms out of the public `BlockString` and `BlockStringView` API classes. All
/// indexes used by public methods are local to the visible range; returned `BlockRange` values are absolute storage
/// ranges.
class BlockStringRangeView final {
public:
    using Storage = BlockStringData::Storage;                       ///< Storage container for the characters.
    using const_iterator = Storage::const_iterator;                 ///< Immutable forward iterator.
    using const_reverse_iterator = Storage::const_reverse_iterator; ///< Immutable reverse iterator.
    using difference_type = Storage::difference_type;               ///< Signed distance type.

public:
    /// Create a read-only view over a clamped storage range.
    /// @param data The backing storage that must outlive this view.
    /// @param range The absolute storage range to expose.
    explicit constexpr BlockStringRangeView(const BlockStringData &data, const BlockRange range) noexcept :
        _data{data}, _range{range} {}

public:
    /// Get the visible storage range.
    [[nodiscard]] auto range() const noexcept -> BlockRange { return _range; }
    /// Get the number of visible characters.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _range.length(); }
    /// Test if this range is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _range.isEmpty(); }
    /// Convert a local index into an absolute storage index.
    [[nodiscard]] auto storageIndex(BlockIndex localIndex) const noexcept -> BlockIndex;
    /// Access one character without bounds checking.
    [[nodiscard]] auto characterAt(BlockIndex localIndex) const noexcept -> const Block &;
    /// Get a const iterator to the first character.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Get a const iterator past the last character.
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Get a const iterator to the first character.
    [[nodiscard]] auto cbegin() const noexcept -> const_iterator;
    /// Get a const iterator past the last character.
    [[nodiscard]] auto cend() const noexcept -> const_iterator;
    /// Get a const reverse iterator to the last character.
    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator;
    /// Get a const reverse iterator past the first character.
    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator;
    /// Get a const reverse iterator to the last character.
    [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator;
    /// Get a const reverse iterator past the first character.
    [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator;
    /// Get the width of the visible range in terminal cells.
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Access one character without bounds checking.
    [[nodiscard]] auto operator[](BlockIndex index) const noexcept -> Block;
    /// Access one character with bounds checking.
    /// @param index The local character index.
    /// @param typeName The public type name to use in the exception message.
    /// @return A copy of the character at `index`.
    /// @throws std::out_of_range If `index` is outside the visible range.
    [[nodiscard]] auto at(BlockIndex index, std::string_view typeName) const -> Block;
    /// Count fully styled character matches.
    [[nodiscard]] auto count(const Block &character) const noexcept -> BlockCount;
    /// Count one-code-point matches, ignoring style.
    [[nodiscard]] auto count(text::Char character) const noexcept -> BlockCount;
    /// Find the next fully styled character match.
    [[nodiscard]] auto indexOf(const Block &character, BlockIndex startIndex = {}) const noexcept -> BlockIndex;
    /// Find the next one-code-point match, ignoring style.
    [[nodiscard]] auto indexOf(text::Char character, BlockIndex startIndex = {}) const noexcept -> BlockIndex;
    /// Find the next one-code-point character that is part of the given set.
    [[nodiscard]] auto indexOf(const text::CharSet &characterSet, BlockIndex startIndex = {}) const noexcept
        -> BlockIndex;
    /// Find the next one-code-point character that is not part of the given set.
    [[nodiscard]] auto indexNotOf(const text::CharSet &characterSet, BlockIndex startIndex = {}) const noexcept
        -> BlockIndex;
    /// Calculate an absolute storage sub-range from local range coordinates.
    [[nodiscard]] auto subRange(BlockRange range = BlockRange::all()) const noexcept -> BlockRange;
    /// Calculate an absolute storage range cropped to a terminal display width.
    [[nodiscard]] auto croppedRange(bgeo::BlockCoordinate displayWidth, bgeo::Alignment alignment) const noexcept
        -> BlockRange;
    /// Calculate an absolute storage range trimmed at both ends.
    [[nodiscard]] auto trimmedRange(const text::CharSet &characters) const noexcept -> BlockRange;
    /// Test if this range contains control characters.
    [[nodiscard]] auto containsControlCharacters() const noexcept -> bool;
    /// Split the range into absolute word ranges.
    [[nodiscard]] auto splitWordRanges() const noexcept -> std::vector<BlockRange>;
    /// Split the range into absolute line ranges.
    [[nodiscard]] auto splitLineRanges() const noexcept -> std::vector<BlockRange>;
    /// Count how many terminal lines this range occupies for a given terminal width.
    [[nodiscard]] auto terminalLines(int width) const noexcept -> int;
    /// Get the natural rectangular size for this range without wrapping.
    [[nodiscard]] auto naturalBlockTextSize() const noexcept -> bgeo::BlockSize;

private:
    [[nodiscard]] auto rawSize() const noexcept -> std::size_t { return length().toSizeT(); }
    [[nodiscard]] auto rawStorageIndex(BlockIndex localIndex) const noexcept -> std::size_t;

private:
    const BlockStringData &_data; ///< The backing storage.
    BlockRange _range;            ///< The visible absolute range inside `_data`.
};

}
