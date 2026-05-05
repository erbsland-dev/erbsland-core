// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"
#include "U32StringConstIterator.hpp"
#include "U32StringList_fwd.hpp"
#include "U32StringLiteral_fwd.hpp"
#include "U32StringViewList_fwd.hpp"

#include "impl/U32StringReader_fwd.hpp"
#include "impl/U32StringTransformTools.hpp"
#include "impl/U32StringViewStorage.hpp"

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
#include "../u8/impl/U8StringBuilder_fwd.hpp"
#include "../u8/U8String_fwd.hpp"

#include "../../bgeo/Alignment.hpp"
#include "../../debug/impl/StringDebugAccess_fwd.hpp"
#include "../../math/IntegerTraits.hpp"
#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteBlockView_fwd.hpp"
#include "../../mem/StorageIdentifier.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/ElementCount.hpp"
#include "../../util/impl/ComparisonHelper.hpp"
#include "../../util/LoopResult.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace erbsland::text {

/// An owning UTF-32 read-only string with copy-on-write semantics for random code-point access.
/// Use it to store, read and pass string parameters.
/// A `U32String` and `U32StringLiteral` are implicitly convertible to a `U32StringView`, no copy involved.
/// Copy, move, slicing, trimming are fast and copy-free operations.
/// Use `StringView` for most use cases and `U32StringView` only if you need random access to code points or require
/// UTF-32 encoding.
/// @tested{U32StringTest}
class U32StringView final {
    friend class debug::impl::StringDebugAccess;
    friend class U32String;
    friend class U32StringConstIterator;
    friend class impl::StringReaderBase;
    friend class impl::StringConversionTools;
    friend class impl::U16StringBuilder;
    friend class impl::U32StringReader;
    friend class impl::U8StringBuilder;
    template <typename>
    friend class impl::StringList;

public:
    using View = U32StringView; ///< The matching view type for this view.
    using Editable = U32String; ///< The matching editable string type.

public:
    /// Create a view referencing the data from a UTF-32 string.
    U32StringView(const U32String &str) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a view referencing a UTF-32 string literal.
    U32StringView(const U32StringLiteral &str) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    U32StringView() = default;
    ~U32StringView() = default;
    U32StringView(const U32StringView &) = default;
    U32StringView(U32StringView &&) = default;
    auto operator=(const U32StringView &) -> U32StringView & = default;
    auto operator=(U32StringView &&) -> U32StringView & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const U32StringView &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const U32StringView &other, other);
    /// Access the character at the given code-point position.
    /// Convenience call to `charAt(unit::CpIndex)`.
    /// @param index The code-point index to access the character at.
    /// @return The character at the given index, or a signal character if no character can be read there.
    [[nodiscard]] auto operator[](unit::CpIndex index) const noexcept -> Char;

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-32 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U32StringView &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;
    /// Create a hash value from the decoded code points.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Create a hash value from the decoded code points after Unicode simple case folding.
    [[nodiscard]] auto toHashCI() const noexcept -> std::size_t;

