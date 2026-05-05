// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"
#include "U16StringView_fwd.hpp"

#include "impl/U16StringTransformTools.hpp"
#include "impl/U16StringViewStorage.hpp"

#include "../Char.hpp"
#include "../CharCompareFn.hpp"
#include "../CharSet.hpp"
#include "../impl/StringConversionTools_fwd.hpp"
#include "../ProcessCharacterFn.hpp"
#include "../SafeStringFlag.hpp"
#include "../StringEncoding.hpp"
#include "../StringSide.hpp"
#include "../TransformCharacterFn.hpp"
#include "../TruncateMode.hpp"

#include "../../bgeo/Alignment.hpp"
#include "../../mem/ByteBlock_fwd.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../util/LoopResult.hpp"

#include <cstddef>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace erbsland::text {

/// A specialized owning UTF-16 read-only string with copy-on-write semantics for code-point index access.
/// Accessing characters via a code-point index is slow.
/// Use this interface only if it solves a specific use-case, and performance does not matter.
/// For most use cases, use `StringView` or if UTF-16 matters, use `U16StringView`.
/// @tested{U16StringTest}
class U16StringCharView final {
    friend class U16String;
    friend class U16StringView;
    friend class impl::StringConversionTools;

private:
    explicit U16StringCharView(impl::U16StringViewStorage storage) noexcept : _storage{std::move(storage)} {}

public:
    // default
    ~U16StringCharView() = default;
    U16StringCharView(const U16StringCharView &) = delete;
    U16StringCharView(U16StringCharView &&) = delete;
    auto operator=(const U16StringCharView &) -> U16StringCharView & = delete;
    auto operator=(U16StringCharView &&) -> U16StringCharView & = delete;

public: // tests
    /// Test if this view contains no decoded characters.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this view starts with another string.
    [[nodiscard]] auto startsWith(const U16StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this view ends with another string.
    [[nodiscard]] auto endsWith(const U16StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this view contains another string.
    [[nodiscard]] auto contains(const U16StringView &other, CharCompareFn compareFn = {}) const noexcept -> bool;
    /// Test if this view contains any decoded character from the set.
    [[nodiscard]] auto containsOneOf(const CharSet &characters) const noexcept -> bool;
    /// Test if this view only contains characters from the given set.
    /// Malformed UTF-16 is decoded as `Char::replacement()`.
    /// @param characters The character set to match.
    /// @return `true` all characters in the string are from the given set.
    [[nodiscard]] auto containsOnly(const CharSet &characters) const noexcept -> bool;

public: // read
    /// Get the character length of this string.
    /// This method provides the number of code-points in the string.
    /// Counting follows the tolerant UTF-16 index movement rule documented by `U16String`.
    [[nodiscard]] auto length() const noexcept -> unit::CpLength;
    /// Get the character index for one side of the string.
    [[nodiscard]] auto indexAt(StringSide side) const noexcept -> unit::CpIndex;
    /// Get the first or last character in this string.
    [[nodiscard]] auto charAt(StringSide side) const noexcept -> Char;
    /// Slow: Access the character at the given character position.
    /// @seeref{u16-string-char-view-character-based-reading}
    /// @param index The character index to access the character at.
    /// @return The character at the given character index, or a null character if no character can be read there.
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;

public: // slice
    /// Slow: Return a slice of this string.
    /// Sequentially iterates over characters until the selected slice is found.
    /// @param range The character range to slice.
    /// @return The sliced string.
    [[nodiscard]] auto slice(unit::CpRange range) const noexcept -> U16StringView;
    /// Slow: Get the initial or trailing char-based portion of this string.
    [[nodiscard]] auto slice(StringSide side, unit::CpLength length) const noexcept -> U16StringView;
    /// Slow: Slice one decoded character from the given side and return it with the remaining string.
    [[nodiscard]] auto slice(StringSide side) const noexcept -> std::tuple<Char, U16StringView>;

public: // trim
    /// Return a view without leading and trailing ASCII whitespace or selected characters.
    [[nodiscard]] auto trimmed(const std::optional<CharSet> &characters = {}, std::optional<StringSide> side = {}) const
        -> U16StringView;

public: // find
    /// Slow: Find the first occurrence of a decoded character from the given character set.
    /// @param characters The character set to match.
    /// @return The character index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Slow: Find the first occurrence of a decoded character from the given character set at or after the given
    /// character index.
    /// @param characters The character set to match.
    /// @param start The character index where the search starts.
    /// @return The character index of the first match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Slow: Find the first decoded character that is not contained in the given character set.
    /// @param characters The character set to exclude.
    /// @return The character index of the first non-matching character, or `CpIndex::noIndex()` if there is none.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Slow: Find the first decoded character that is not contained in the given character set at or after the given
    /// character index.
    /// @param characters The character set to exclude.
    /// @param start The character index where the search starts.
    /// @return The character index of the first non-matching character, or `CpIndex::noIndex()` if there is none.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Slow: Find the last occurrence of a decoded character from the given character set.
    /// @param characters The character set to match.
    /// @return The character index of the last match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Slow: Find the last occurrence of a decoded character from the given character set before the given character
    /// index.
    /// @param characters The character set to match.
    /// @param end The exclusive character index where the reverse search starts.
    /// @return The character index of the last match, or `CpIndex::noIndex()` if there is no match.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Slow: Find the last decoded character that is not contained in the given character set.
    /// @param characters The character set to exclude.
    /// @return The character index of the last non-matching character, or `CpIndex::noIndex()` if there is none.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Slow: Find the last decoded character that is not contained in the given character set before the given
    /// character index.
    /// @param characters The character set to exclude.
    /// @param end The exclusive character index where the reverse search starts.
    /// @return The character index of the last non-matching character, or `CpIndex::noIndex()` if there is none.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Slow: Find the first occurrence of the decoded UTF-16 text.
    /// Invalid UTF-16 in `text` is matched as the replacement character.
    [[nodiscard]] auto find(const U16StringView &text, CharCompareFn compareFn = {}) const noexcept -> unit::CpIndex;
    /// Slow: Find the first occurrence of the decoded UTF-16 text at or after the given character index.
    [[nodiscard]] auto find(const U16StringView &text, unit::CpIndex start, CharCompareFn compareFn = {}) const noexcept
        -> unit::CpIndex;

public: // transform and copy-modify
    /// Return a copy with a character-based range removed.
    [[nodiscard]] auto removed(unit::CpRange range) const -> U16String;
    /// Return a copy with all characters from the set removed.
    [[nodiscard]] auto removedAll(const CharSet &characters) const -> U16String;
    /// Return a copy with all occurrences of decoded UTF-16 text removed.
    [[nodiscard]] auto removedAll(const U16StringView &text, CharCompareFn compareFn = {}) const -> U16String;
    /// Return a copy with the first occurrence of decoded UTF-16 text removed.
    [[nodiscard]] auto removedFirst(const U16StringView &text, CharCompareFn compareFn = {}) const -> U16String;
    /// Return a copy keeping only a character-based range.
    [[nodiscard]] auto kept(unit::CpRange range) const -> U16String;
    /// Return a copy with text inserted at a character index.
    [[nodiscard]] auto inserted(unit::CpIndex index, const U16StringView &text) const -> U16String;
    /// Return a copy with a character-based range replaced by text.
    [[nodiscard]] auto replaced(unit::CpRange range, const U16StringView &text) const -> U16String;
    /// Call a function for every decoded code point, stopping early if the function requests it.
    auto forEach(const ProcessCharacterFn &function) const -> util::LoopResult;
    /// Return a string where every decoded code point is mapped through the given function.
    [[nodiscard]] auto transformed(TransformCharacterFn function) const -> U16String;
    /// Return a string truncated to a maximum decoded code-point width.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode = TruncateMode::End) const -> U16String;
    /// Return a string truncated to a maximum decoded code-point width, inserting an optional ellipsis.
    [[nodiscard]] auto truncated(unit::CpLength maximumWidth, TruncateMode mode, const U16StringView &ellipsis) const
        -> U16String;
    /// Return a string padded to the requested decoded code-point length.
    [[nodiscard]] auto aligned(unit::CpLength length, bgeo::Alignment alignment, Char fill = U' ') const -> U16String;
    /// Return a bounded representation that is safe for logs and debug output.
    [[nodiscard]] auto toSafeString(unit::CpLength maximumWidth, SafeStringFlags flags = SafeStringFlag::Defaults) const
        -> U16String;
    /// Return a copy with all characters from the set replaced by one character.
    [[nodiscard]] auto replacedAll(const CharSet &characters, Char replacement) const -> U16String;
    /// Return a copy with all characters from the set replaced by text.
    [[nodiscard]] auto replacedAll(const CharSet &characters, const U16StringView &replacement) const -> U16String;
    /// Return a copy with all occurrences of decoded UTF-16 text replaced.
    [[nodiscard]] auto replacedAll(
        const U16StringView &text, const U16StringView &replacement, CharCompareFn compareFn = {}) const -> U16String;
    /// Return a copy with the first occurrence of decoded UTF-16 text replaced.
    [[nodiscard]] auto replacedFirst(
        const U16StringView &text, const U16StringView &replacement, CharCompareFn compareFn = {}) const -> U16String;

private:
    /// Test if this view covers the full backing storage range.
    [[nodiscard]] auto isFullStorageRange() const noexcept -> bool;
    /// Create a view for a transformation that did not change decoded text.
    [[nodiscard]] auto viewForUnchangedTransform() const -> U16StringView;
    /// Create a view with the same storage and a different storage range.
    [[nodiscard]] auto withRange(unit::U16DataRange range) const noexcept -> U16StringView;
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U16StringDataView;

private:
    impl::U16StringViewStorage _storage; ///< The view storage.
};

}
