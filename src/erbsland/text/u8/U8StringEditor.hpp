// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"
#include "U8StringConstIterator_fwd.hpp"
#include "U8StringEditor_fwd.hpp"
#include "U8StringEditorList_fwd.hpp"
#include "U8StringList_fwd.hpp"
#include "U8StringLiteral_fwd.hpp"

#include "impl/U8StringBuilder_fwd.hpp"
#include "impl/U8StringEncodingTools_fwd.hpp"
#include "impl/U8StringSharedStorage.hpp"

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
#include "../u16/U16StringEditor_fwd.hpp"
#include "../u32/U32StringEditor_fwd.hpp"

#include "../../debug/impl/StringDebugAccess_fwd.hpp"
#include "../../geometry/Alignment.hpp"
#include "../../math/IntegerTraits.hpp"
#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/StorageIdentifier.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../util/impl/ComparisonHelper.hpp"
#include "../../util/LoopResult.hpp"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace erbsland::text {

/// An owning UTF-8 string editor with copy-on-write semantics for sequential code-point access.
/// Use it as a local mutable working value for UTF-8 construction and multi-step editing.
/// Use the `StringEditor` alias in user code and only `U8StringEditor` if UTF-8 encoding matters.
/// Use `String`/`U8String` for storage, read-only access and copy-based transformations.
/// Always creates a copy of the data when constructed from a read-only string.
/// @seedoc{/reference/text/strings}
/// @tested{U8StringEditorTest StringEscapingTest BooleanConversionTest UnicodeNormalizationTest}
class U8StringEditor {
    friend class debug::impl::StringDebugAccess<U8StringEditor>;
    friend class U8String;
    friend class impl::U8StringEncodingTools;
    friend class impl::StringConversionTools;
    template <typename>
    friend class impl::StringNormalizationTools;
    friend class impl::UnsafeU8StringEditorAccess;
    friend class impl::UnsafeU8StringBuffer;
    friend class impl::U8StringBuilder;

public:
    /// Create a copy of the given string.
    /// @param stdString The string to copy.
    explicit U8StringEditor(std::string_view stdString);
    /// Create a copy of the given string.
    /// @param stdString The string to copy.
    explicit U8StringEditor(std::u8string_view stdString);
    /// Create a copy of the given string literal.
    /// @param literal The string literal to copy.
    explicit U8StringEditor(const U8StringLiteral<char> &literal);
    /// Create a copy of the given string literal.
    /// @param literal The string literal to copy.
    explicit U8StringEditor(const U8StringLiteral<char8_t> &literal);
    /// Create a copy of the given read-only string.
    /// The copied data is not shared with the original string.
    /// @param view The read-only string to copy.
    explicit U8StringEditor(const U8String &view);

