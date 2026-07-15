// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"
#include "U8StringCharView.hpp"
#include "U8StringConstIterator.hpp"
#include "U8StringList_fwd.hpp"
#include "U8StringLiteral_fwd.hpp"
#include "U8StringViewList_fwd.hpp"

#include "impl/U8StringBuilder_fwd.hpp"
#include "impl/U8StringReader_fwd.hpp"
#include "impl/U8StringTransformTools.hpp"
#include "impl/U8StringViewStorage.hpp"

#include "../Char.hpp"
#include "../CharCompareFn.hpp"
#include "../CharSet.hpp"
#include "../EscapeAmount.hpp"
#include "../EscapeFormat.hpp"
#include "../FloatParseOptions.hpp"
#include "../impl/FloatTraits.hpp"
#include "../impl/IntegerConversion.hpp"
#include "../impl/StringConversionTools_fwd.hpp"
#include "../impl/StringReaderBase_fwd.hpp"
#include "../impl/UnsafeU8StringViewAccess_fwd.hpp"
#include "../IntegerParseOptions.hpp"
#include "../Literals.hpp"
#include "../ProcessCharacterFn.hpp"
#include "../SafeStringFlag.hpp"
#include "../StringBuilder.hpp"
#include "../StringCharReader.hpp"
#include "../StringEncoding.hpp"
#include "../StringSide.hpp"
#include "../TransformCharacterFn.hpp"
#include "../TruncateMode.hpp"
#include "../u16/impl/U16StringBuilder_fwd.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u32/U32String_fwd.hpp"

#include "../../bgeo/Alignment.hpp"
#include "../../debug/impl/StringDebugAccess_fwd.hpp"
#include "../../math/IntegerTraits.hpp"
#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteBlockView_fwd.hpp"
#include "../../mem/StorageIdentifier.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/ElementCount.hpp"
#include "../../util/impl/ComparisonHelper.hpp"
#include "../../util/LoopResult.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace erbsland::text {

/// An owning UTF-8 read-only string with copy-on-write semantics for sequential code-point access.
/// Use it to store, read and pass string parameters.
/// Use the `StringView` alias in user code and only `U8StringView` if UTF-8 encoding matters.
/// A `String` and `StringLiteral` are implicitly convertible to a `StringView`, no copy involved.
/// Copy, move, slicing, trimming are fast and copy-free operations.
/// @tested{U8StringViewTest}
class U8StringView final {
    friend class debug::impl::StringDebugAccess;
    friend class U8String;
    friend class U8StringCharView;
    friend class U8StringConstIterator;
    friend class impl::StringReaderBase;
    friend class impl::StringConversionTools;
    friend class impl::U16StringBuilder;
    friend class impl::U8StringBuilder;
    friend class impl::U8StringReader;
    friend class impl::UnsafeU8StringViewAccess;
    template <typename>
    friend class impl::StringList;

public:
    using View = U8StringView; ///< The matching view type for this view.
    using Editable = U8String; ///< The matching editable string type.

public:
    /// Create a view referencing the data from a UTF-8 string.
    U8StringView(const U8String &str) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a view referencing a narrow UTF-8 string literal.
    U8StringView(const U8StringLiteral<char> &str) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a view referencing a UTF-8 string literal.
    U8StringView(const U8StringLiteral<char8_t> &str) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    U8StringView() = default;
    ~U8StringView() = default;
    U8StringView(const U8StringView &) = default;
    U8StringView(U8StringView &&) = default;
    auto operator=(const U8StringView &) -> U8StringView & = default;
    auto operator=(U8StringView &&) -> U8StringView & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const U8StringView &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const U8StringView &other, other);
    /// Access the character at the given start byte position.
    /// Convenience call to `charAt(unit::ByteIndex)`.
    /// @param index The byte index to access the character at.
    /// @return The character at the given index, or a signal character if no character can be read there.
    [[nodiscard]] auto operator[](unit::ByteIndex index) const noexcept -> Char;
    /// Slow: Access the character at the given code-point position.
    /// Convenience call to `charAt(unit::CpIndex)`.
    /// This operation may be slow for large strings, as the position must be found by iterating over the string.
    /// @param index The code-point index to access the character at.
    /// @return The character at the given index, or a signal character if no character can be read there.
    [[nodiscard]] auto operator[](unit::CpIndex index) const noexcept -> Char;

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-8 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;
    /// Create a hash value from the decoded code points.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Create a hash value from the decoded code points after Unicode simple case folding.
    [[nodiscard]] auto toHashCI() const noexcept -> std::size_t;

