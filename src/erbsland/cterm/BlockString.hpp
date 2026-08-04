// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockCount.hpp"
#include "BlockIndex.hpp"
#include "BlockRange.hpp"
#include "BlockString_fwd.hpp"
#include "BlockStringEditor.hpp"
#include "ParagraphSpacing.hpp"

#include "impl/BlockStringData_fwd.hpp"

#include "../text/Char.hpp"
#include "../text/CharSet.hpp"
#include "../text/StringSide.hpp"

#include <vector>

namespace erbsland::cterm {

/// A sequence of completed terminal text lines.
using BlockStringLines = std::vector<BlockString>;

/// An owning read-only terminal string value backed by shared storage.
class BlockString final {
public:
    using Storage = std::vector<Block>;                             ///< Storage container for the characters.
    using const_iterator = Storage::const_iterator;                 ///< Immutable forward iterator.
    using const_reverse_iterator = Storage::const_reverse_iterator; ///< Immutable reverse iterator.
    using difference_type = Storage::difference_type;               ///< Signed distance type.
    using value_type = Storage::value_type;                         ///< Stored value type.
    using const_reference = Storage::const_reference;               ///< Immutable element reference.
    using const_pointer = Storage::const_pointer;                   ///< Immutable element pointer.

public:
    /// Create an empty read-only string.
    BlockString() noexcept;
    /// Create a terminal string from UTF-8 text.
    explicit BlockString(const text::String &string);
    /// Create a terminal string from UTF-8 text with a uniform style.
    explicit BlockString(const text::String &string, BlockStyle style);
    /// Create a terminal string from UTF-32 text.
    explicit BlockString(const text::U32String &string);
    /// Create a terminal string from UTF-32 text with a uniform style.
    explicit BlockString(const text::U32String &string, BlockStyle style);
    /// Create a terminal string repeating the same block.
    explicit BlockString(BlockCount count, Block character) noexcept;
    /// Create a read-only string from a string.
    /// This conversion is implicit so APIs can migrate from `BlockStringEditor` to `BlockString`.
    /// @param string The source string.
    BlockString(const BlockStringEditor &string) noexcept;

    // defaults
    ~BlockString() = default;
    BlockString(const BlockString &) = default;
    BlockString(BlockString &&other) noexcept;

