// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockCount.hpp"
#include "BlockIndex.hpp"
#include "BlockRange.hpp"
#include "BlockString.hpp"
#include "ParagraphSpacing.hpp"

#include "../text/Char.hpp"
#include "../text/CharSet.hpp"
#include "../text/StringSide.hpp"

#include <vector>

namespace erbsland::cterm {

namespace impl {
class BlockStringData;
}

/// A read-only view onto shared terminal string data.
class BlockStringView final {
public:
    using Storage = std::vector<Block>;                             ///< Storage container for the characters.
    using const_iterator = Storage::const_iterator;                 ///< Immutable forward iterator.
    using const_reverse_iterator = Storage::const_reverse_iterator; ///< Immutable reverse iterator.
    using difference_type = Storage::difference_type;               ///< Signed distance type.
    using value_type = Storage::value_type;                         ///< Stored value type.
    using const_reference = Storage::const_reference;               ///< Immutable element reference.
    using const_pointer = Storage::const_pointer;                   ///< Immutable element pointer.

public:
    /// Create an empty string view.
    BlockStringView() noexcept;
    /// Create a string view from a string.
    /// This conversion is implicit so APIs can migrate from `BlockString` to `BlockStringView`.
    /// @param string The source string.
    BlockStringView(const BlockString &string) noexcept;

