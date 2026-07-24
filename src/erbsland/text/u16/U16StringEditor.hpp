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
#include "../IntegerFormat.hpp"
#include "../IntegerParseOptions.hpp"
#include "../Literals.hpp"
#include "../ProcessCharacterFn.hpp"
#include "../SafeStringFlag.hpp"
#include "../StringBomMode.hpp"
#include "../StringCharReader.hpp"
#include "../StringEncoding.hpp"
#include "../StringSide.hpp"
#include "../TransformCharacterFn.hpp"
#include "../TruncateMode.hpp"
#include "../u32/U32StringEditor_fwd.hpp"
#include "../u8/U8StringEditor_fwd.hpp"

#include "../../bgeo/Alignment.hpp"
#include "../../debug/impl/StringDebugAccess_fwd.hpp"
#include "../../math/IntegerTraits.hpp"
#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/StorageIdentifier.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/ElementCount.hpp"
#include "../../unit/U16DataIndex.hpp"
#include "../../unit/U16DataLength.hpp"
#include "../../unit/U16DataRange.hpp"
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

/// An owning UTF-16 string editor with copy-on-write semantics for sequential code-point access.
/// Use it to build new and edit UTF-16 strings.
/// Use `U16String` for storage and read-only access.
/// Always creates a copy of the data when constructed from a read-only string.
/// Use `StringEditor` for most use cases and `U16StringEditor` only if you need random access to code points or require
/// UTF-16 encoding.
/// @seedoc{/reference/text/string_width_variants}
/// @tested{U16StringTest StringEscapingTest BooleanConversionTest}
class U16StringEditor {
    friend class debug::impl::StringDebugAccess;
    friend class U16String;
    friend class impl::U16StringBuilder;
    friend class impl::U16StringEncodingTools;
    friend class impl::StringConversionTools;
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
    /// @copydoc erbsland::text::U16String::operator[](unit::U16DataIndex) const
    [[nodiscard]] auto operator[](unit::U16DataIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::operator[](unit::CpIndex) const
    [[nodiscard]] auto operator[](unit::CpIndex index) const noexcept -> Char;

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-16 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U16String &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;
    /// Create a hash value from the decoded code points.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Create a hash value from the decoded code points after Unicode simple case folding.
    [[nodiscard]] auto toHashCI() const noexcept -> std::size_t;

public: // tests
    /// @copydoc erbsland::text::U16String::isEmpty() const
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::isValidUtf16() const
    [[nodiscard]] auto isValidUtf16() const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::startsWith(const U16String &, CharCompareFn) const
    [[nodiscard]] auto startsWith(const U16String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::endsWith(const U16String &, CharCompareFn) const
    [[nodiscard]] auto endsWith(const U16String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::contains(const U16String &, CharCompareFn) const
    [[nodiscard]] auto contains(const U16String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::count(const U16String &, CharCompareFn) const
    [[nodiscard]] auto count(const U16String &text, CharCompareFn compareFn = {}) const noexcept -> unit::ElementCount;
    /// @copydoc erbsland::text::U16String::containsOneOf(const CharSet &) const
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::containsOnly(const CharSet &) const
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;

public: // read
    /// @copydoc erbsland::text::U16String::length() const
    [[nodiscard]] auto length() const noexcept -> unit::U16DataLength;
    /// @copydoc erbsland::text::U16String::characterLength() const
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength;
    /// @copydoc erbsland::text::U16String::displayWidth() const
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// @copydoc erbsland::text::U16String::indexAt(StringSide) const
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::charAt(StringSide) const
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::charAt(unit::U16DataIndex) const
    [[nodiscard]] auto charAt(unit::U16DataIndex startIndex) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::readCharAndAdvance(unit::U16DataIndex &) const
    [[nodiscard]] auto readCharAndAdvance(unit::U16DataIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::readCharAndRetreat(unit::U16DataIndex &) const
    [[nodiscard]] auto readCharAndRetreat(unit::U16DataIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::charAt(unit::CpIndex) const
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U16String::advance(unit::U16DataIndex &, unit::CpLength) const
    auto advance(unit::U16DataIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// @copydoc erbsland::text::U16String::retreat(unit::U16DataIndex &, unit::CpLength) const
    auto retreat(unit::U16DataIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;

public: // byte/char index conversion.
    /// @copydoc erbsland::text::U16String::indexAt(unit::CpIndex) const
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::toCharIndex(unit::U16DataIndex) const
    [[nodiscard]] auto toCharIndex(unit::U16DataIndex index) const noexcept -> unit::CpIndex;

public: // slice
    /// @copydoc erbsland::text::U16String::slice(unit::U16DataRange) const
    [[nodiscard]] auto slice(unit::U16DataRange range) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(unit::CpRange) const
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::U16DataLength) const
    [[nodiscard]] auto slice(StringSide side, unit::U16DataLength length) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::U16DataIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::U16DataIndex index) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::CpLength) const
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide, unit::CpIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::CpIndex index) const noexcept -> U16StringEditor;
    /// @copydoc erbsland::text::U16String::slice(StringSide) const
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U16StringEditor>;
    /// @copydoc erbsland::text::U16String::splitAt(unit::U16DataIndex) const
    [[nodiscard]] auto splitAt(unit::U16DataIndex index) const noexcept -> std::pair<U16StringEditor, U16StringEditor>;
    /// @copydoc erbsland::text::U16String::splitAt(unit::CpIndex) const
    [[nodiscard]] auto splitAt(unit::CpIndex index) const noexcept -> std::pair<U16StringEditor, U16StringEditor>;

public: // trim
    /// Remove leading and trailing ASCII whitespace or selected characters.
    auto trim(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) -> U16StringEditor &;
    /// Return a copy without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U16StringEditor;

public: // find
    /// @copydoc erbsland::text::U16String::findFirstOf(const CharSet &) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findFirstOf(const CharSet &, unit::U16DataIndex) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::U16DataIndex start) const noexcept
        -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findFirstNotOf(const CharSet &) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findFirstNotOf(const CharSet &, unit::U16DataIndex) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::U16DataIndex start) const noexcept
        -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findLastOf(const CharSet &) const
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findLastOf(const CharSet &, unit::U16DataIndex) const
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::U16DataIndex end) const noexcept
        -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findLastNotOf(const CharSet &) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::U16DataIndex;
    /// @copydoc erbsland::text::U16String::findLastNotOf(const CharSet &, unit::U16DataIndex) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::U16DataIndex end) const noexcept
        -> unit::U16DataIndex;
    /// Find text in this string.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The UTF-16 data index of the first match, or `U16DataIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U16String &text, CharCompareFn compareFn = {}) const noexcept -> unit::U16DataIndex;
    /// Find text in this string starting at a UTF-16 data index.
    /// @param text The text to find.
    /// @param start The UTF-16 data index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @param compareFn Optional character comparison function.
    /// @return The UTF-16 data index of the first match, or `U16DataIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(
        const U16String &text, unit::U16DataIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::U16DataIndex;

public: // modifiers
    /// Remove all characters from the string.
    /// StringEditor capacity is not changed.
    auto clear() noexcept -> U16StringEditor &;
    /// Reset the string to its initial state, clearing all characters and resetting capacity to default.
    void reset() noexcept;
    /// Append a UTF-16 read-only string one or more times.
    auto append(const U16String &text, unit::ElementCount count = unit::ElementCount::one()) -> U16StringEditor &;
    /// Append one Unicode code point one or more times.
    auto append(Char character, unit::CpLength count = unit::CpLength::one()) -> U16StringEditor &;
    /// Remove a UTF-16 data range.
    auto remove(unit::U16DataRange range) -> U16StringEditor &;
    /// Remove a character-based range.
    auto remove(unit::CpRange range) -> U16StringEditor &;
    /// Remove all characters contained in the set.
    auto removeAll(const CharSet &characters) -> U16StringEditor &;
    /// Remove all occurrences of the given decoded UTF-16 text.
    auto removeAll(const U16String &text, CharCompareFn compareFn = {}) -> U16StringEditor &;
    /// Remove the first occurrence of the given decoded UTF-16 text.
    auto removeFirst(const U16String &text, CharCompareFn compareFn = {}) -> U16StringEditor &;
    /// Keep only a UTF-16 data range.
    auto keep(unit::U16DataRange range) -> U16StringEditor &;
    /// Keep only a character-based range.
    auto keep(unit::CpRange range) -> U16StringEditor &;
    /// Insert text at a UTF-16 data index.
    auto insert(unit::U16DataIndex index, const U16String &text) -> U16StringEditor &;
    /// Insert text at a character index.
    auto insert(unit::CpIndex index, const U16String &text) -> U16StringEditor &;
    /// Replace a UTF-16 data range with text.
    auto replace(unit::U16DataRange range, const U16String &text) -> U16StringEditor &;
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
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) -> U16StringEditor &;
    /// Truncate this string to a maximum decoded code-point width, inserting an optional ellipsis.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode, const U16String &ellipsis) -> U16StringEditor &;
    /// Return a copy with a UTF-16 data range removed.
    [[nodiscard]] auto removed(unit::U16DataRange range) const -> U16StringEditor;
    /// Return a copy with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U16StringEditor;
    /// Return a copy with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U16StringEditor;
    /// Return a copy with all occurrences of decoded UTF-16 text removed.
    [[nodiscard]] auto removedAll(const U16String &text, CharCompareFn compareFn = {}) const -> U16StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-16 text removed.
    [[nodiscard]] auto removedFirst(const U16String &text, CharCompareFn compareFn = {}) const -> U16StringEditor;
    /// Return a copy keeping only a UTF-16 data range.
    [[nodiscard]] auto kept(unit::U16DataRange range) const -> U16StringEditor;
    /// Return a copy keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U16StringEditor;
    /// Return a copy with text inserted at a UTF-16 data index.
    [[nodiscard]] auto inserted(unit::U16DataIndex index, const U16String &text) const -> U16StringEditor;
    /// Return a copy with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U16String &text) const -> U16StringEditor;
    /// Return a copy with a UTF-16 data range replaced by text.
    [[nodiscard]] auto replaced(unit::U16DataRange range, const U16String &text) const -> U16StringEditor;
    /// Return a copy with a character-based range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U16String &text) const -> U16StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-16 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U16String &text, const U16String &replacement, CharCompareFn compareFn = {}) const -> U16StringEditor;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U16StringEditor;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U16String &replacement) const -> U16StringEditor;
    /// Return a copy with all occurrences of decoded UTF-16 text replaced.
    [[nodiscard]] auto replacedAll(
        const U16String &text, const U16String &replacement, CharCompareFn compareFn = {}) const -> U16StringEditor;

public: // transform
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return a string where every decoded code point is mapped through the given function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U16StringEditor;
    /// Return a string truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) const
        -> U16StringEditor;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U16String &ellipsis) const
        -> U16StringEditor;
    /// Return a string padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill = U' ') const
        -> U16StringEditor;
    /// Return a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
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
        -> unit::U16DataLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> U16StringEditor;
    /// Create a string from one Unicode code point repeated one or more times.
    [[nodiscard]] static auto fromCharacter(Char character, unit::CpLength count = unit::CpLength::one())
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
    void reserve(unit::U16DataLength capacity);
    /// Try to free memory by shrinking the memory to the actual used size.
    /// @seeref{u16-string-storage-management}
    void shrinkToFit();
    /// Get the current storage capacity in UTF-16 code units.
    /// This function returns the reserved capacity available for the string data.
    /// @return The reserved capacity in UTF-16 code units.
    [[nodiscard]] auto capacity() const noexcept -> unit::U16DataLength;
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
    [[nodiscard]] auto withRange(unit::U16DataRange range) const noexcept -> U16StringEditor;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U16StringDataView;
    /// Create a new string with the given storage.
    explicit U16StringEditor(impl::U16StringSharedStorage storage) : _storage{std::move(storage)} {}

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
