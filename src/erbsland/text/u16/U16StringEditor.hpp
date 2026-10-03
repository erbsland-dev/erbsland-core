// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String_fwd.hpp"
#include "U16StringConstIterator_fwd.hpp"
#include "U16StringEditor_fwd.hpp"
#include "U16StringEditorList_fwd.hpp"
#include "U16StringList_fwd.hpp"
#include "U16StringLiteral_fwd.hpp"

#include "impl/U16StringBuilder_fwd.hpp"
#include "impl/U16StringEncodingTools_fwd.hpp"
#include "impl/U16StringSharedStorage.hpp"

#include "../AnyStringBuilder.hpp"
#include "../AsciiCategory.hpp"
#include "../BooleanFormat.hpp"
#include "../Char.hpp"
#include "../CharCompareFn.hpp"
#include "../CharSet.hpp"
#include "../EscapeAmount.hpp"
#include "../EscapeFormat.hpp"
#include "../FloatFormat.hpp"
#include "../FloatParseOptions.hpp"
#include "../impl/FloatTraits.hpp"
#include "../impl/IntegerConversion.hpp"
#include "../impl/StringConversionTools_fwd.hpp"
#include "../impl/StringNormalizationTools_fwd.hpp"
#include "../IntegerFormat.hpp"
#include "../IntegerParseOptions.hpp"
#include "../Literals.hpp"
#include "../NormalizationForm.hpp"
#include "../ProcessCharacterFn.hpp"
#include "../SafeStringFlag.hpp"
#include "../StringBomMode.hpp"
#include "../StringEncoding.hpp"
#include "../StringSide.hpp"
#include "../TransformCharacterFn.hpp"
#include "../TruncateMode.hpp"
#include "../u32/U32StringEditor_fwd.hpp"
#include "../u8/U8StringEditor_fwd.hpp"