public: // tests
    /// Test if this string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this string is valid UTF-32.
    [[nodiscard]] auto isValidUtf32() const noexcept -> bool;
    /// Test if this string starts with another one.
    [[nodiscard]] auto startsWith(const U32StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this string ends with another one.
    [[nodiscard]] auto endsWith(const U32StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this string contains another one.
    [[nodiscard]] auto contains(const U32StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Count non-overlapping occurrences of another string.
    /// Empty text counts as zero occurrences.
    [[nodiscard]] auto count(const U32StringView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::ElementCount;
    /// Test if this string contains any character from the given set.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return `true` if at least one decoded character is contained in `characters`.
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// Test if this string only contains characters from the given set.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return `true` all characters in the string are from the given set.
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;

public: // read
    /// Create a copy of this view.
    [[nodiscard]] auto copy() const -> U32String;
    /// Get the UTF-32 code-unit length of this string.
    [[nodiscard]] auto length() const noexcept -> unit::CpLength;
    /// Get the native data index for one side of the string.
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::CpIndex;
    /// Get the first or last character in this string.
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// Access the character at the given start code-unit position.
    /// @seeref{u32-string-view-code-unit-based-reading}
    /// @param startIndex The UTF-32 data index to access the character at.
    /// @return The character at the given code-unit position, or a null character if no character can be read there.
    [[nodiscard]] auto charAt(unit::CpIndex startIndex) const noexcept -> Char;
    /// Advance the given UTF-32 data index to the start of the next character.
    /// @seeref{u32-string-view-advance-retreat}
    /// @param index The index to advance.
    /// @param count The number of characters to advance.
    /// @return `true` if the index was advanced, `false` if it wasn't advanced.
    auto advance(unit::CpIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// Retreat the given UTF-32 data index to the start of the previous character.
    /// @seeref{u32-string-view-advance-retreat}
    /// @param index The index to retreat.
    /// @param count The number of characters to retreat.
    /// @return `true` if the index was retreated, `false` if it was already at the start or was "no index".
    auto retreat(unit::CpIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;

public: // data/char index conversion.
    /// Slow: Get the start UTF-32 data index of the character at a given code-point index.
    /// Sequentially iterates over characters until the target char index is reached.
    /// Seeking follows the tolerant UTF-32 index movement rule documented by `U32String`.
    /// @param index The code-point index to get the UTF-32 data index for.
    /// @return The start UTF-32 data index of the character at the given code-point index.
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::CpIndex;
    /// Slow: Get the code-point index from a UTF-32 data index.
    /// @seeref{u32-string-view-character-indexed-reading}
    /// @param index The UTF-32 data index to get the code-point index for.
    /// @return The code-point index at the given UTF-32 data index.
    [[nodiscard]] auto toCharIndex(unit::CpIndex index) const noexcept -> unit::CpIndex;

public: // slice
    /// Return a slice of this string.
    /// Returns a string with a UTF-32 code-unit-based slice of this string.
    /// No UTF-32 validation is performed, if you slice in the middle of a character, the result contains
    /// encoding errors at the start or end of the resulting string.
    /// @param range The UTF-32 data range to slice.
    /// @return The sliced string.
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U32StringView;
    /// Get the initial or trailing UTF-32 data portion of this string.
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U32StringView;
    /// Slice one decoded character from the given side and return it with the remaining string.
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U32StringView>;

public: // trim
    /// Return a view without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U32StringView;

public: // find
    /// Find the first decoded character contained in the given set.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return The UTF-32 data index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the first decoded character contained in the given set at or after the given UTF-32 data index.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @param start The UTF-32 data index where the search starts.
    /// @return The UTF-32 data index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Find the first decoded character not contained in the given set.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @return The UTF-32 data index of the first non-matching character, or `CpIndex::noIndex()` if there is
    /// none.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the first decoded character not contained in the given set at or after the given UTF-32 data index.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @param start The UTF-32 data index where the search starts.
    /// @return The UTF-32 data index of the first non-matching character, or `CpIndex::noIndex()` if there is
    /// none.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Find the last decoded character contained in the given set.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return The UTF-32 data index of the last match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the last decoded character contained in the given set before the given UTF-32 data index.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @param end The exclusive UTF-32 data index where the reverse search starts.
    /// @return The UTF-32 data index of the last match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Find the last decoded character not contained in the given set.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @return The UTF-32 data index of the last non-matching character, or `CpIndex::noIndex()` if there is none.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the last decoded character not contained in the given set before the given UTF-32 data index.
    /// Malformed UTF-32 is decoded as `Char::replacement()`.
    /// @param characters The character set to exclude.
    /// @param end The exclusive UTF-32 data index where the reverse search starts.
    /// @return The UTF-32 data index of the last non-matching character, or `CpIndex::noIndex()` if there is none.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Find text in this string view.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The code point index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U32StringView &text, CharCompareFn compareFn = {}) const noexcept -> unit::CpIndex;
    /// Find text in this string view starting at a code point index.
    /// @param text The text to find.
    /// @param start The code point index where the search starts.
    /// @param compareFn Optional character comparison function.
    /// @return The code point index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U32StringView &text, unit::CpIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::CpIndex;

public: // transform and copy-modify
    /// Return a copy with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U32String;
    /// Return a copy with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U32String;
    /// Return a copy with all occurrences of decoded UTF-32 text removed.
    [[nodiscard]] auto removedAll(const U32StringView &text, CharCompareFn compareFn = {}) const -> U32String;
    /// Return a copy with the first occurrence of decoded UTF-32 text removed.
    [[nodiscard]] auto removedFirst(const U32StringView &text, CharCompareFn compareFn = {}) const -> U32String;
    /// Return a copy keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U32String;
    /// Return a copy with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U32StringView &text) const -> U32String;
    /// Return a copy with a character-based range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U32StringView &text) const -> U32String;
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return a string where every decoded code point is mapped through the given function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U32String;
    /// Return a string truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) const -> U32String;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U32StringView &ellipsis) const
        -> U32String;
    /// Return a string padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill = U' ') const -> U32String;
    /// Return a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
        -> U32String;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U32String;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U32StringView &replacement) const -> U32String;
    /// Return a copy with all occurrences of decoded UTF-32 text replaced.
    [[nodiscard]] auto replacedAll(
        const U32StringView &text, const U32StringView &replacement, CharCompareFn compareFn = {}) const -> U32String;
    /// Return a copy with the first occurrence of decoded UTF-32 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U32StringView &text, const U32StringView &replacement, CharCompareFn compareFn = {}) const -> U32String;

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
    /// @tested{StringEscapingTest}
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::CpLength;
    /// Escape this view according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    /// @tested{StringEscapingTest}
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const -> U32String;

public: // low-level management
    /// Get a unique identifier for the visible storage range.
    /// The identifier changes when the string detaches, reallocates, or when a view selects a different range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;

public: // minimal std-library compatibility
    /// The value returned by this view's const iterator.
    using value_type = Char;
    /// The const iterator type for decoded UTF-32 code points.
    using const_iterator = U32StringConstIterator;
    /// Get an iterator to the first decoded character.
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// Get an iterator pointing after the last decoded character.
    [[nodiscard]] auto end() const noexcept -> const_iterator;

private:
    /// Test if this view covers the full backing storage range.
    [[nodiscard]] auto isFullStorageRange() const noexcept -> bool;
    /// Create a view for a transformation that did not change decoded text.
    [[nodiscard]] auto viewForUnchangedTransform() const -> U32StringView;
    /// Create a view with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::CpRange range) const noexcept -> U32StringView;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U32StringDataView;
    /// Create a new view with the given storage.
    explicit U32StringView(impl::U32StringViewStorage storage) : _storage(std::move(storage)) {}

private:
    impl::U32StringViewStorage _storage; ///< Either literal or shared string data.
};

}

template <>
struct std::hash<erbsland::text::U32StringView> {
    [[nodiscard]] auto operator()(const erbsland::text::U32StringView &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

#include "U32StringList.hpp"
#include "U32StringView_integer.tpp"
#include "U32StringViewList.hpp"
