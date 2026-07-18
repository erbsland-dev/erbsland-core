// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String_fwd.hpp"
#include "U32StringConstIterator_fwd.hpp"
#include "U32StringEditor_fwd.hpp"
#include "U32StringEditorList_fwd.hpp"
#include "U32StringList_fwd.hpp"
#include "U32StringLiteral_fwd.hpp"

#include "impl/U32StringEncodingTools_fwd.hpp"
#include "impl/U32StringSharedStorage.hpp"

#include "../AnyStringBuilder.hpp"
#include "../BooleanFormat.hpp"
#include "../Char.hpp"
#include "../CharCompareFn.hpp"
#include "../CharSet.hpp"
#include "../EncodingErrorMode.hpp"
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
#include "../u16/U16StringEditor_fwd.hpp"
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

/// An owning UTF-32 string editor with copy-on-write semantics for random code-point access.
/// Use it to build new and edit UTF-32 strings.
/// Use `U32String` for storage and read-only access.
/// Always creates a copy of the data when constructed from a read-only string.
/// Use `StringEditor` for most use cases and `U32StringEditor` only if you need random access to code points or require
/// UTF-32 encoding.
/// @seedoc{/reference/text/string_width_variants}
/// @tested{U32StringTest StringEscapingTest}
class U32StringEditor {
    friend class debug::impl::StringDebugAccess;
    friend class U32String;
    friend class impl::U32StringEncodingTools;
    friend class impl::StringConversionTools;

public:
    /// Create a copy of the given UTF-32 string.
    /// @param stdString The string to copy.
    explicit U32StringEditor(std::u32string_view stdString);
    /// Create a copy of the given string literal.
    /// @param literal The string literal to copy.
    explicit U32StringEditor(const U32StringLiteral &literal);
    /// Create a copy of the given read-only string.
    /// The copied data is not shared with the original string.
    /// @param view The read-only string to copy.
    explicit U32StringEditor(const U32String &view);