    // defaults
    auto operator=(const BlockString &) -> BlockString & = default;
    /// Move another terminal string into this string.
    auto operator=(BlockString &&other) noexcept -> BlockString &;

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
    /// Test if this read-only string is empty.
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
    /// @return The number of matching characters in this string.
    [[nodiscard]] auto count(const Block &character) const noexcept -> BlockCount;
    /// Count the number of characters matching one code point regardless of style.
    /// @param character The character to count.
    /// @return The number of matching characters in this string.
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
    /// Get an owning read-only slice.
    /// @param range The block range to slice.
    /// @return The substring or an empty string if the range is out of bounds.
    [[nodiscard]] auto slice(BlockRange range = BlockRange::all()) const noexcept -> BlockString;
    /// Get the initial or trailing block-based portion of this string.
    [[nodiscard]] auto slice(text::StringSide side, BlockCount count) const noexcept -> BlockString;
    /// Get a substring that fits into the given display width.
    /// If a double-sized character is at the edge, it isn't included in the result.
    /// Therefore, the resulting string may be shorter than the display width.
    /// @param displayWidth The maximum width of the substring in display units.
    /// @param alignment The alignment of the cropped text. Only `bgeo::Alignment::Left` and `bgeo::Alignment::Right`
    /// are supported.
    /// @return The cropped substring or an empty string if displayWidth is <=0.
    [[nodiscard]] auto croppedToDisplayWidth(
        bgeo::BlockCoordinate displayWidth, bgeo::Alignment alignment) const noexcept -> BlockString;
    /// Trim the given characters from the beginning and end of the string.
    /// Only single-code-point characters are matched.
    /// @param characters The characters to remove from both ends. If empty, removes space, tab, and newline characters.
    /// @return A trimmed read-only string.
    [[nodiscard]] auto trimmed(const text::CharSet &characters = defaultTrimCharacters()) const noexcept -> BlockString;

public: // tests
    /// Test if this read-only string contains control characters.
    /// As most control codes are filtered on construction, this mainly tests for NL and TAB.
    [[nodiscard]] auto containsControlCharacters() const noexcept -> bool;

public: // tools
    /// Split the string into words at space, tab, carriage return, or newline characters.
    /// @return A sequence of owning read-only word slices.
    [[nodiscard]] auto splitWords() const noexcept -> std::vector<BlockString>;
    /// Wrap this string into lines that have a maximum display width.
    /// Paragraph breaks from newline characters are preserved using the selected paragraph spacing.
    /// @param width The maximum terminal width in cells. Must be greater than zero.
    /// @param paragraphSpacing The spacing to use between newline-separated paragraphs.
    /// @return A sequence of materialized lines.
    [[nodiscard]] auto wrapIntoLines(
        int width, ParagraphSpacing paragraphSpacing = ParagraphSpacing::SingleLine) const noexcept -> BlockStringLines;
    /// Count how many terminal lines this string occupies for a given terminal width.
    /// @param width The available terminal width in cells. Must be greater than zero.
    /// @return The number of occupied terminal lines.
    [[nodiscard]] auto terminalLines(int width) const noexcept -> int;
    /// Get the natural rectangular size for this text without wrapping.
    /// The returned size is at least 1x1, preserves explicit non-trailing newline characters as separate lines,
    /// and uses terminal cell width for wide and combining characters.
    /// @return The natural text size in terminal cells.
    [[nodiscard]] auto naturalBlockTextSize() const noexcept -> bgeo::BlockSize;
    /// Calculate the height required to render this text with `WritableBuffer::drawBlockText()`.
    /// The given width is the full target rectangle width, including margins configured in `options`.
    /// @param width The available rectangle width in terminal cells.
    /// @param options The text options used for paragraph layout.
    /// @return The required rectangle height in terminal cells.
    [[nodiscard]] auto wrappedBlockTextHeight(
        bgeo::BlockCoordinate width, const BlockTextOptions &options) const noexcept -> bgeo::BlockCoordinate;
    /// Split this string into individual lines.
    /// The string is split at the NL character that is not included in the result.
    /// Empty lines are preserved.
    /// A NL at the end of the string does not generate an additional empty line.
    /// @return A sequence of owning read-only line slices.
    [[nodiscard]] auto splitLines() const noexcept -> std::vector<BlockString>;
    /// Create a new string with the given style applied as a base.
    /// @param style The style used as base for the resulting string.
    /// @return A new string with the base style applied.
    [[nodiscard]] auto withBase(BlockStyle style) const noexcept -> BlockString;

public: // conversion
    /// Create completed terminal text from UTF-8 lines joined with newlines.
    [[nodiscard]] static auto fromLines(
        std::initializer_list<text::String> lines, Color color = {}, BlockAttributes attributes = {}) noexcept
        -> BlockString;
    /// @overload
    [[nodiscard]] static auto fromLines(std::initializer_list<text::String> lines, BlockStyle style) noexcept
        -> BlockString;
    /// Create completed terminal text from UTF-32 lines joined with newlines.
    [[nodiscard]] static auto fromLines(
        std::initializer_list<text::U32String> lines, Color color = {}, BlockAttributes attributes = {}) noexcept
        -> BlockString;
    /// @overload
    [[nodiscard]] static auto fromLines(std::initializer_list<text::U32String> lines, BlockStyle style) noexcept
        -> BlockString;

private:
    friend class BlockStringEditor;

    /// Create a string view over shared storage and range.
    /// @param data The shared string storage.
    /// @param range The visible range.
    BlockString(impl::BlockStringDataPtr data, BlockRange range) noexcept;

    /// Access a character relative to this string's visible range.
    [[nodiscard]] auto characterAt(BlockIndex localIndex) const noexcept -> const Block &;
    /// Get the characters trimmed by the default trim operations.
    [[nodiscard]] static auto defaultTrimCharacters() -> const text::CharSet &;

private:
    impl::BlockStringDataPtr _data; ///< Shared backing storage.
    BlockRange _range;              ///< Visible sub-range inside `_data`.
};

}