#include "../../debug/impl/StringDebugAccess_fwd.hpp"
#include "../../geometry/Alignment.hpp"
#include "../../math/IntegerTraits.hpp"
#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/StorageIdentifier.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/CpCount.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/U16Count.hpp"
#include "../../unit/U16Index.hpp"
#include "../../unit/U16Range.hpp"
#include "../../util/impl/ComparisonHelper.hpp"
#include "../../util/LoopResult.hpp"

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace erbsland::text {

// clang-format off
/// An owning UTF-16 string editor with copy-on-write semantics for sequential code-point access.
/// Use it as a local mutable working value for UTF-16 construction and multi-step editing.
/// Use `U16String` for storage, read-only access and copy-based transformations.
/// Always creates a copy of the data when constructed from a read-only string.
/// Use `U16StringEditor` only when the mutable workflow requires UTF-16 encoding.
/// @seedoc{/reference/text/strings}
/// @tested{U16StringTest StringEscapingTest BooleanConversionTest UnicodeNormalizationTest StringReplacementTest StringBinaryComparisonTest StringSeparatorSplitTest StringEncodingValidationTest}
class U16StringEditor {
    // clang-format on
    friend class debug::impl::StringDebugAccess<U16StringEditor>;
    friend class U16String;
    friend class impl::U16StringBuilder;
    friend class impl::U16StringEncodingTools;
    friend class impl::StringConversionTools;
    template <typename>
    friend class impl::StringNormalizationTools;
    friend class impl::UnsafeU16StringEditorAccess;
    friend class impl::UnsafeU16StringBuffer;

public:
    /// Create a copy of the given UTF-16 string.
    /// @param stdString The string to copy.
    explicit U16StringEditor(std::u16string_view stdString);
    /// Create a copy of the given string literal.
    /// @param literal The string literal to copy.
    explicit U16StringEditor(const U16StringLiteral &literal);
    /// Create a copy of the given read-only string.
    /// The copied data is not shared with the original string.
    /// @param view The read-only string to copy.
    explicit U16StringEditor(const U16String &view);

    // defaults
    U16StringEditor() = default;
    ~U16StringEditor() = default;
    U16StringEditor(const U16StringEditor &) = default;
    U16StringEditor(U16StringEditor &&) = default;
    auto operator=(const U16StringEditor &) -> U16StringEditor & = default;
    auto operator=(U16StringEditor &&) -> U16StringEditor & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const U16String &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const U16String &other, other);
    /// @copydoc erbsland::text::U16String::operator[](unit::U16Index) const
    [[nodiscard]] auto operator[](unit::U16Index index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::operator[](unit::CpIndex) const
    [[nodiscard]] auto operator[](unit::CpIndex index) const noexcept -> Char;

public: // comparison
    /// Compare decoded code points, replacing malformed UTF-16 with `Char::replacement()`.
    /// No normalization or locale collation is performed.
    /// @param other The string to compare.
    /// @param compareFn Optional character comparator; numeric code-point order is the default.
    /// @return Lexicographic decoded-character order, with a shorter equal prefix first.
    /// @seedoc{/topics/text_strings/comparing_strings}
    [[nodiscard]] auto compare(const U16String &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;
    /// Compare the visible unsigned storage bytes without decoding or allocation.
    /// Native byte order is used; binary equality can differ from decoded equality.
    /// @param other The same-width string to compare.
    /// @return Lexicographic byte order, with a shorter equal byte prefix first.
    /// @seedoc{/topics/text_strings/comparing_strings}
    [[nodiscard]] auto compareBinary(const U16String &other) const noexcept -> std::strong_ordering;
    /// Create a hash value from the decoded code points.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Create a hash value from the decoded code points after Unicode simple case folding.
    [[nodiscard]] auto toHashCI() const noexcept -> std::size_t;

public: // tests
    /// @copydoc erbsland::text::U16String::isEmpty() const
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::isEncodingValid() const
    [[nodiscard]] auto isEncodingValid() const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::encodingValidOrThrow() const
    void encodingValidOrThrow() const;
    /// @copydoc erbsland::text::U16String::startsWith(const U16String &, CharCompareFn) const
    [[nodiscard]] auto startsWith(const U16String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::endsWith(const U16String &, CharCompareFn) const
    [[nodiscard]] auto endsWith(const U16String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::contains(const U16String &, CharCompareFn) const
    [[nodiscard]] auto contains(const U16String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::count(const U16String &, CharCompareFn) const
    [[nodiscard]] auto count(const U16String &text, CharCompareFn compareFn = {}) const noexcept -> unit::ItemCount;
    /// @copydoc erbsland::text::U16String::containsOneOf(const CharSet &) const
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::containsOnly(const CharSet &) const
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::containsOnly(AsciiCategory) const
    [[nodiscard]] auto containsOnly(AsciiCategory category) const noexcept -> bool;

public: // read
    /// @copydoc erbsland::text::U16String::length() const
    [[nodiscard]] auto length() const noexcept -> unit::U16Count;
    /// @copydoc erbsland::text::U16String::characterLength() const
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpCount;
    /// @copydoc erbsland::text::U16String::displayWidth() const
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// @copydoc erbsland::text::U16String::indexAt(StringSide) const
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::charAt(StringSide) const
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::charAt(unit::U16Index) const
    [[nodiscard]] auto charAt(unit::U16Index startIndex) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::readCharAndAdvance(unit::U16Index &) const
    [[nodiscard]] auto readCharAndAdvance(unit::U16Index &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::readCharAndRetreat(unit::U16Index &) const
    [[nodiscard]] auto readCharAndRetreat(unit::U16Index &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::charAt(unit::CpIndex) const
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::advance(unit::U16Index &, unit::CpCount) const
    auto advance(unit::U16Index &index, unit::CpCount count = unit::CpCount::one()) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::retreat(unit::U16Index &, unit::CpCount) const
    auto retreat(unit::U16Index &index, unit::CpCount count = unit::CpCount::one()) const noexcept -> bool;

public: // byte/char index conversion.
    /// @copydoc erbsland::text::U16String::indexAt(unit::CpIndex) const
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::toCharIndex(unit::U16Index) const
    [[nodiscard]] auto toCharIndex(unit::U16Index index) const noexcept -> unit::CpIndex;

public: // slice
    /// @copydoc erbsland::text::U16String::slice(unit::U16Range) const
    [[nodiscard]] auto slice(unit::U16Range range) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(unit::CpRange) const
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::U16Count) const
    [[nodiscard]] auto slice(StringSide side, unit::U16Count length) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::U16Index) const
    [[nodiscard]] auto slice(StringSide side, unit::U16Index index) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::CpCount) const
    [[nodiscard]] auto slice(StringSide side, unit::CpCount length) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::CpIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::CpIndex index) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide) const
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U16StringEditor>;
    /// @copydoc erbsland::text::U16String::splitAt(unit::U16Index) const
    [[nodiscard]] auto splitAt(unit::U16Index index) const noexcept -> std::pair<U16StringEditor, U16StringEditor>;
    /// @copydoc erbsland::text::U16String::splitAt(unit::CpIndex) const
    [[nodiscard]] auto splitAt(unit::CpIndex index) const noexcept -> std::pair<U16StringEditor, U16StringEditor>;
    /// Split around the first matching decoded separator.
    /// @seedoc{/reference/text/strings}
    /// @param separator The string separator to exclude from the results.
    /// @param compareFn Optional decoded character comparison function.
    /// @return Shared slices before and after the match, or no value for an absent or empty separator.
    [[nodiscard]] auto splitAtFirst(const U16String &separator, CharCompareFn compareFn = {}) const noexcept
        -> std::optional<std::pair<U16StringEditor, U16StringEditor>>;
    /// Split around the first decoded character contained in the set.
    /// @seedoc{/reference/text/strings}
    /// @param separators The set of single-character separators.
    /// @return Shared slices before and after the match, or no value for an absent or empty separator.
    [[nodiscard]] auto splitAtFirst(const CharSet &separators) const noexcept
        -> std::optional<std::pair<U16StringEditor, U16StringEditor>>;
    /// Split around the last matching decoded separator.
    /// @seedoc{/reference/text/strings}
    /// @param separator The string separator to exclude from the results.
    /// @param compareFn Optional decoded character comparison function.
    /// @return Shared slices before and after the match, or no value for an absent or empty separator.
    [[nodiscard]] auto splitAtLast(const U16String &separator, CharCompareFn compareFn = {}) const noexcept
        -> std::optional<std::pair<U16StringEditor, U16StringEditor>>;
    /// Split around the last decoded character contained in the set.
    /// @seedoc{/reference/text/strings}
    /// @param separators The set of single-character separators.
    /// @return Shared slices before and after the match, or no value for an absent or empty separator.
    [[nodiscard]] auto splitAtLast(const CharSet &separators) const noexcept
        -> std::optional<std::pair<U16StringEditor, U16StringEditor>>;

public: // trim
    /// Remove leading and trailing ASCII whitespace or selected characters.
    /// @param characters The characters to remove. If omitted uses `Char::isAsciiWhitespace()` that removes
    /// space, tab, line feed, vertical tab, form feed, and carriage return (`[ \\t\\n\\v\\f]`).
    /// @param side Trim only `StringSide::Front` or `StringSide::Back`; if omitted trim both sides.
    /// @return A reference to this editor after trimming.
    auto trim(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) -> U16StringEditor &;
    /// Return a string without leading and trailing ASCII whitespace or selected characters.
    /// Trimming does not copy the original storage and returns a slice of the original string.
    /// @param characters The characters to remove. If omitted uses `Char::isAsciiWhitespace()` that removes
    /// space, tab, line feed, vertical tab, form feed, and carriage return (`[ \\t\\n\\v\\f]`).
    /// @param side Trim only `StringSide::Front` or `StringSide::Back`; if omitted trim both sides.
    /// @return A trimmed string, leaving this string unchanged.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U16StringEditor;

public: // find
    /// @copydoc erbsland::text::U16String::findFirstOf(const CharSet &) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findFirstOf(const CharSet &, unit::U16Index) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::U16Index start) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findFirstNotOf(const CharSet &) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findFirstNotOf(const CharSet &, unit::U16Index) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::U16Index start) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findLastOf(const CharSet &) const
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findLastOf(const CharSet &, unit::U16Index) const
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::U16Index end) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findLastNotOf(const CharSet &) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// @copydoc erbsland::text::U16String::findLastNotOf(const CharSet &, unit::U16Index) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::U16Index end) const noexcept -> unit::U16Index;
    /// Find text in this string.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The UTF-16 data index of the first match, or `U16Index::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U16String &text, CharCompareFn compareFn = {}) const noexcept -> unit::U16Index;
    /// Find text in this string starting at a UTF-16 data index.
    /// @param text The text to find.
    /// @param start The UTF-16 data index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @param compareFn Optional character comparison function.
    /// @return The UTF-16 data index of the first match, or `U16Index::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U16String &text, unit::U16Index start, CharCompareFn compareFn = {}) const noexcept
        -> unit::U16Index;

public: // modifiers
    /// Remove all characters from the string.
    /// StringEditor capacity is not changed.
    auto clear() noexcept -> U16StringEditor &;
    /// Reset the string to its initial state, clearing all characters and resetting capacity to default.
    void reset() noexcept;
    /// Append a UTF-16 read-only string one or more times.
    auto append(const U16String &text, unit::ItemCount count = unit::ItemCount::one()) -> U16StringEditor &;
    /// Append one Unicode code point one or more times.
    auto append(Char character, unit::CpCount count = unit::CpCount::one()) -> U16StringEditor &;
    /// Remove a UTF-16 data range.
    auto remove(unit::U16Range range) -> U16StringEditor &;
    /// Remove a character-based range.
    auto remove(unit::CpRange range) -> U16StringEditor &;
    /// Remove all characters contained in the set.
    auto removeAll(const CharSet &characters) -> U16StringEditor &;
    /// Remove all occurrences of the given decoded UTF-16 text.
    auto removeAll(const U16String &text, CharCompareFn compareFn = {}) -> U16StringEditor &;
    /// Remove the first occurrence of the given decoded UTF-16 text.
    auto removeFirst(const U16String &text, CharCompareFn compareFn = {}) -> U16StringEditor &;
    /// Keep only a UTF-16 data range.
    auto keep(unit::U16Range range) -> U16StringEditor &;
    /// Keep only a character-based range.
    auto keep(unit::CpRange range) -> U16StringEditor &;
    /// Insert text at a UTF-16 data index.
    auto insert(unit::U16Index index, const U16String &text) -> U16StringEditor &;
    /// Insert text at a character index.
    auto insert(unit::CpIndex index, const U16String &text) -> U16StringEditor &;
    /// Replace a UTF-16 data range with text.
    auto replace(unit::U16Range range, const U16String &text) -> U16StringEditor &;
    /// Replace a character-based range with text.
    auto replace(unit::CpRange range, const U16String &text) -> U16StringEditor &;
    /// Replace the first occurrence of decoded UTF-16 text.
    auto replaceFirst(const U16String &text, const U16String &replacement, CharCompareFn compareFn = {})
        -> U16StringEditor &;
    /// Replace all characters contained in the set with one character.
    auto replaceAll(const CharSet &characters, Char replacement) -> U16StringEditor &;
    /// Replace all characters contained in the set with text.
    auto replaceAll(const CharSet &characters, const U16String &replacement) -> U16StringEditor &;
    /// Replace all occurrences of decoded UTF-16 text.
    auto replaceAll(const U16String &text, const U16String &replacement, CharCompareFn compareFn = {})
        -> U16StringEditor &;
    /// Truncate this string to a maximum decoded code-point width.
    auto truncate(unit::CpCount maximumWidth, TruncateMode mode = TruncateMode::End) -> U16StringEditor &;
    /// Truncate this string to a maximum decoded code-point width, inserting an optional ellipsis.
    auto truncate(unit::CpCount maximumWidth, TruncateMode mode, const U16String &ellipsis) -> U16StringEditor &;
    /// Return a copy with a UTF-16 data range removed.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto removed(unit::U16Range range) const -> U16StringEditor;
    /// Return a copy with a character-based range removed.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U16StringEditor;
    /// Return a copy with all characters from the set removed.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U16StringEditor;
    /// Return a copy with all occurrences of decoded UTF-16 text removed.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto removedAll(const U16String &text, CharCompareFn compareFn = {}) const -> U16StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-16 text removed.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto removedFirst(const U16String &text, CharCompareFn compareFn = {}) const -> U16StringEditor;
    /// Return a copy keeping only a UTF-16 data range.
    /// Non-empty results copy the selected range into separate pruned storage.
    [[nodiscard]] auto kept(unit::U16Range range) const -> U16StringEditor;
    /// Return a copy keeping only a character-based range.
    /// Non-empty results copy the selected range into separate pruned storage.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U16StringEditor;
    /// Return a copy with text inserted at a UTF-16 data index.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto inserted(unit::U16Index index, const U16String &text) const -> U16StringEditor;
    /// Return a copy with text inserted at a character index.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U16String &text) const -> U16StringEditor;
    /// Return a copy with a UTF-16 data range replaced by text.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto replaced(unit::U16Range range, const U16String &text) const -> U16StringEditor;
    /// Return a copy with a character-based range replaced by text.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto replaced(unit::CpRange range, const U16String &text) const -> U16StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-16 text replaced.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto replacedFirst(
        const U16String &text, const U16String &replacement, CharCompareFn compareFn = {}) const -> U16StringEditor;
    /// Return a copy with all characters from the set replaced by one character.
    /// Unchanged results retain the original storage.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return The replaced value, sharing the original storage when unchanged.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U16StringEditor;
    /// Return a copy with all characters from the set replaced by text.
    /// Unchanged results retain the original storage.
    /// @param characters The characters to replace.
    /// @param replacement The replacement character or text.
    /// @return The replaced value, sharing the original storage when unchanged.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U16String &replacement) const -> U16StringEditor;
    /// Return a copy with all occurrences of decoded UTF-16 text replaced.
    /// Unchanged results retain the original storage.
    /// @param text The decoded text to replace.
    /// @param replacement The replacement character or text.
    /// @param compareFn The optional character comparison function.
    /// @return The replaced value, sharing the original storage when unchanged.
    [[nodiscard]] auto replacedAll(
        const U16String &text, const U16String &replacement, CharCompareFn compareFn = {}) const -> U16StringEditor;

public: // transform
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// @copydoc erbsland::text::U8String::transformed(TransformCharacterFn) const
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U16StringEditor;
    /// @copydoc erbsland::text::U8StringEditor::normalize(NormalizationForm)
    auto normalize(NormalizationForm form) -> U16StringEditor &;
    /// @copydoc erbsland::text::U8StringEditor::normalized(NormalizationForm) const
    [[nodiscard]] auto normalized(NormalizationForm form) const -> U16StringEditor;
    /// Return a string truncated to a maximum decoded code-point width.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto truncated(unit::CpCount maximumWidth, TruncateMode mode = TruncateMode::End) const
        -> U16StringEditor;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto truncated(unit::CpCount maximumWidth, TruncateMode mode, const U16String &ellipsis) const
        -> U16StringEditor;
    /// Return a string padded to the requested decoded code-point length.
    /// Unchanged results retain the original storage.
    [[nodiscard]] auto aligned(unit::CpCount length, geometry::Alignment alignment, Char fill = U' ') const
        -> U16StringEditor;
    /// Return a bounded representation that is safe for logs and debug output.
    /// @seedoc{/reference/text/formatting_and_parsing}
    [[nodiscard]] auto toSafeString(unit::CpCount maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
        -> U16StringEditor;

public: // conversion
    /// Convert an ASCII-case-insensitive ELCL boolean literal, or return a default for unsupported text.
    /// @param defaultValue The value returned for invalid, incomplete, padded, or empty text.
    /// @return The recognized boolean value, or `defaultValue`.
    [[nodiscard]] auto toBoolean(bool defaultValue = {}) const noexcept -> bool;
    /// Convert an ASCII-case-insensitive ELCL boolean literal.
    /// @return The recognized boolean value.
    /// @throws err::ParseError if the complete text is not a supported literal.
    [[nodiscard]] auto toBooleanOrThrow() const -> bool;
    /// Convert this string to an integer, or return the given default value on error.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto toInteger(
        T defaultValue = {}, IntegerParseOptions options = IntegerParseOptions::stringDefault()) const noexcept -> T;
    /// Convert this string to an integer or throw on parse errors and overflow.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto toIntegerOrThrow(IntegerParseOptions options = IntegerParseOptions::stringDefault()) const -> T;
    /// Convert this string to a floating point value, or return the given default value on error.
    template <impl::AnyFloatType T>
    [[nodiscard]] auto toFloat(
        T defaultValue = {}, FloatParseOptions options = FloatParseOptions::defaultOptions()) const noexcept -> T;
    /// Convert this string to a floating point value or throw on parse errors and overflow.
    template <impl::AnyFloatType T>
    [[nodiscard]] auto toFloatOrThrow(FloatParseOptions options = FloatParseOptions::defaultOptions()) const -> T;
    /// Get the size of the escaped string.
    [[nodiscard]] auto escapedSize(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const noexcept
        -> unit::U16Count;
    /// Escape this string according to the given format and amount.
    /// Unchanged results retain the original storage.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> U16StringEditor;
    /// Create a string from one Unicode code point repeated one or more times.
    [[nodiscard]] static auto fromCharacter(Char character, unit::CpCount count = unit::CpCount::one())
        -> U16StringEditor;
    /// Create a string by joining all parts without a separator.
    /// @param parts The UTF-16 read-only strings to join.
    /// @return The joined string.
    [[nodiscard]] static auto fromJoined(std::initializer_list<U16String> parts) -> U16StringEditor;
    /// Create a string from an integer using the given format.
    template <math::AnyIntegerType T>
    [[nodiscard]] static auto fromInteger(T value, IntegerFormat format = IntegerFormat::defaultFormat())
        -> U16StringEditor;
    /// Create a string from a floating point value using the given format.
    [[nodiscard]] static auto fromFloat(double value, FloatFormat format = FloatFormat::defaultFormat())
        -> U16StringEditor;
    /// Create a string from a boolean value using the given format.
    [[nodiscard]] static auto fromBoolean(bool value, BooleanFormat format = BooleanFormat::defaultFormat())
        -> U16StringEditor;
    /// Create a hexadecimal string from a byte block using the given format.
    [[nodiscard]] static auto fromByteBlock(
        const mem::ByteBlock &bytes, ByteFormat format = ByteFormat::defaultFormat()) -> U16StringEditor;

public: // low-level management
    /// Get a unique identifier for the visible storage range.
    /// The identifier changes when the string detaches, reallocates, or when a string selects a different range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;
    /// Reserve capacity for this string.
    /// @seeref{u16-string-storage-management}
    void reserve(unit::U16Count capacity);
    /// Try to free memory by shrinking the memory to the actual used size.
    /// @seeref{u16-string-storage-management}
    void shrinkToFit();
    /// Get the current storage capacity in UTF-16 code units.
    /// This function returns the reserved capacity available for the string data.
    /// @return The reserved capacity in UTF-16 code units.
    [[nodiscard]] auto capacity() const noexcept -> unit::U16Count;
    /// Get the current memory usage in bytes.
    /// This function returns an estimation of the actual memory usage, including required management data and
    /// alignment. Use this function if you need to monitor memory usage (e.g. for caching). Be aware that
    /// through fragmentation and other factors, the actual memory usage may be higher than reported.
    /// @return The estimated memory usage in bytes.
    [[nodiscard]] auto memoryUsage() const noexcept -> unit::ByteCount;
    /// Detach the string data.
    /// After a call of this method, you have exclusive access to the string data.
    /// Detach is managed automatically, only use this method if you need manual control.
    void detach();

public: // minimal std-library compatibility
    /// The value returned by this string's const iterator.
    using value_type = Char;
    /// The const iterator type for decoded UTF-16 code points.
    using const_iterator = U16StringConstIterator;
    /// @copydoc erbsland::text::U16String::begin() const
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// @copydoc erbsland::text::U16String::end() const
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two strings.
    friend void swap(U16StringEditor &first, U16StringEditor &second) noexcept;

private:
    /// Create a string with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::U16Range range) const noexcept -> U16StringEditor;
    /// Create shared results around a relative native match range.
    [[nodiscard]] auto splitAroundRange(unit::U16Range match) const noexcept
        -> std::optional<std::pair<U16StringEditor, U16StringEditor>>;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U16StringDataView;
    /// Create a new string with the given storage.
    explicit U16StringEditor(impl::U16StringSharedStorage storage) : _storage{std::move(storage)} {}
    /// Test if the string storage is shared.
    [[nodiscard]] auto isStorageShared() const noexcept -> bool;

private:
    impl::U16StringSharedStorage _storage; ///< The string storage.
};

}

template <>
struct std::hash<erbsland::text::U16StringEditor> {
    [[nodiscard]] auto operator()(const erbsland::text::U16StringEditor &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

#include "U16StringEditor_integer.tpp"