    U32StringEditor() = default;
    ~U32StringEditor() = default;
    U32StringEditor(const U32StringEditor &) = default;
    U32StringEditor(U32StringEditor &&) = default;
    auto operator=(const U32StringEditor &) -> U32StringEditor & = default;
    auto operator=(U32StringEditor &&) -> U32StringEditor & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const U32String &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const U32String &other, other);
    /// @copydoc erbsland::text::U32String::operator[](unit::CpIndex) const
    [[nodiscard]] auto operator[](unit::CpIndex index) const noexcept -> Char;

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed UTF-32 with `Char::replacement()`.
    [[nodiscard]] auto compare(const U32String &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;
    /// Create a hash value from the decoded code points.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Create a hash value from the decoded code points after Unicode simple case folding.
    [[nodiscard]] auto toHashCI() const noexcept -> std::size_t;

public: // tests
    /// @copydoc erbsland::text::U32String::isEmpty() const
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::isValidUtf32() const
    [[nodiscard]] auto isValidUtf32() const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::startsWith(const U32String &, CharCompareFn) const
    [[nodiscard]] auto startsWith(const U32String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::endsWith(const U32String &, CharCompareFn) const
    [[nodiscard]] auto endsWith(const U32String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::contains(const U32String &, CharCompareFn) const
    [[nodiscard]] auto contains(const U32String &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::count(const U32String &, CharCompareFn) const
    [[nodiscard]] auto count(const U32String &text, CharCompareFn compareFn = {}) const noexcept -> unit::ElementCount;
    /// @copydoc erbsland::text::U32String::containsOneOf(const CharSet &) const
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::containsOnly(const CharSet &) const
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;

public: // read
    /// @copydoc erbsland::text::U32String::length() const
    [[nodiscard]] auto length() const noexcept -> unit::CpLength;
    /// @copydoc erbsland::text::U32String::characterLength() const
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength;
    /// @copydoc erbsland::text::U32String::displayWidth() const
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// @copydoc erbsland::text::U32String::indexAt(StringSide) const
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::charAt(StringSide) const
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// @copydoc erbsland::text::U32String::charAt(unit::CpIndex) const
    [[nodiscard]] auto charAt(unit::CpIndex startIndex) const noexcept -> Char;
    /// @copydoc erbsland::text::U32String::readCharAndAdvance(unit::CpIndex &) const
    [[nodiscard]] auto readCharAndAdvance(unit::CpIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U32String::readCharAndRetreat(unit::CpIndex &) const
    [[nodiscard]] auto readCharAndRetreat(unit::CpIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U32String::advance(unit::CpIndex &, unit::CpLength) const
    auto advance(unit::CpIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// @copydoc erbsland::text::U32String::retreat(unit::CpIndex &, unit::CpLength) const
    auto retreat(unit::CpIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;

public: // byte/char index conversion.
    /// @copydoc erbsland::text::U32String::indexAt(unit::CpIndex) const
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::toCharIndex(unit::CpIndex) const
    [[nodiscard]] auto toCharIndex(unit::CpIndex index) const noexcept -> unit::CpIndex;

public: // slice
    /// @copydoc erbsland::text::U32String::slice(unit::CpRange) const
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U32StringEditor;
    /// @copydoc erbsland::text::U32String::slice(StringSide, unit::CpLength) const
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U32StringEditor;
    /// @copydoc erbsland::text::U32String::slice(StringSide, unit::CpIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::CpIndex index) const noexcept -> U32StringEditor;
    /// @copydoc erbsland::text::U32String::slice(StringSide) const
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U32StringEditor>;
    /// @copydoc erbsland::text::U32String::splitAt(unit::CpIndex) const
    [[nodiscard]] auto splitAt(unit::CpIndex index) const noexcept -> std::pair<U32StringEditor, U32StringEditor>;

public: // trim
    /// Remove leading and trailing ASCII whitespace or selected characters.
    auto trim(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) -> U32StringEditor &;
    /// Return a copy without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U32StringEditor;

public: // find
    /// @copydoc erbsland::text::U32String::findFirstOf(const CharSet &) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findFirstOf(const CharSet &, unit::CpIndex) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findFirstNotOf(const CharSet &) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findFirstNotOf(const CharSet &, unit::CpIndex) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findLastOf(const CharSet &) const
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findLastOf(const CharSet &, unit::CpIndex) const
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findLastNotOf(const CharSet &) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// @copydoc erbsland::text::U32String::findLastNotOf(const CharSet &, unit::CpIndex) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Find text in this string.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The code point index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U32String &text, CharCompareFn compareFn = {}) const noexcept -> unit::CpIndex;
    /// Find text in this string starting at a code point index.
    /// @param text The text to find.
    /// @param start The code point index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @param compareFn Optional character comparison function.
    /// @return The code point index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U32String &text, unit::CpIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::CpIndex;

public: // modifiers
    /// Remove all characters from the string.
    /// StringEditor capacity is not changed.
    auto clear() noexcept -> U32StringEditor &;
    /// Reset the string to its initial state, clearing all characters and resetting capacity to default.
    void reset() noexcept;
    /// Append a UTF-32 read-only string one or more times.
    auto append(const U32String &text, unit::ElementCount count = unit::ElementCount::one()) -> U32StringEditor &;
    /// Append one Unicode code point one or more times.
    auto append(Char character, unit::CpLength count = unit::CpLength::one()) -> U32StringEditor &;
    /// Remove a character-based range.
    auto remove(unit::CpRange range) -> U32StringEditor &;
    /// Remove all characters contained in the set.
    auto removeAll(const CharSet &characters) -> U32StringEditor &;
    /// Remove all occurrences of the given decoded UTF-32 text.
    auto removeAll(const U32String &text, CharCompareFn compareFn = {}) -> U32StringEditor &;
    /// Remove the first occurrence of the given decoded UTF-32 text.
    auto removeFirst(const U32String &text, CharCompareFn compareFn = {}) -> U32StringEditor &;
    /// Keep only a character-based range.
    auto keep(unit::CpRange range) -> U32StringEditor &;
    /// Insert text at a character index.
    auto insert(unit::CpIndex index, const U32String &text) -> U32StringEditor &;
    /// Replace a character-based range with text.
    auto replace(unit::CpRange range, const U32String &text) -> U32StringEditor &;
    /// Replace the first occurrence of decoded UTF-32 text.
    auto replaceFirst(const U32String &text, const U32String &replacement, CharCompareFn compareFn = {})
        -> U32StringEditor &;
    /// Replace all characters contained in the set with one character.
    auto replaceAll(const CharSet &characters, Char replacement) -> U32StringEditor &;
    /// Replace all characters contained in the set with text.
    auto replaceAll(const CharSet &characters, const U32String &replacement) -> U32StringEditor &;
    /// Replace all occurrences of decoded UTF-32 text.
    auto replaceAll(const U32String &text, const U32String &replacement, CharCompareFn compareFn = {})
        -> U32StringEditor &;
    /// Truncate this string to a maximum decoded code-point width.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) -> U32StringEditor &;
    /// Truncate this string to a maximum decoded code-point width, inserting an optional ellipsis.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode, const U32String &ellipsis) -> U32StringEditor &;
    /// Return a copy with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U32StringEditor;
    /// Return a copy with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U32StringEditor;
    /// Return a copy with all occurrences of decoded UTF-32 text removed.
    [[nodiscard]] auto removedAll(const U32String &text, CharCompareFn compareFn = {}) const -> U32StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-32 text removed.
    [[nodiscard]] auto removedFirst(const U32String &text, CharCompareFn compareFn = {}) const -> U32StringEditor;
    /// Return a copy keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U32StringEditor;
    /// Return a copy with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U32String &text) const -> U32StringEditor;
    /// Return a copy with a character-based range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U32String &text) const -> U32StringEditor;
    /// Return a copy with the first occurrence of decoded UTF-32 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U32String &text, const U32String &replacement, CharCompareFn compareFn = {}) const -> U32StringEditor;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U32StringEditor;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U32String &replacement) const -> U32StringEditor;
    /// Return a copy with all occurrences of decoded UTF-32 text replaced.
    [[nodiscard]] auto replacedAll(
        const U32String &text, const U32String &replacement, CharCompareFn compareFn = {}) const -> U32StringEditor;

public: // transform
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return a string where every decoded code point is mapped through the given function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U32StringEditor;
    /// Return a string truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) const
        -> U32StringEditor;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U32String &ellipsis) const
        -> U32StringEditor;
    /// Return a string padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill = U' ') const
        -> U32StringEditor;
    /// Return a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
        -> U32StringEditor;

public: // conversion
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
        -> unit::CpLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const
        -> U32StringEditor;
    /// Create a string from one Unicode code point repeated one or more times.
    [[nodiscard]] static auto fromCharacter(Char character, unit::CpLength count = unit::CpLength::one())
        -> U32StringEditor;
    /// Create a string by joining all parts without a separator.
    /// @param parts The UTF-32 read-only strings to join.
    /// @return The joined string.
    [[nodiscard]] static auto fromJoined(std::initializer_list<U32String> parts) -> U32StringEditor;
    /// Create a string from an integer using the given format.
    template <math::AnyIntegerType T>
    [[nodiscard]] static auto fromInteger(T value, IntegerFormat format = IntegerFormat::defaultFormat())
        -> U32StringEditor;
    /// Create a string from a floating point value using the given format.
    [[nodiscard]] static auto fromFloat(double value, FloatFormat format = FloatFormat::defaultFormat())
        -> U32StringEditor;
    /// Create a string from a boolean value using the given format.
    [[nodiscard]] static auto fromBoolean(bool value, BooleanFormat format = BooleanFormat::defaultFormat())
        -> U32StringEditor;
    /// Create a hexadecimal string from a byte block using the given format.
    [[nodiscard]] static auto fromByteBlock(
        const mem::ByteBlock &bytes, ByteFormat format = ByteFormat::defaultFormat()) -> U32StringEditor;

public: // low-level management
    /// Get a unique identifier for the visible storage range.
    /// The identifier changes when the string detaches, reallocates, or when a string selects a different range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;
    /// Reserve capacity for this string.
    /// @seeref{u32-string-storage-management}
    void reserve(unit::CpLength capacity);
    /// Try to free memory by shrinking the memory to the actual used size.
    /// @seeref{u32-string-storage-management}
    void shrinkToFit();
    /// Get the current storage capacity in UTF-32 code units.
    /// This function returns the reserved capacity available for the string data.
    /// @return The reserved capacity in UTF-32 code units.
    [[nodiscard]] auto capacity() const noexcept -> unit::CpLength;
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
    /// The const iterator type for decoded UTF-32 code points.
    using const_iterator = U32StringConstIterator;
    /// @copydoc erbsland::text::U32String::begin() const
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// @copydoc erbsland::text::U32String::end() const
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two strings.
    friend void swap(U32StringEditor &first, U32StringEditor &second) noexcept;

private:
    /// Create a string with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::CpRange range) const noexcept -> U32StringEditor;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U32StringDataView;
    /// Create a new string with the given storage.
    explicit U32StringEditor(impl::U32StringSharedStorage storage) : _storage{std::move(storage)} {}

private:
    impl::U32StringSharedStorage _storage; ///< The string storage.
};

}

template <>
struct std::hash<erbsland::text::U32StringEditor> {
    [[nodiscard]] auto operator()(const erbsland::text::U32StringEditor &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

#include "U32StringEditor_integer.tpp"
