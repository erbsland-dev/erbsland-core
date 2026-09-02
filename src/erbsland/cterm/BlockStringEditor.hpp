// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BlockCount.hpp"
#include "BlockIndex.hpp"
#include "BlockPrintContext.hpp"
#include "BlockRange.hpp"
#include "BlockString_fwd.hpp"
#include "BlockStringEditor_fwd.hpp"
#include "BlockTextOptions_fwd.hpp"
#include "ParagraphSpacing.hpp"
#include "TypeTraits.hpp"

#include "impl/BlockPrintContextToBlockString_fwd.hpp"
#include "impl/BlockStringBuilder_fwd.hpp"
#include "impl/BlockStringData.hpp"

#include "../block/Size.hpp"
#include "../text/CharSet.hpp"
#include "../text/String.hpp"
#include "../text/StringSide.hpp"
#include "../text/u32/U32String.hpp"

#include <initializer_list>
#include <vector>

namespace erbsland::cterm {

/// A terminal string represented as a sequence of `Block` values.
///
/// `BlockStringEditor` is implicitly shared. Copying a string shares its backing data, and a deep copy is only made
/// when one instance is modified through a mutating API.
class BlockStringEditor {
public:
    using Storage = std::vector<Block>;                             ///< Storage container for the characters.
    using iterator = Storage::iterator;                             ///< Mutable forward iterator.
    using const_iterator = Storage::const_iterator;                 ///< Immutable forward iterator.
    using reverse_iterator = Storage::reverse_iterator;             ///< Mutable reverse iterator.
    using const_reverse_iterator = Storage::const_reverse_iterator; ///< Immutable reverse iterator.
    using difference_type = Storage::difference_type;               ///< Signed distance type.
    using value_type = Storage::value_type;                         ///< Stored value type.
    using reference = Storage::reference;                           ///< Mutable element reference.
    using const_reference = Storage::const_reference;               ///< Immutable element reference.
    using pointer = Storage::pointer;                               ///< Mutable element pointer.
    using const_pointer = Storage::const_pointer;                   ///< Immutable element pointer.

public:
    /// Create an empty terminal string.
    BlockStringEditor() noexcept;
    /// Create a terminal string from UTF-8 text.
    /// @param str The UTF-8 text to split into terminal characters.
    /// Control codes are ignored except for tab and newline.
    /// Malformed UTF-8 is replaced with Unicode replacement characters.
    explicit BlockStringEditor(const text::String &str);
    /// Create a terminal string from UTF-8 text with a uniform style.
    /// @param str The UTF-8 text to split into terminal characters.
    /// @param style The style to use for the characters.
    /// Control codes are ignored except for tab and newline.
    /// Malformed UTF-8 is replaced with Unicode replacement characters.
    explicit BlockStringEditor(const text::String &str, BlockStyle style);
    /// Create a terminal string from UTF-32 text.
    /// @param str The UTF-32 text to split into terminal characters.
    /// Control codes are ignored except for tab and newline.
    explicit BlockStringEditor(const text::U32String &str);
    /// Create a terminal string from UTF-32 text with a uniform style.
    /// @param str The UTF-32 text to split into terminal characters.
    /// @param style The style to use for the characters.
    /// Control codes are ignored except for tab and newline.
    explicit BlockStringEditor(const text::U32String &str, BlockStyle style);
    /// Create a terminal string repeating the same `Block`.
    /// @param count The repetition count. Limited to 10'000'000.
    /// @param character The character to repeat.
    explicit BlockStringEditor(BlockCount count, Block character) noexcept;
    /// Create an editable terminal string from a read-only string.
    /// @param view The read-only string to copy into an owned string.
    explicit BlockStringEditor(const BlockString &view);

    // defaults
    ~BlockStringEditor() = default;
    BlockStringEditor(const BlockStringEditor &) = default;
    BlockStringEditor(BlockStringEditor &&other) noexcept;

