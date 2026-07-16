// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"
#include "U8StringCharView_fwd.hpp"
#include "U8StringConstIterator_fwd.hpp"
#include "U8StringList_fwd.hpp"
#include "U8StringLiteral_fwd.hpp"
#include "U8StringView_fwd.hpp"
#include "U8StringViewList_fwd.hpp"

#include "impl/U8StringBuilder_fwd.hpp"
#include "impl/U8StringEncodingTools_fwd.hpp"
#include "impl/U8StringSharedStorage.hpp"

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
#include "../StringBuilder.hpp"
#include "../StringCharReader.hpp"
#include "../StringEncoding.hpp"
#include "../StringSide.hpp"
#include "../TransformCharacterFn.hpp"
#include "../TruncateMode.hpp"
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
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace erbsland::text {

/// An owning UTF-8 string editor with copy-on-write semantics for sequential code-point access.
/// Use it to build new and edit UTF-8 strings.
/// Use the `String` alias in user code and only `U8String` if UTF-8 encoding matters.
/// Use `StringView`/`U8StringView` for storage and read-only access.
/// Always creates a copy of the data when constructed from a view.
/// @seedoc{/reference/text/string_width_variants}
/// @tested{U8StringTest StringEscapingTest}
class U8String {
    friend class debug::impl::StringDebugAccess;
    friend class U8StringView;
    friend class U8StringCharView;
    friend class impl::U8StringEncodingTools;
    friend class impl::StringConversionTools;
    friend class impl::UnsafeU8StringAccess;
    friend class impl::UnsafeU8StringBuffer;
    friend class impl::U8StringBuilder;

public:
    using View = U8StringView; ///< The matching view type for this string.
    using Editable = U8String; ///< The matching editable string type.

public:
    /// Create a copy of the given string.
    /// @param stdString The string to copy.
    explicit U8String(std::string_view stdString);
    /// Create a copy of the given string.
    /// @param stdString The string to copy.
    explicit U8String(std::u8string_view stdString);
    /// Create a copy of the given string literal.
    /// @param literal The string literal to copy.
    explicit U8String(const U8StringLiteral<char> &literal);
    /// Create a copy of the given string literal.
    /// @param literal The string literal to copy.
    explicit U8String(const U8StringLiteral<char8_t> &literal);
    /// Create a copy of the given view.
    /// The copied data is not shared with the original view.
    /// @param view The view to copy.
    explicit U8String(const U8StringView &view);

    U8String() = default;
    ~U8String() = default;
    U8String(const U8String &) = default;
    U8String(U8String &&) = default;
    auto operator=(const U8String &) -> U8String & = default;
    auto operator=(U8String &&) -> U8String & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const U8StringView &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const U8StringView &other, other);
    /// @copydoc erbsland::text::U8StringView::operator[](unit::ByteIndex) const
    [[nodiscard]] auto operator[](unit::ByteIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8StringView::operator[](unit::CpIndex) const
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
    /// @copydoc erbsland::text::U8StringView::isEmpty() const
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::isValidUtf8() const
    [[nodiscard]] auto isValidUtf8() const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::startsWith(const U8StringView &, CharCompareFn) const
    [[nodiscard]] auto startsWith(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::endsWith(const U8StringView &, CharCompareFn) const
    [[nodiscard]] auto endsWith(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::contains(const U8StringView &, CharCompareFn) const
    [[nodiscard]] auto contains(const U8StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::count(const U8StringView &, CharCompareFn) const
    [[nodiscard]] auto count(const U8StringView &text, CharCompareFn compareFn = {}) const noexcept
        -> unit::ElementCount;
    /// @copydoc erbsland::text::U8StringView::containsOneOf(const CharSet &) const
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::containsOnly(const CharSet &) const
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;

public: // read
    /// @copydoc erbsland::text::U8StringView::length() const
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// @copydoc erbsland::text::U8StringView::characterLength() const
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength;
    /// @copydoc erbsland::text::U8StringView::displayWidth() const
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// @copydoc erbsland::text::U8StringView::indexAt(StringSide) const
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::charAt(StringSide) const
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// @copydoc erbsland::text::U8StringView::charAt(unit::ByteIndex) const
    [[nodiscard]] auto charAt(unit::ByteIndex startIndex) const noexcept -> Char;
    /// @copydoc erbsland::text::U8StringView::readCharAndAdvance(unit::ByteIndex &) const
    [[nodiscard]] auto readCharAndAdvance(unit::ByteIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8StringView::readCharAndRetreat(unit::ByteIndex &) const
    [[nodiscard]] auto readCharAndRetreat(unit::ByteIndex &index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8StringView::charAt(unit::CpIndex) const
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;
    /// @copydoc erbsland::text::U8StringView::advance(unit::ByteIndex &, unit::CpLength) const
    auto advance(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// @copydoc erbsland::text::U8StringView::retreat(unit::ByteIndex &, unit::CpLength) const
    auto retreat(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;

public: // byte/char index conversion.
    /// @copydoc erbsland::text::U8StringView::indexAt(unit::CpIndex) const
    [[nodiscard]] auto indexAt(unit::CpIndex index) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::toCharIndex(unit::ByteIndex) const
    [[nodiscard]] auto toCharIndex(unit::ByteIndex index) const noexcept -> unit::CpIndex;

public: // slice
    /// @copydoc erbsland::text::U8StringView::slice(unit::ByteRange) const
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> U8String;
    /// @copydoc erbsland::text::U8StringView::slice(unit::CpRange) const
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U8String;
    /// @copydoc erbsland::text::U8StringView::slice(StringSide, unit::ByteLength) const
    [[nodiscard]] auto slice(StringSide side, unit::ByteLength length) const noexcept -> U8String;
    /// @copydoc erbsland::text::U8StringView::slice(StringSide, unit::ByteIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::ByteIndex index) const noexcept -> U8String;
    /// @copydoc erbsland::text::U8StringView::slice(StringSide, unit::CpLength) const
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U8String;
    /// @copydoc erbsland::text::U8StringView::slice(StringSide, unit::CpIndex) const
    [[nodiscard]] auto slice(StringSide side, unit::CpIndex index) const noexcept -> U8String;
    /// @copydoc erbsland::text::U8StringView::slice(StringSide) const
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U8String>;
    /// @copydoc erbsland::text::U8StringView::splitAt(unit::ByteIndex) const
    [[nodiscard]] auto splitAt(unit::ByteIndex index) const noexcept -> std::pair<U8String, U8String>;
    /// @copydoc erbsland::text::U8StringView::splitAt(unit::CpIndex) const
    [[nodiscard]] auto splitAt(unit::CpIndex index) const noexcept -> std::pair<U8String, U8String>;

public: // trim
    /// Remove leading and trailing ASCII whitespace or selected characters.
    auto trim(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) -> U8String &;
    /// Return a copy without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U8String;

public: // find
    /// @copydoc erbsland::text::U8StringView::findFirstOf(const CharSet &) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findFirstOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findFirstNotOf(const CharSet &) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findFirstNotOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findLastOf(const CharSet &) const
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findLastOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findLastNotOf(const CharSet &) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// @copydoc erbsland::text::U8StringView::findLastNotOf(const CharSet &, unit::ByteIndex) const
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// Find text in this string.
    /// @param text The text to find.
    /// @param compareFn Optional character comparison function.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(const U8StringView &text, CharCompareFn compareFn = {}) const noexcept -> unit::ByteIndex;
    /// Find text in this string starting at a byte index.
    /// @param text The text to find.
    /// @param start The byte index where the search starts.
    ///     If `start` is no-index, this function returns no-index immediately.
    /// @param compareFn Optional character comparison function.
    /// @return The byte index of the first match, or `ByteIndex::noIndex()` if there is no match.
    [[nodiscard]] auto find(
        const U8StringView &text, unit::ByteIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::ByteIndex;

public: // modifiers
    /// Remove all characters from the string.
    /// String capacity is not changed.
    auto clear() noexcept -> U8String &;
    /// Reset the string to its initial state, clearing all characters and resetting capacity to default.
    void reset() noexcept;
    /// Append a UTF-8 string view one or more times.
    auto append(const U8StringView &text, unit::ElementCount count = unit::ElementCount::one()) -> U8String &;
    /// Append one Unicode code point one or more times.
    auto append(Char character, unit::CpLength count = unit::CpLength::one()) -> U8String &;
    /// Remove a byte-based range.
    auto remove(unit::ByteRange range) -> U8String &;
    /// Remove a character-based range.
    auto remove(unit::CpRange range) -> U8String &;
    /// Remove all characters contained in the set.
    auto removeAll(const CharSet &characters) -> U8String &;
    /// Remove all occurrences of the given decoded UTF-8 text.
    auto removeAll(const U8StringView &text, CharCompareFn compareFn = {}) -> U8String &;
    /// Remove the first occurrence of the given decoded UTF-8 text.
    auto removeFirst(const U8StringView &text, CharCompareFn compareFn = {}) -> U8String &;
    /// Keep only a byte-based range.
    auto keep(unit::ByteRange range) -> U8String &;
    /// Keep only a character-based range.
    auto keep(unit::CpRange range) -> U8String &;
    /// Insert text at a byte index.
    auto insert(unit::ByteIndex index, const U8StringView &text) -> U8String &;
    /// Insert text at a character index.
    auto insert(unit::CpIndex index, const U8StringView &text) -> U8String &;
    /// Replace a byte-based range with text.
    auto replace(unit::ByteRange range, const U8StringView &text) -> U8String &;
    /// Replace a character-based range with text.
    auto replace(unit::CpRange range, const U8StringView &text) -> U8String &;
    /// Replace the first occurrence of decoded UTF-8 text.
    auto replaceFirst(const U8StringView &text, const U8StringView &replacement, CharCompareFn compareFn = {})
        -> U8String &;
    /// Replace all characters contained in the set with one character.
    auto replaceAll(const CharSet &characters, Char replacement) -> U8String &;
    /// Replace all characters contained in the set with text.
    auto replaceAll(const CharSet &characters, const U8StringView &replacement) -> U8String &;
    /// Replace all occurrences of decoded UTF-8 text.
    auto replaceAll(const U8StringView &text, const U8StringView &replacement, CharCompareFn compareFn = {})
        -> U8String &;
    /// Truncate this string to a maximum decoded code-point width.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) -> U8String &;
    /// Truncate this string to a maximum decoded code-point width, inserting an optional ellipsis.
    auto truncate(unit::CpLength maximumWidth, TruncateMode mode, const U8StringView &ellipsis) -> U8String &;
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
    /// Return a copy with the first occurrence of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U8StringView &text, const U8StringView &replacement, CharCompareFn compareFn = {}) const -> U8String;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U8String;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U8StringView &replacement) const -> U8String;
    /// Return a copy with all occurrences of decoded UTF-8 text replaced.
    [[nodiscard]] auto replacedAll(
        const U8StringView &text, const U8StringView &replacement, CharCompareFn compareFn = {}) const -> U8String;

public: // transform
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

public: // char view
    /// @copydoc erbsland::text::U8StringView::toCharView() const
    [[nodiscard]] auto toCharView() const noexcept -> U8StringCharView;

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
        -> unit::ByteLength;
    /// Escape this string according to the given format and amount.
    /// @param format The target format for the escaping.
    /// @param amount The amount of escaping to perform.
    [[nodiscard]] auto toEscaped(EscapeFormat format, EscapeAmount amount = EscapeAmount::Balanced) const -> U8String;
    /// Create a string from one Unicode code point repeated one or more times.
    [[nodiscard]] static auto fromCharacter(Char character, unit::CpLength count = unit::CpLength::one()) -> U8String;
    /// Create a string by joining all parts without a separator.
    /// @param parts The UTF-8 string views to join.
    /// @return The joined string.
    [[nodiscard]] static auto fromJoined(std::initializer_list<U8StringView> parts) -> U8String;
    /// Create a string from an integer using the given format.
    template <math::AnyIntegerType T>
    [[nodiscard]] static auto fromInteger(T value, IntegerFormat format = IntegerFormat::defaultFormat()) -> U8String;
    /// Create a string from a floating point value using the given format.
    [[nodiscard]] static auto fromFloat(double value, FloatFormat format = FloatFormat::defaultFormat()) -> U8String;
    /// Create a string from a boolean value using the given format.
    [[nodiscard]] static auto fromBoolean(bool value, BooleanFormat format = BooleanFormat::defaultFormat())
        -> U8String;
    /// Create a hexadecimal string from a byte block using the given format.
    [[nodiscard]] static auto fromByteBlock(
        const mem::ByteBlockView &bytes, ByteFormat format = ByteFormat::defaultFormat()) -> U8String;

public: // low-level management
    /// Get a unique identifier for the visible storage range.
    /// The identifier changes when the string detaches, reallocates, or when a view selects a different range.
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
    /// @copydoc erbsland::text::U8StringView::begin() const
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    /// @copydoc erbsland::text::U8StringView::end() const
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    /// Swap two strings.
    friend void swap(U8String &first, U8String &second) noexcept;

private:
    /// Create a string with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::ByteRange range) const noexcept -> U8String;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U8StringDataView;
    /// Create a new string with the given storage.
    explicit U8String(impl::U8StringSharedStorage storage) : _storage{std::move(storage)} {}

private:
    impl::U8StringSharedStorage _storage; ///< The string storage.
};

}

template <>
struct std::hash<erbsland::text::U8String> {
    [[nodiscard]] auto operator()(const erbsland::text::U8String &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

#include "U8String_integer.tpp"