    // defaults
    U8StringEditor() = default;
    ~U8StringEditor() = default;
    U8StringEditor(const U8StringEditor &) = default;
    U8StringEditor(U8StringEditor &&) = default;
    auto operator=(const U8StringEditor &) -> U8StringEditor & = default;
    auto operator=(U8StringEditor &&) -> U8StringEditor & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const U8String &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const U8String &other, other);
    /// @copydoc erbsland::text::U8String::operator[](unit::ByteIndex) const
    [[nodiscard]] auto operator[](unit::ByteIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8String::operator[](unit::CpIndex) const
    [[nodiscard]] auto operator[](unit::CpIndex index) const noexcept -> Char;

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-8 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U8String &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;
    /// Create a hash value from the decoded code points.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Create a hash value from the decoded code points after Unicode simple case folding.
    [[nodiscard]] auto toHashCI() const noexcept -> std::size_t;

public: // tests
    /// @copydoc erbsland::text::U8String::isEmpty() const
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::isSensitive() const
    [[nodiscard]] auto isSensitive() const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::markAsSensitive()
    void markAsSensitive() noexcept;
    /// @copydoc erbsland::text::U8String::isValidUtf8() const
    [[nodiscard]] auto isValidUtf8() const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::startsWith(const U8String &, CharCompareFn) const
    [[nodiscard]] auto startsWith(const U8String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::endsWith(const U8String &, CharCompareFn) const
    [[nodiscard]] auto endsWith(const U8String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::contains(const U8String &, CharCompareFn) const
    [[nodiscard]] auto contains(const U8String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::count(const U8String &, CharCompareFn) const
    [[nodiscard]] auto count(const U8String &text, CharCompareFn compareFn = {}) const noexcept -> unit::ItemCount;
    /// @copydoc erbsland::text::U8String::containsOneOf(const CharSet &) const
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::containsOnly(const CharSet &) const
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::containsOnly(AsciiCategory) const
    [[nodiscard]] auto containsOnly(AsciiCategory category) const noexcept -> bool;

public: // read
    /// @copydoc erbsland::text::U8String::length() const
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// @copydoc erbsland::text::U8String::characterLength() const
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength;
    /// @copydoc erbsland::text::U8String::displayWidth() const
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// @copydoc erbsland::text::U8String::indexAt(StringSide) const
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::charAt(StringSide) const
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// @copydoc erbsland::text::U8String::charAt(unit::ByteIndex) const
    [[nodiscard]] auto charAt(unit::ByteIndex startIndex) const noexcept -> Char;
    /// @copydoc erbsland::text::U8String::readCharAndAdvance(unit::ByteIndex &) const
    [[nodiscard]] auto readCharAndAdvance(unit::ByteIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8String::readCharAndRetreat(unit::ByteIndex &) const
    [[nodiscard]] auto readCharAndRetreat(unit::ByteIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8String::charAt(unit::CpIndex) const
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8String::advance(unit::ByteIndex &, unit::CpLength) const
    auto advance(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// @copydoc erbsland::text::U8String::retreat(unit::ByteIndex &, unit::CpLength) const
    auto retreat(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;

public: // byte/char index conversion.
    /// @copydoc erbsland::text::U8String::indexAt(unit::CpIndex) const
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::toCharIndex(unit::ByteIndex) const
    [[nodiscard]] auto toCharIndex(unit::ByteIndex index) const noexcept -> unit::CpIndex;

public: // slice
    /// @copydoc erbsland::text::U8String::slice(unit::ByteRange) const
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> U8StringEditor;
    /// @copydoc erbsland::text::U8String::slice(unit::CpRange) const
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U8StringEditor;
    /// @copydoc erbsland::text::U8String::slice(StringSide, unit::ByteLength) const
    [[nodiscard]] auto slice(StringSide side, unit::ByteLength length) const noexcept -> U8StringEditor;
    /// @copydoc erbsland::text::U8String::slice(StringSide, unit::ByteIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::ByteIndex index) const noexcept -> U8StringEditor;
    /// @copydoc erbsland::text::U8String::slice(StringSide, unit::CpLength) const
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U8StringEditor;
    /// @copydoc erbsland::text::U8String::slice(StringSide, unit::CpIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::CpIndex index) const noexcept -> U8StringEditor;
    /// @copydoc erbsland::text::U8String::slice(StringSide) const
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U8StringEditor>;
    /// @copydoc erbsland::text::U8String::splitAt(unit::ByteIndex) const
    [[nodiscard]] auto splitAt(unit::ByteIndex index) const noexcept -> std::pair<U8StringEditor, U8StringEditor>;
    /// @copydoc erbsland::text::U8String::splitAt(unit::CpIndex) const
    [[nodiscard]] auto splitAt(unit::CpIndex index) const noexcept -> std::pair<U8StringEditor, U8StringEditor>;

public: // trim
    /// Remove leading and trailing ASCII whitespace or selected characters.
    auto trim(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) -> U8StringEditor &;
    /// Return a copy without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U8StringEditor;

public: // find
    /// @copydoc erbsland::text::U8String::findFirstOf(const CharSet &) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findFirstOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findFirstNotOf(const CharSet &) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findFirstNotOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findLastOf(const CharSet &) const
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findLastOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findLastNotOf(const CharSet &) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8String::findLastNotOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// Find text in this string.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U8String &text, CharCompareFn compareFn = {}) const noexcept -> unit::ByteIndex;
    /// Find text in this string starting at a byte index.
    /// @param text The text to find.
    /// @param start The byte index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @param compareFn Optional character comparison function.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U8String &text, unit::ByteIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::ByteIndex;

public: // modifiers
    /// Remove all characters from the string.
    /// StringEditor capacity is not changed.
    auto clear() noexcept -> U8StringEditor &;
    /// Reset the string to its initial state, clearing all characters and resetting capacity to default.
    void reset() noexcept;
    /// Append a UTF-8 read-only string one or more times.
    auto append(const U8String &text, unit::ItemCount count = unit::ItemCount::one()) -> U8StringEditor &;
    /// Append one Unicode code point one or more times.
    auto append(Char character, unit::CpLength count = unit::CpLength::one()) -> U8StringEditor &;
    /// Remove a byte-based range.
    auto remove(unit::ByteRange range) -> U8StringEditor &;
    /// Remove a character-based range.
    auto remove(unit::CpRange range) -> U8StringEditor &;
    /// Remove all characters contained in the set.
    auto removeAll(const CharSet &characters) -> U8StringEditor &;
    /// Remove all occurrences of the given decoded UTF-8 text.
    auto removeAll(const U8String &text, CharCompareFn compareFn = {}) -> U8StringEditor &;
    /// Remove the first occurrence of the given decoded UTF-8 text.
    auto removeFirst(const U8String &text, CharCompareFn compareFn = {}) -> U8StringEditor &;
    /// Keep only a byte-based range.
    auto keep(unit::ByteRange range) -> U8StringEditor &;
    /// Keep only a character-based range.
    auto keep(unit::CpRange range) -> U8StringEditor &;
    /// Insert text at a byte index.
    auto insert(unit::ByteIndex index, const U8String &text) -> U8StringEditor &;
    /// Insert text at a character index.
    auto insert(unit::CpIndex index, const U8String &text) -> U8StringEditor &;
    /// Replace a byte-based range with text.
    auto replace(unit::ByteRange range, const U8String &text) -> U8StringEditor &;
    /// Replace a character-based range with text.
    auto replace(unit::CpRange range, const U8String &text) -> U8StringEditor &;
    /// Replace the first occurrence of decoded UTF-8 text.
    auto replaceFirst(const U8String &text, const U8String &replacement, CharCompareFn compareFn = {})
        -> U8StringEditor &;
    /// Replace all characters contained in the set with one character.
    auto replaceAll(const CharSet &characters, Char replacement) -> U8StringEditor &;
    /// Replace all characters contained in the set with text.
    auto replaceAll(const CharSet &characters, const U8String &replacement) -> U8StringEditor &;
    /// Replace all occurrences of decoded UTF-8 text.
    auto replaceAll(const U8String &text, const U8String &replacement, CharCompareFn compareFn = {})
        -> U8StringEditor &;
    /// Truncate this string to a maximum decoded code-point width.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) -> U8StringEditor &;
    /// Truncate this string to a maximum decoded code-point width, inserting an optional ellipsis.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode, const U8String &ellipsis) -> U8StringEditor &;
    /// Return a copy with a byte-based range removed.
    [[nodiscard]] auto removed(unit::ByteRange range) const -> U8StringEditor;
    /// Return a copy with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U8StringEditor;
    /// Return a copy with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U8StringEditor;
    /// Return a copy with all occurrences of decoded UTF-8 text removed.
    [[nodiscard]] auto removedAll(const U8String &text, CharCompareFn compareFn = {}) const -> U8StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-8 text removed.
    [[nodiscard]] auto removedFirst(const U8String &text, CharCompareFn compareFn = {}) const -> U8StringEditor;
    /// Return a copy keeping only a byte-based range.
    [[nodiscard]] auto kept(unit::ByteRange range) const -> U8StringEditor;
    /// Return a copy keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U8StringEditor;
    /// Return a copy with text inserted at a byte index.
    [[nodiscard]] auto inserted(unit::ByteIndex index, const U8String &text) const -> U8StringEditor;
    /// Return a copy with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U8String &text) const -> U8StringEditor;
    /// Return a copy with a byte-based range replaced by text.
    [[nodiscard]] auto replaced(unit::ByteRange range, const U8String &text) const -> U8StringEditor;
    /// Return a copy with a character-based range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U8String &text) const -> U8StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U8String &text, const U8String &replacement, CharCompareFn compareFn = {}) const -> U8StringEditor;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U8StringEditor;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U8String &replacement) const -> U8StringEditor;
    /// Return a copy with all occurrences of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedAll(
        const U8String &text, const U8String &replacement, CharCompareFn compareFn = {}) const -> U8StringEditor;

public: // transform
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return a transformed copy of this string editor.
    /// The function receives every decoded character and returns its replacement. Returning
    /// `Char::noCodePoint()` removes the current character, and returning `Char::endOfData()` stops the
    /// transformation and truncates the result at that position. A null function returns an unchanged copy.
    /// The function can be called more than once for the same character and must have no observable side effects.
    /// Malformed UTF-8 is decoded as U+FFFD before it is passed to the function. If no character changes, the
    /// returned editor shares its storage with this editor. This method does not modify this editor.
    /// @param function The character transformation function.
    /// @return The transformed string editor.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U8StringEditor;
    /// Normalize this string in place using the selected Unicode normalization form.
    /// Malformed UTF-8 is replaced with U+FFFD. Storage is untouched if no change is required.
    /// @param form The explicit normalization form to apply.
    /// @return This editor for chaining.
    /// @usesunidb{Uses the generated Unicode normalization database.}
    /// @seedoc{/topics/text/normalizing_strings}
    auto normalize(NormalizationForm form) -> U8StringEditor &;
    /// Return this string in the selected Unicode normalization form.
    /// Malformed UTF-8 is replaced with U+FFFD. Unchanged valid text retains its original storage.
    /// @param form The explicit normalization form to apply.
    /// @return The normalized string, sharing this storage if no change is required.
    /// @usesunidb{Uses the generated Unicode normalization database.}
    /// @seedoc{/topics/text/normalizing_strings}
    [[nodiscard]] auto normalized(NormalizationForm form) const -> U8StringEditor;
    /// Return a string truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) const
        -> U8StringEditor;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U8String &ellipsis) const
        -> U8StringEditor;
    /// Return a string padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, geometry::Alignment alignment, Char fill = U' ') const
        -> U8StringEditor;
    /// Return a bounded representation that is safe for logs and debug output.
    /// @seedoc{/reference/text/formatting_and_parsing}
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
        -> U8StringEditor;

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
        -> unit::ByteLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> U8StringEditor;
    /// Create a string from one Unicode code point repeated one or more times.
    [[nodiscard]] static auto fromCharacter(Char character, unit::CpLength count = unit::CpLength::one())
        -> U8StringEditor;
    /// Create a string by joining all parts without a separator.
    /// @param parts The UTF-8 read-only strings to join.
    /// @return The joined string.
    [[nodiscard]] static auto fromJoined(std::initializer_list<U8String> parts) -> U8StringEditor;
    /// Create a string from an integer using the given format.
    template <math::AnyIntegerType T>
    [[nodiscard]] static auto fromInteger(T value, IntegerFormat format = IntegerFormat::defaultFormat())
        -> U8StringEditor;
    /// Create a string from a floating point value using the given format.
    [[nodiscard]] static auto fromFloat(double value, FloatFormat format = FloatFormat::defaultFormat())
        -> U8StringEditor;
    /// Create a string from a boolean value using the given format.
    [[nodiscard]] static auto fromBoolean(bool value, BooleanFormat format = BooleanFormat::defaultFormat())
        -> U8StringEditor;
    /// Create a hexadecimal string from a byte block using the given format.
    [[nodiscard]] static auto fromByteBlock(
        const mem::ByteBlock &bytes, ByteFormat format = ByteFormat::defaultFormat()) -> U8StringEditor;

public: // low-level management
    /// Get a unique identifier for the visible storage range.
    /// The identifier changes when the string detaches, reallocates, or when a string selects a different range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;
    /// Reserve capacity for this string.
    /// @seeref{u8-string-storage-management}
    void reserve(unit::ByteLength capacity);
    /// Try to free memory by shrinking the memory to the actual used size.
    /// @seeref{u8-string-storage-management}
    void shrinkToFit();
    /// Get the current memory capacity in bytes.
    /// This function returns the reserved capacity available for the string data.
    /// @return The reserved capacity in bytes.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Get the current memory usage in bytes.
    /// This function returns an estimation of the actual memory usage, including required management data and
    /// alignment. Use this function if you need to monitor memory usage (e.g. for caching). Be aware that
    /// through fragmentation and other factors, the actual memory usage may be higher than reported.
    /// @return The estimated memory usage in bytes.
    [[nodiscard]] auto memoryUsage() const noexcept -> unit::ByteLength;
    /// Detach the string data.
    /// After a call of this method, you have exclusive access to the string data.
    /// Detach is managed automatically, only use this method if you need manual control.
    void detach();

public: // minimal std-library compatibility
    /// The value returned by this string's const iterator.
    using value_type = Char;
    /// The const iterator type for decoded UTF-8 code points.
    using const_iterator = U8StringConstIterator;
    /// @copydoc erbsland::text::U8String::begin() const
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// @copydoc erbsland::text::U8String::end() const
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two strings.
    friend void swap(U8StringEditor &first, U8StringEditor &second) noexcept;

private:
    /// Create a string with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::ByteRange range) const noexcept -> U8StringEditor;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U8StringDataView;
    /// Create a new string with the given storage.
    explicit U8StringEditor(impl::U8StringSharedStorage storage) : _storage{std::move(storage)} {}
    /// Test if the string storage is shared.
    [[nodiscard]] auto isStorageShared() const noexcept -> bool;

private:
    impl::U8StringSharedStorage _storage; ///< The string storage.
};

}

template <>
struct std::hash<erbsland::text::U8StringEditor> {
    [[nodiscard]] auto operator()(const erbsland::text::U8StringEditor &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

#include "U8StringEditor_integer.tpp"