    // defaults
    auto operator=(const BlockStringEditor &) -> BlockStringEditor & = default;
    /// Move another terminal string into this string.
    auto operator=(BlockStringEditor &&other) noexcept -> BlockStringEditor &;

public: // operators
    /// Compare two strings.
    /// Two strings are only equal, if all characters and styles are equal.
    auto operator==(const BlockStringEditor &other) const noexcept -> bool;
    /// Compare two strings for inequality.
    /// @param other The string to compare.
    /// @return `true` if the strings differ in characters or styles.
    auto operator!=(const BlockStringEditor &other) const noexcept -> bool { return !operator==(other); }
    /// Access one character without bounds checking.
    /// @param index The character index.
    /// @return A copy of the character at `index`, or `Block{}` if `index` is out of bounds.
    [[nodiscard]] auto operator[](BlockIndex index) const noexcept -> Block;
    /// Access one character without bounds checking.
    /// @param index The character index.
    /// @return A mutable reference to the character at `index`, or a discarded `Block{}` if `index` is out of bounds.
    [[nodiscard]] auto operator[](BlockIndex index) noexcept -> Block &;
    /// Append one character to this string.
    /// @param character The character to append.
    /// @return Reference to this string.
    auto operator+=(const Block &character) noexcept -> BlockStringEditor & {
        append(character);
        return *this;
    }
    /// Append another terminal string to this string.
    /// @param other The string to append.
    /// @return Reference to this string.
    auto operator+=(const BlockStringEditor &other) noexcept -> BlockStringEditor &;
    /// Append a read-only string to this editor.
    /// @param other The read-only string to append.
    /// @return Reference to this string.
    auto operator+=(const BlockString &other) noexcept -> BlockStringEditor &;
    /// Create a new string with one appended character.
    /// @param character The character to append.
    /// @return The concatenated string.
    auto operator+(const Block &character) const noexcept -> BlockStringEditor {
        auto copy = *this;
        copy.append(character);
        return copy;
    }
    /// Create a new string with another appended string.
    /// @param other The string to append.
    /// @return The concatenated string.
    auto operator+(const BlockStringEditor &other) const noexcept -> BlockStringEditor;
    /// Create a new editor with an appended read-only string.
    /// @param other The read-only string to append.
    /// @return The concatenated string.
    auto operator+(const BlockString &other) const noexcept -> BlockStringEditor;

public: // accessors
    /// Get the number of stored characters.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return _range.length(); }
    /// Get the width of the string in terminal cells.
    /// @return The sum of all character display widths.
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Return a copy with tab characters expanded to styled spaces up to the given column.
    [[nodiscard]] auto withTabsExpanded(int targetColumn) const -> BlockStringEditor;
    /// Test if this string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _range.isEmpty(); }
    /// Access one character with bounds checking.
    /// @param index The character index.
    /// @return A copy of the character at `index`.
    [[nodiscard]] auto at(BlockIndex index) const -> Block;
    /// Get an iterator to the first character.
    [[nodiscard]] auto begin() noexcept -> iterator;
    /// Get an iterator past the last character.
    [[nodiscard]] auto end() noexcept -> iterator;
    /// Get a const iterator to the first character.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Get a const iterator past the last character.
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Get a const iterator to the first character.
    [[nodiscard]] auto cbegin() const noexcept -> const_iterator;
    /// Get a const iterator past the last character.
    [[nodiscard]] auto cend() const noexcept -> const_iterator;
    /// Get a reverse iterator to the last character.
    [[nodiscard]] auto rbegin() noexcept -> reverse_iterator;
    /// Get a reverse iterator past the first character.
    [[nodiscard]] auto rend() noexcept -> reverse_iterator;
    /// Get a const reverse iterator to the last character.
    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator;
    /// Get a const reverse iterator past the first character.
    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator;
    /// Get a const reverse iterator to the last character.
    [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator;
    /// Get a const reverse iterator past the first character.
    [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator;
    /// Count the number of characters with a given color.
    /// @param character The character to count.
    /// @return The number of characters in this string.
    [[nodiscard]] auto count(const Block &character) const noexcept -> BlockCount;
    /// Count the number of characters matching any color.
    /// @param character The character to count (1 code-point).
    /// @return The number of characters in this string.
    [[nodiscard]] auto count(text::Char character) const noexcept -> BlockCount;
    /// Get the index of the next character with a given color.
    /// If startIndex is out of bounds, returns `BlockIndex::noIndex()`.
    /// If no character is found, returns `BlockIndex::noIndex()`.
    /// @param character The character to search for. Compares both, character and color!
    /// @param startIndex The start index to search from. Defaults to 0.
    /// @return The index of the next character or `BlockIndex::noIndex()` if not found.
    [[nodiscard]] auto indexOf(const Block &character, BlockIndex startIndex = {}) const noexcept -> BlockIndex;
    /// Get the index of the next matching character.
    /// Ignores the character style.
    /// If startIndex is out of bounds, returns `BlockIndex::noIndex()`.
    /// If no character is found, returns `BlockIndex::noIndex()`.
    /// @param character The character to search for. Only compares single-code-point characters.
    /// @param startIndex The start index to search from. Defaults to 0.
    /// @return The index of the next character or `BlockIndex::noIndex()` if not found.
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
    /// Get a substring.
    /// @param range The block range to slice.
    /// @return The substring or an empty string if the range is out of bounds.
    [[nodiscard]] auto slice(BlockRange range = BlockRange::all()) const noexcept -> BlockStringEditor;
    /// Get the initial or trailing block-based portion of this string.
    [[nodiscard]] auto slice(text::StringSide side, BlockCount count) const noexcept -> BlockStringEditor;
    /// Get a substring that fits into the given display width.
    /// If a double-sized character is at the edge, it isn't included in the result.
    /// Therefore, the resulting string may be shorter than the display width.
    /// @param displayWidth The maximum width of the substring in display units.
    /// @param alignment The alignment of the cropped text. Only `geometry::Alignment::Left` and
    /// `geometry::Alignment::Right` are supported.
    /// @return The cropped substring or an empty string if displayWidth is <=0.
    [[nodiscard]] auto croppedToDisplayWidth(
        block::Coordinate displayWidth, geometry::Alignment alignment) const noexcept -> BlockStringEditor;
    /// Return a string with the given characters trimmed from the beginning and end.
    /// Only single-code-point characters are matched.
    /// @param characters The characters to remove from both ends. If empty, removes space, tab, and newline characters.
    /// @return A copy without matching leading and trailing characters.
    [[nodiscard]] auto trimmed(const text::CharSet &characters = defaultTrimCharacters()) const noexcept
        -> BlockStringEditor;
    /// Return a normalized string.
    /// Trim the given characters from start/end, and eplace any number of the given characters inside the
    /// string with a single separator.
    /// Only single-code-point characters are matched.
    /// The style of replaced characters is determined by using the first matches character style
    /// as a base for `separator`.
    /// @param characters The characters to match. If empty: space, tab, and newline characters.
    /// @param separator The separator to use.
    [[nodiscard]] auto normalized(
        const text::CharSet &characters = defaultTrimCharacters(), Block separator = Block::space()) const noexcept
        -> BlockStringEditor;

public: // tests
    /// Test if this string contains control characters.
    /// Most control codes are filtered on construction, therefore, this test searched for NL and TAB.
    [[nodiscard]] auto containsControlCharacters() const noexcept -> bool;

public: // modifiers
    /// Reserve storage for at least the given number of characters.
    /// @param size The requested capacity.
    void reserve(BlockCount size) noexcept;
    /// Remove all characters from this string.
    void clear() noexcept;
    /// Trim the given characters from the beginning and end of the string.
    /// Only single-code-point characters are matched.
    /// @param characters The characters to remove from both ends. If empty, removes space, tab, and newline characters.
    void trim(const text::CharSet &characters = defaultTrimCharacters()) noexcept;
    /// Normalize this string.
    /// Trim the given characters from start/end, and eplace any number of the given characters inside the
    /// string with a single separator.
    /// Only single-code-point characters are matched.
    /// The style of replaced characters is determined by using the first matches character style
    /// as a base for `separator`.
    /// @param characters The characters to match. If empty: space, tab, and newline characters.
    /// @param separator The separator to use.
    void normalize(
        const text::CharSet &characters = defaultTrimCharacters(), Block separator = Block::space()) noexcept;
    /// Replace characters in this string.
    /// If the range is out of bounds, it is clamped to the string's length.
    /// If the range is outside the string or empty, no replacement is performed.
    /// If the operation does not change the string, no replacement is performed.
    /// @param range The range of characters to replace.
    /// @param replacement The replacement for this range.
    void replace(BlockRange range, Block replacement) noexcept;
    /// @overload
    void replace(BlockRange range, const BlockString &replacement) noexcept;
    /// Remove a part of the string.
    /// If the range is out of bounds, it is clamped to the string's length.
    /// If the range is outside the string or empty, no replacement is performed.
    void remove(BlockRange range) noexcept;
    /// Set a character in this string.
    /// This call is more efficient than using the index operator.
    void set(BlockIndex index, Block character) noexcept;
    /// Append another terminal string with a base style.
    /// If `pos` is out of range, appends the string at the end.
    /// @param pos The position to insert the string at.
    /// @param other The source text view.
    /// @param style The style used as base for inherited components in the appended range.
    void insertWithBaseStyle(BlockIndex pos, const BlockString &other, BlockStyle style) noexcept;
    /// Append text using one uniform style.
    /// @param text The text to append.
    /// @param style The style applied to the appended characters.
    void appendStyled(const text::String &text, BlockStyle style);
    /// @overload
    void appendStyled(const text::U32String &text, BlockStyle style) noexcept;
    /// Append another terminal string with a base style.
    /// @param other The source text view.
    /// @param style The style used as base for inherited components in the appended range.
    void appendStyled(const BlockString &other, BlockStyle style) noexcept;
    /// Append a repeated character.
    /// @param count The repetition count. Limited to 10'000'000.
    /// @param character The character to repeat.
    void append(BlockCount count, Block character) noexcept;
    /// Append a repeated Unicode code point with a uniform style.
    /// @param count The repetition count. Limited to 10'000'000.
    /// @param character The character to repeat.
    /// @param style The style for every appended character.
    void append(BlockCount count, text::Char character, BlockStyle style) noexcept;
    /// Append elements to this string.
    /// This works similar to `Terminal::print()`.
    /// If you add a color, this color is "active" for all following characters *in the same call*.
    /// If you add character attributes, these attributes are active for all following characters *in the same call*.
    /// Appended `Block`, `BlockStringEditor`, and `BlockString` values with inherited color components or inherited
    /// attributes resolve against the currently active state. Just adding a color does not change the string.
    /// @param args The arguments to append.
    template <PrintableArg... Args>
    void append(Args... args) noexcept {
        const auto context = createPrintContext();
        (context->print(args), ...);
        context->commit();
    }

public: // tools
    /// Split the string into words at space, tab, carriage return, or newline characters.
    /// @return A sequence of words.
    [[nodiscard]] auto splitWords() const noexcept -> std::vector<BlockStringEditor>;
    /// Wrap this string into lines that have a maximum display width.
    /// Paragraph breaks from newline characters are preserved using the selected paragraph spacing.
    /// @param width The maximum terminal width in cells. Must be greater than zero.
    /// @param paragraphSpacing The spacing to use between newline-separated paragraphs.
    /// @return A sequence of lines.
    [[nodiscard]] auto wrapIntoLines(
        int width, ParagraphSpacing paragraphSpacing = ParagraphSpacing::SingleLine) const noexcept
        -> std::vector<BlockStringEditor>;
    /// Count how many terminal lines this string occupies for a given terminal width.
    /// Newline characters start a new terminal line and printable characters wrap at the given width.
    /// The result is undefined for abstract terminal widths smaller than 10 cells.
    /// @param width The available terminal width in cells. Must be greater than zero.
    /// @return The number of occupied terminal lines.
    [[nodiscard]] auto terminalLines(int width) const noexcept -> int;
    /// Get the natural rectangular size for this text without wrapping.
    /// The returned size is at least 1x1, preserves explicit non-trailing newline characters as separate lines,
    /// and uses terminal cell width for wide and combining characters.
    /// @return The natural text size in terminal cells.
    [[nodiscard]] auto naturalBlockTextSize() const noexcept -> block::Size;
    /// Calculate the height required to render this text with `WritableBuffer::drawBlockText()`.
    /// The given width is the full target rectangle width, including margins configured in `options`.
    /// @param width The available rectangle width in terminal cells.
    /// @param options The text options used for paragraph layout.
    /// @return The required rectangle height in terminal cells.
    [[nodiscard]] auto wrappedBlockTextHeight(block::Coordinate width, const BlockTextOptions &options) const noexcept
        -> block::Coordinate;
    /// Splits this string into individual lines.
    /// The string is split at the NL character that is not included in the result.
    /// Empty lines are preserved.
    /// A NL at the end of the string does not generate an additional empty line.
    /// @return A sequence of lines.
    [[nodiscard]] auto splitLines() const noexcept -> std::vector<BlockStringEditor>;
    /// Create a new string with the given style applied as a base.
    /// @param style The style used as base for the resulting string.
    /// @return A new string with the base style applied.
    [[nodiscard]] auto withBase(BlockStyle style) const noexcept -> BlockStringEditor;

public: // conversion
    /// Create a new string from a list of lines.
    /// All lines are joined using a new-line character.
    /// @param lines The lines for the string.
    /// @param color The base color to use for each character.
    /// @param attributes The base attributes to use for each character.
    /// Invalid UTF-8 bytes are replaced with the Unicode replacement character.
    /// @return The new string.
    [[nodiscard]] static auto fromLines(
        std::initializer_list<text::String> lines, Color color = {}, BlockAttributes attributes = {}) noexcept
        -> BlockStringEditor;
    /// @overload
    [[nodiscard]] static auto fromLines(std::initializer_list<text::String> lines, BlockStyle style) noexcept
        -> BlockStringEditor;
    /// @overload
    [[nodiscard]] static auto fromLines(
        std::initializer_list<text::U32String> lines, Color color = {}, BlockAttributes attributes = {}) noexcept
        -> BlockStringEditor;
    /// @overload
    [[nodiscard]] static auto fromLines(std::initializer_list<text::U32String> lines, BlockStyle style) noexcept
        -> BlockStringEditor;

private:
    friend class BlockString;
    friend class impl::BlockPrintContextToBlockString;
    friend class impl::BlockStringBuilder;

    /// Create an editor over a shared string range.
    /// @param data The shared string storage.
    /// @param range The visible range in the storage.
    explicit BlockStringEditor(impl::BlockStringDataPtr data, BlockRange range) noexcept;
    /// Create an editor owning character storage.
    /// @param chars The character storage.
    explicit BlockStringEditor(Storage chars) noexcept;
    /// Create a string from storage with a known display width.
    /// @param chars The owned character storage.
    /// @param displayWidth The precomputed display width.
    /// @return The string over the supplied storage.
    [[nodiscard]] static auto fromStorageWithDisplayWidth(Storage chars, int displayWidth) noexcept
        -> BlockStringEditor;

    /// Split UTF-8 text into terminal character blocks.
    /// @param str The UTF-8 text to split.
    /// @param color The base color for split characters.
    /// @param attributes The base attributes for split characters.
    /// @return The resulting character storage.
    [[nodiscard]] static auto splitCharacters(
        const text::String &str, Color color = {}, BlockAttributes attributes = {}) -> Storage;
    /// Split UTF-32 text into terminal character blocks.
    /// @param str The UTF-32 text to split.
    /// @param color The base color for split characters.
    /// @param attributes The base attributes for split characters.
    /// @return The resulting character storage.
    [[nodiscard]] static auto splitCharacters(
        const text::U32String &str, Color color = {}, BlockAttributes attributes = {}) -> Storage;
    /// Create the print context used by variadic append operations.
    /// @return The initialized print context.
    [[nodiscard]] auto createPrintContext() noexcept -> BlockPrintContextPtr;
    /// Append a read-only string while resolving inherited style components.
    /// @param view The string to append.
    /// @param style The style to resolve against.
    void appendString(const BlockString &view, BlockStyle style) noexcept;
    /// Append a read-only string while applying a base style.
    /// @param view The string to append.
    /// @param style The base style to apply.
    void appendStringWithBaseStyle(const BlockString &view, BlockStyle style) noexcept;
    /// Ensure this editor has unique mutable backing storage.
    void detach();
    /// Synchronize the visible range with the backing storage length.
    void syncRangeWithStorage() noexcept;
    /// Access a character, returning a discarded block when out of bounds.
    /// @param index The character index.
    /// @return The character reference or the discarded block.
    [[nodiscard]] auto characterAt(BlockIndex index) const noexcept -> const Block &;
    /// Return the mutable discarded block for invalid mutable access.
    /// @return The discarded block.
    [[nodiscard]] static auto ignoredMutableCharacter() noexcept -> Block &;
    /// Get the default set of characters removed by trim operations.
    /// @return The default trim character set.
    [[nodiscard]] static auto defaultTrimCharacters() -> const text::CharSet &;

private:
    impl::BlockStringDataPtr _data; ///< Shared backing storage.
    BlockRange _range;              ///< Visible sub-range inside `_data`.
};

/// A sequence of wrapped terminal text lines.
using BlockStringEditorLines = std::vector<BlockStringEditor>;

}