    // defaults
    ~BlockStringView() = default;
    BlockStringView(const BlockStringView &) = default;
    BlockStringView(BlockStringView &&) = default;
    auto operator=(const BlockStringView &) -> BlockStringView & = default;
    auto operator=(BlockStringView &&) -> BlockStringView & = default;

public: // operators
    /// Access one character without bounds checking.
    /// @param index The character index.
    /// @return A copy of the character at `index`, or `Block{}` if `index` is out of bounds.
    [[nodiscard]] auto operator[](BlockIndex index) const noexcept -> Block;

public: // accessors
    /// Get the number of stored characters.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _range.length(); }
    /// Get the width of the string in terminal cells.
    /// @return The sum of all character display widths.
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Test if this string view is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _range.isEmpty(); }
    /// Access one character with bounds checking.
    /// @param index The character index.
    /// @return A copy of the character at `index`.
    [[nodiscard]] auto at(BlockIndex index) const -> Block;
    /// Get an iterator to the first character.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Get an iterator past the last character.
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
    /// Count the number of characters matching a fully styled character.
    /// @param character The character to count.
    /// @return The number of matching characters in this view.
    [[nodiscard]] auto count(const Block &character) const noexcept -> BlockCount;
    /// Count the number of characters matching one code point regardless of style.
    /// @param character The character to count.
    /// @return The number of matching characters in this view.
    [[nodiscard]] auto count(text::Char character) const noexcept -> BlockCount;
    /// Get the index of the next character with a given full style.
    /// @param character The character to search for.
    /// @param startIndex The first local index to inspect.
    /// @return The local index of the next match, or `BlockIndex::noIndex()`.
    [[nodiscard]] auto indexOf(const Block &character, BlockIndex startIndex = {}) const noexcept -> BlockIndex;
    /// Get the index of the next character matching one code point regardless of style.
    /// @param character The character to search for.
    /// @param startIndex The first local index to inspect.
    /// @return The local index of the next match, or `BlockIndex::noIndex()`.
    [[nodiscard]] auto indexOf(text::Char character, BlockIndex startIndex = {}) const noexcept -> BlockIndex;
    /// Get the index of the next matching character.
    /// Ignores the character style.
    /// If startIndex is out of bounds, returns `BlockIndex::noIndex()`.
    /// If no character is found, returns `BlockIndex::noIndex()`.
    /// This function matches **any** character in the character set.
    /// @param characterSet The character-set to search for. Only compares single-code-point characters.
    /// @param startIndex The start index to search from. Defaults to 0.
    /// @return The index of the next character or `BlockIndex::noIndex()` if not found.
    [[nodiscard]] auto indexOf(const text::CharSet &characterSet, BlockIndex startIndex = {}) const noexcept
        -> BlockIndex;
    /// Get the index of the next **not** matching character.
    /// Ignores the character style.
    /// If startIndex is out of bounds, returns `BlockIndex::noIndex()`.
    /// If no character is found, returns `BlockIndex::noIndex()`.
    /// This function matches if **no** character in the character set matches the text.
    /// @param characterSet The character-set to search for. Only compares single-code-point characters.
    /// @param startIndex The start index to search from. Defaults to 0.
    /// @return The index of the next character or `BlockIndex::noIndex()` if not found.
    [[nodiscard]] auto indexNotOf(const text::CharSet &characterSet, BlockIndex startIndex = {}) const noexcept
        -> BlockIndex;
    /// Get a sub-view.
    /// @param range The block range to slice.
    /// @return The substring view or an empty view if the range is out of bounds.
    [[nodiscard]] auto slice(BlockRange range = BlockRange::all()) const noexcept -> BlockStringView;
    /// Get the initial or trailing block-based portion of this string.
    [[nodiscard]] auto slice(text::StringSide side, BlockCount count) const noexcept -> BlockStringView;
    /// Get a substring that fits into the given display width.
    /// If a double-sized character is at the edge, it isn't included in the result.
    /// Therefore, the resulting string may be shorter than the display width.
    /// @param displayWidth The maximum width of the substring in display units.
    /// @param alignment The alignment of the cropped text. Only `bgeo::Alignment::Left` and `bgeo::Alignment::Right`
    /// are supported.
    /// @return The cropped substring or an empty string if displayWidth is <=0.
    [[nodiscard]] auto croppedToDisplayWidth(
        bgeo::BlockCoordinate displayWidth, bgeo::Alignment alignment) const noexcept -> BlockStringView;
    /// Trim the given characters from the beginning and end of the view.
    /// Only single-code-point characters are matched.
    /// @param characters The characters to remove from both ends. If empty, removes space, tab, and newline characters.
    /// @return A trimmed string view.
    [[nodiscard]] auto trimmed(const text::CharSet &characters = defaultTrimCharacters()) const noexcept
        -> BlockStringView;

public: // tests
    /// Test if this string view contains control characters.
    /// As most control codes are filtered on construction, this mainly tests for NL and TAB.
    [[nodiscard]] auto containsControlCharacters() const noexcept -> bool;

public: // tools
    /// Split the view into words at space, tab, carriage return, or newline characters.
    /// @return A sequence of word views.
    [[nodiscard]] auto splitWords() const noexcept -> std::vector<BlockStringView>;
    /// Wrap this view into lines that have a maximum display width.
    /// Paragraph breaks from newline characters are preserved using the selected paragraph spacing.
    /// @param width The maximum terminal width in cells. Must be greater than zero.
    /// @param paragraphSpacing The spacing to use between newline-separated paragraphs.
    /// @return A sequence of materialized lines.
    [[nodiscard]] auto wrapIntoLines(
        int width, ParagraphSpacing paragraphSpacing = ParagraphSpacing::SingleLine) const noexcept -> BlockStringLines;
    /// Count how many terminal lines this view occupies for a given terminal width.
    /// @param width The available terminal width in cells. Must be greater than zero.
    /// @return The number of occupied terminal lines.
    [[nodiscard]] auto terminalLines(int width) const noexcept -> int;
    /// Get the natural rectangular size for this text view without wrapping.
    /// The returned size is at least 1x1, preserves explicit non-trailing newline characters as separate lines,
    /// and uses terminal cell width for wide and combining characters.
    /// @return The natural text size in terminal cells.
    [[nodiscard]] auto naturalBlockTextSize() const noexcept -> bgeo::BlockSize;
    /// Calculate the height required to render this text view with `WritableBuffer::drawBlockText()`.
    /// The given width is the full target rectangle width, including margins configured in `options`.
    /// @param width The available rectangle width in terminal cells.
    /// @param options The text options used for paragraph layout.
    /// @return The required rectangle height in terminal cells.
    [[nodiscard]] auto wrappedBlockTextHeight(
        bgeo::BlockCoordinate width, const BlockTextOptions &options) const noexcept -> bgeo::BlockCoordinate;
    /// Split this view into individual lines.
    /// The string is split at the NL character that is not included in the result.
    /// Empty lines are preserved.
    /// A NL at the end of the view does not generate an additional empty line.
    /// @return A sequence of line views.
    [[nodiscard]] auto splitLines() const noexcept -> std::vector<BlockStringView>;
    /// Create a new string with the given style applied as a base.
    /// @param style The style used as base for the resulting string.
    /// @return A new string with the base style applied.
    [[nodiscard]] auto withBase(BlockStyle style) const noexcept -> BlockString;

private:
    friend class BlockString;

    BlockStringView(impl::BlockStringDataPtr data, BlockRange range) noexcept;

    [[nodiscard]] auto characterAt(BlockIndex localIndex) const noexcept -> const Block &;
    [[nodiscard]] static auto defaultTrimCharacters() -> const text::CharSet &;

private:
    impl::BlockStringDataPtr _data; ///< Shared backing storage.
    BlockRange _range;              ///< Visible sub-range inside `_data`.
};

}