public: // tests
    /// Test if this string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this string is valid UTF-8.
    [[nodiscard]] auto isValidUtf8() const noexcept -> bool;
    /// Test if this string starts with another one.
    [[nodiscard]] auto startsWith(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this string ends with another one.
    [[nodiscard]] auto endsWith(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this string contains another one.
    [[nodiscard]] auto contains(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Count non-overlapping occurrences of another string.
    /// Empty text counts as zero occurrences.
    [[nodiscard]] auto count(const U8StringView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::ElementCount;
    /// Test if this string contains any character from the given set.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return `true` if at least one decoded character is contained in `characters`.
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// Test if this string only contains characters from the given set.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return `true` all characters in the string are from the given set.
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;

public: // read
    /// Create a copy of this view.
    [[nodiscard]] auto copy() const -> U8String;
    /// Get the byte length of this string.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the character length of this string.
    /// This method provides the number of code points in the string.
    /// Counting follows the tolerant UTF-8 index movement rule documented by `U8String`.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength;
    /// Get the approximate display width of this string.
    /// This is a simple sum of decoded character display widths. Control characters, including line breaks, count as
    /// zero. Complex shaping, grapheme clusters, bidi layout, and terminal-specific behavior are not modeled.
    /// @usesunidb{Uses generated Unicode Character Database character metadata.}
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Get the native data index for one side of the string.
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::ByteIndex;
    /// Get the first or last character in this string.
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// Access the character at the given start byte position.
    /// @seeref{u8-string-view-byte-based-reading}
    /// @param startIndex The byte index to access the character at.
    /// @return The character at the given index, or a null character if no character can be read there.
    [[nodiscard]] auto charAt(unit::ByteIndex startIndex) const noexcept -> Char;
    /// Read the character at the given byte index and advance the index.
    /// @seeref{u8-string-view-indexed-sequential-read}
    /// @param index The byte index to read from. Updated to the position after the read character on success.
    /// @return The character at the given index, or a signal character if no character can be read there.
    [[nodiscard]] auto readCharAndAdvance(unit::ByteIndex &index) const noexcept -> Char;
    /// Read the character before the given byte index and retreat the index.
    /// @seeref{u8-string-view-indexed-sequential-read}
    /// @param index The byte index after the character to read. Updated to the start of the read character on success.
    /// @return The character before the given index, or a signal character if no character can be read there.
    [[nodiscard]] auto readCharAndRetreat(unit::ByteIndex &index) const noexcept -> Char;
    /// Slow: Access the character at the given code-point position.
    /// This operation may be slow for large strings, as the position must be found by iterating over the string.
    /// @seeref{u8-string-view-character-indexed-reading}
    /// @param index The code-point index to access the character at.
    /// @return The character at the given index, or a signal character if no character can be read there.
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;
    /// Advance the given byte index to the start of the next character.
    /// @seeref{u8-string-view-advance-retreat}
    /// @param index The index to advance.
    /// @param count The number of characters to advance.
    /// @return `true` if the index was advanced, `false` if it wasn't advanced.
    auto advance(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// Retreat the given byte index to the start of the previous character.
    /// @seeref{u8-string-view-advance-retreat}
    /// @param index The index to retreat.
    /// @param count The number of characters to retreat.
    /// @return `true` if the index was retreated, `false` if it was already at the start or was "no index".
    auto retreat(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;

public: // byte/char index conversion.
    /// Slow: Get the start byte index of the character at a given char index.
    /// Sequentially iterates over characters until the target char index is reached.
    /// Seeking follows the tolerant UTF-8 index movement rule documented by `U8String`.
    /// @param index The char index to get the byte index for.
    /// @return The start byte index of the character at the given char index.
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::ByteIndex;
    /// Slow: Get the character index from a byte index.
    /// @seeref{u8-string-view-character-indexed-reading}
    /// @param index The byte index to get the character index for.
    /// @return The character index at the given byte index.
    [[nodiscard]] auto toCharIndex(unit::ByteIndex index) const noexcept -> unit::CpIndex;

public: // slice
    /// Return a slice of this string.
    /// Returns a string with a byte-based slice of this string.
    /// No UTF-8 validation is performed, if you slice in the middle of a character, the result contains
    /// encoding errors at the start or end of the resulting string.
    /// @param range The byte range to slice.
    ///     If you pass a zero-length, invalid or out-of-bounds range, an empty string is returned.
    /// @return The sliced string.
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> U8StringView;
    /// Return a character-indexed slice of this string.
    /// Returns a string with a code-point-based slice of this string.
    /// Malformed UTF-8 is decoded according to the tolerant UTF-8 index movement rule documented by `U8String`.
    /// @param range The code-point range to slice.
    ///     If you pass a zero-length, invalid or out-of-bounds range, an empty string is returned.
    /// @return The sliced string.
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U8StringView;
    /// Get the initial or trailing byte-based portion of this string.
    /// @param side The side of the string to slice from.
    /// @param length The number of bytes to slice.
    ///     If you pass a zero-length, an empty string is returned.
    ///     If you pass an infinite-length, the entire string is returned.
    /// @return The sliced string.
    [[nodiscard]] auto slice(StringSide side, unit::ByteLength length) const noexcept -> U8StringView;
    /// Get the byte-indexed portion before or after a split point.
    /// `StringSide::Front` returns the text before the index, `StringSide::Back` returns the text from the index.
    /// `ByteIndex::noIndex()` and indexes at or beyond the end return the full view for front and an empty view for
    /// back.
    /// @param side The side of the split point to keep.
    /// @param index The byte index where the back portion starts.
    /// @return The sliced string.
    [[nodiscard]] auto slice(StringSide side, unit::ByteIndex index) const noexcept -> U8StringView;
    /// Get the initial or trailing code-point-based portion of this string.
    /// @param side The side of the string to slice from.
    /// @param length The number of bytes to slice.
    ///     If you pass a zero-length, an empty string is returned.
    ///     If you pass an infinite-length, the entire string is returned.
    /// @return The sliced string.
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U8StringView;
    /// Get the code-point-indexed portion before or after a split point.
    /// `StringSide::Front` returns the text before the index, `StringSide::Back` returns the text from the index.
    /// `CpIndex::noIndex()` and indexes at or beyond the end return the full view for front and an empty view for back.
    /// @param side The side of the split point to keep.
    /// @param index The code-point index where the back portion starts.
    /// @return The sliced string.
    [[nodiscard]] auto slice(StringSide side, unit::CpIndex index) const noexcept -> U8StringView;
    /// Slice one decoded character from the given side and return it with the remaining string.
    /// @param side The side of the string to slice from.
    /// @return The sliced character and the remaining string.
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U8StringView>;
    /// Split this string view at a byte index.
    /// `ByteIndex::noIndex()` and indexes at or beyond the end return the full view followed by an empty view.
    /// @param index The byte index where the second returned view starts.
    /// @return The two views before and after the split point.
    [[nodiscard]] auto splitAt(unit::ByteIndex index) const noexcept -> std::pair<U8StringView, U8StringView>;
    /// Split this string view at a code-point index.
    /// `CpIndex::noIndex()` and indexes at or beyond the end return the full view followed by an empty view.
    /// @param index The code-point index where the second returned view starts.
    /// @return The two views before and after the split point.
    [[nodiscard]] auto splitAt(unit::CpIndex index) const noexcept -> std::pair<U8StringView, U8StringView>;

public: // trim
    /// Return a view without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U8StringView;

public: // find
    /// Find the first decoded character contained in the given set.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the first decoded character contained in the given set at or after the given byte index.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @param start The byte index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// Find the first decoded character not contained in the given set.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @return The byte index of the first non-matching character, or `ByteIndex::noIndex()` if there is none.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the first decoded character not contained in the given set at or after the given byte index.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @param start The byte index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @return The byte index of the first non-matching character, or `ByteIndex::noIndex()` if there is none.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// Find the last decoded character contained in the given set.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return The byte index of the last match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character contained in the given set before the given byte index.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @param end The exclusive byte index where the reverse search starts.
    ///     If `end` is no-index, this function returns no-index immediately.
    /// @return The byte index of the last match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character not contained in the given set.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @return The byte index of the last non-matching character, or `ByteIndex::noIndex()` if there is none.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character not contained in the given set before the given byte index.
    /// Malformed UTF-8 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @param end The exclusive byte index where the reverse search starts.
    ///     If `end` is no-index, this function returns no-index immediately.
    /// @return The byte index of the last non-matching character, or `ByteIndex::noIndex()` if there is none.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// Find text in this string view.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U8StringView &text, CharCompareFn compareFn = {}) const noexcept -> unit::ByteIndex;
    /// Find text in this string view starting at a byte index.
    /// @param text The text to find.
    /// @param start The byte index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @param compareFn Optional character comparison function.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(
        const U8StringView &text, unit::ByteIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::ByteIndex;

public: // transform and copy-modify
    /// Return a copy with a byte-based range removed.
    [[nodiscard]] auto removed(unit::ByteRange range) const -> U8String;
    /// Return a copy with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U8String;
    /// Return a copy with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U8String;
    /// Return a copy with all occurrences of decoded UTF-8 text removed.
    [[nodiscard]] auto removedAll(const U8StringView &text, CharCompareFn compareFn = {}) const -> U8String;
    /// Return a copy with the first occurrence of decoded UTF-8 text removed.
    [[nodiscard]] auto removedFirst(const U8StringView &text, CharCompareFn compareFn = {}) const -> U8String;
    /// Return a copy keeping only a byte-based range.
    [[nodiscard]] auto kept(unit::ByteRange range) const -> U8String;
    /// Return a copy keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U8String;
    /// Return a copy with text inserted at a byte index.
    [[nodiscard]] auto inserted(unit::ByteIndex index, const U8StringView &text) const -> U8String;
    /// Return a copy with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U8StringView &text) const -> U8String;
    /// Return a copy with a byte-based range replaced by text.
    [[nodiscard]] auto replaced(unit::ByteRange range, const U8StringView &text) const -> U8String;
    /// Return a copy with a character-based range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U8StringView &text) const -> U8String;
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return a string where every decoded code point is mapped through the given function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U8String;
    /// Return a string truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) const -> U8String;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U8StringView &ellipsis) const
        -> U8String;
    /// Return a string padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill = U' ') const -> U8String;
    /// Return a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
        -> U8String;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U8String;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U8StringView &replacement) const -> U8String;
    /// Return a copy with all occurrences of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedAll(
        const U8StringView &text, const U8StringView &replacement, CharCompareFn compareFn = {}) const -> U8String;
    /// Return a copy with the first occurrence of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U8StringView &text, const U8StringView &replacement, CharCompareFn compareFn = {}) const -> U8String;

public: // char view
    /// Create a slow character-index-based view.
    [[nodiscard]] auto toCharView() const noexcept -> U8StringCharView;

public: // conversion
    /// Convert this view to an integer, or return the given default value on error.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto toInteger(
        T defaultValue = {}, IntegerParseOptions options = IntegerParseOptions::stringDefault()) const noexcept -> T;
    /// Convert this view to an integer or throw on parse errors and overflow.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto toIntegerOrThrow(IntegerParseOptions options = IntegerParseOptions::stringDefault()) const -> T;
    /// Convert this view to a floating point value, or return the given default value on error.
    template <impl::AnyFloatType T>
    [[nodiscard]] auto toFloat(
        T defaultValue = {}, FloatParseOptions options = FloatParseOptions::defaultOptions()) const noexcept -> T;
    /// Convert this view to a floating point value or throw on parse errors and overflow.
    template <impl::AnyFloatType T>
    [[nodiscard]] auto toFloatOrThrow(FloatParseOptions options = FloatParseOptions::defaultOptions()) const -> T;
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::ByteLength;
    /// Escape this view according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const -> U8String;

public: // low-level management
    /// Get a unique identifier for the visible storage range.
    /// The identifier changes when the string detaches, reallocates, or when a view selects a different range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;

public: // minimal std-library compatibility
    /// The value returned by this view's const iterator.
    using value_type = Char;
    /// The const iterator type for decoded UTF-8 code points.
    using const_iterator = U8StringConstIterator;
    /// Get an iterator to the first decoded character.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Get an iterator pointing after the last decoded character.
    [[nodiscard]] auto end() const noexcept -> const_iterator;

private:
    /// Test if this view covers the full backing storage range.
    [[nodiscard]] auto isFullStorageRange() const noexcept -> bool;
    /// Create a view for a transformation that did not change decoded text.
    [[nodiscard]] auto viewForUnchangedTransform() const -> U8StringView;
    /// Create a view with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::ByteRange range) const noexcept -> U8StringView;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U8StringDataView;
    /// Create a new view with the given storage.
    explicit U8StringView(impl::U8StringViewStorage storage) : _storage(std::move(storage)) {}

private:
    impl::U8StringViewStorage _storage; ///< Either literal or shared string data.
};

}

template <>
struct std::hash<erbsland::text::U8StringView> {
    [[nodiscard]] auto operator()(const erbsland::text::U8StringView &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

#include "U8StringView_integer.tpp"
