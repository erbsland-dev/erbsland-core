// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString.hpp"
#include "StringPattern_fwd.hpp"
#include "StringPatternKind.hpp"

#include "impl/StringPatternData_fwd.hpp"
#include "pattern/AnyElement.hpp"
#include "pattern/Divider.hpp"
#include "pattern/Star.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"

#include "../unit/ByteCount.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteRange.hpp"
#include "../unit/CpCount.hpp"
#include "../unit/CpIndex.hpp"
#include "../unit/CpRange.hpp"
#include "../unit/U16Count.hpp"
#include "../unit/U16Index.hpp"
#include "../unit/U16Range.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace erbsland::text {

/// A lightweight decoded-character string pattern.
/// @seedoc{/reference/text/matching_and_splitting}
/// @tested{StringPatternTest}
class StringPattern final {
public:
    /// Maximum number of elements in a typed pattern.
    static constexpr auto cMaximumElements = pattern::cMaximumElements;
    /// Maximum number of total ranges in a typed pattern.
    static constexpr auto cMaximumRanges = pattern::cMaximumStaticRanges;
    /// Maximum number of ranges in one set element.
    static constexpr auto cMaximumSetRanges = pattern::cMaximumSetRanges;

public:
    // defaults
    ~StringPattern() = default;
    StringPattern(const StringPattern &) noexcept = default;
    StringPattern(StringPattern &&) noexcept = default;
    auto operator=(const StringPattern &) noexcept -> StringPattern & = default;
    auto operator=(StringPattern &&) noexcept -> StringPattern & = default;

public: // accessors
    /// Get the matching and extraction semantics selected by the factory.
    /// @return The factory-selected Divider, Glob, or PosixGlob kind; moved-from patterns report Divider.
    [[nodiscard]] auto kind() const noexcept -> StringPatternKind;

public: // matching and extraction
    /// Test whether text matches this pattern.
    /// Divider matches a prefix, or non-overlapping anchored prefix/suffix parts around its divider.
    /// Both glob kinds require complete text to match; empty globs match empty text. An empty Divider never matches.
    /// @param text Decoded candidate text; matching is case-sensitive.
    /// @return Whether the configured pattern matches.
    [[nodiscard]] auto matches(const U8String &text) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto matches(const U16String &text) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto matches(const U32String &text) const noexcept -> bool;
    /// Remove matched text from an editable string.
    /// Divider removes the matching prefix and/or suffix; both glob kinds clear the complete text on a match.
    /// A mismatch leaves text unchanged, and a match against empty text removes nothing.
    /// @param text Text to modify.
    /// @return Whether any text was removed.
    auto trim(U8StringEditor &text) const -> bool;
    /// @overload
    auto trim(U16StringEditor &text) const -> bool;
    /// @overload
    auto trim(U32StringEditor &text) const -> bool;
    /// Return text after removing its matching parts.
    /// Divider removes its prefix and/or suffix, retaining the middle; both glob kinds return empty text on a match.
    /// @param text Text to match without modifying it.
    /// @return The remaining text on a match, or the original text on a mismatch.
    [[nodiscard]] auto trimmed(const U8String &text) const noexcept -> U8String;
    /// @overload
    [[nodiscard]] auto trimmed(const U16String &text) const noexcept -> U16String;
    /// @overload
    [[nodiscard]] auto trimmed(const U32String &text) const noexcept -> U32String;
    /// Split text according to this pattern's matching boundary.
    /// Divider returns {prefix, remainder} without a suffix, {prefix, middle} with both parts,
    /// or {leading text, suffix} with only a suffix. Both glob kinds return {text, empty} on a match.
    /// @param text Text to match without modifying it.
    /// @return The described pair on a match; {empty, text} on a mismatch.
    [[nodiscard]] auto split(const U8String &text) const noexcept -> std::pair<U8String, U8String>;
    /// @overload
    [[nodiscard]] auto split(const U16String &text) const noexcept -> std::pair<U16String, U16String>;
    /// @overload
    [[nodiscard]] auto split(const U32String &text) const noexcept -> std::pair<U32String, U32String>;
    /// Get the native length selected by a match.
    /// Divider returns the prefix length, or the suffix length for a suffix-only pattern.
    /// Both glob kinds return the complete text length. Units are bytes for UTF-8, code units for UTF-16, and code
    /// points for UTF-32.
    /// @param text Text to match.
    /// @return The selected length, or zero on a mismatch or an empty glob match.
    [[nodiscard]] auto length(const U8String &text) const noexcept -> unit::ByteCount;
    /// @overload
    [[nodiscard]] auto length(const U16String &text) const noexcept -> unit::U16Count;
    /// @overload
    [[nodiscard]] auto length(const U32String &text) const noexcept -> unit::CpCount;
    /// Get the native boundary selected by a match.
    /// Divider returns the index after its prefix, or the start of its suffix for a suffix-only pattern.
    /// Both glob kinds return the end index, including zero for an empty match; units follow the text's native
    /// encoding.
    /// @param text Text to match.
    /// @return The selected index, or no-index on a mismatch.
    [[nodiscard]] auto index(const U8String &text) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto index(const U16String &text) const noexcept -> unit::U16Index;
    /// @overload
    [[nodiscard]] auto index(const U32String &text) const noexcept -> unit::CpIndex;

public: // factories
    /// Parse a case-sensitive prefix/divider pattern from text of any supported width.
    /// Without a divider, matches() accepts a matching prefix; one * divides anchored prefix and suffix parts.
    /// An empty pattern never matches. Use fromGlob() for full-string matching with multiple stars.
    /// @param pattern Literals, ?, [a-z], [^a-z], and at most one *; backslash escapes ?, *, [, ], and backslash.
    /// @return A compiled Divider pattern.
    /// @throws ParseError If syntax is invalid, a star stands alone, or compiled range limits are exceeded.
    [[nodiscard]] static auto fromDivider(const AnyString &pattern) -> StringPattern;
    /// Create a case-sensitive prefix/divider pattern directly from typed elements.
    /// No elements creates a Divider pattern that never matches. Star elements are rejected at compile time.
    /// @tparam Args Supported typed pattern element types.
    /// @param elements The literal, character, set and optional divider elements.
    /// @return A compiled Divider pattern.
    /// @throws ParseError If the divider stands alone, repeats, or typed storage limits are exceeded.
    template <pattern::AnyElement... Args>
        requires((!std::is_same_v<std::remove_cvref_t<Args>, pattern::Star>) && ...)
    [[nodiscard]] static auto fromDivider(const Args &...elements) -> StringPattern;
    /// Compile an Erbsland Core full-string glob using Divider's escaping and character-set syntax.
    /// Stars match zero or more characters, including separators and newlines; empty globs match only empty text.
    /// @param pattern Text of any supported width: literals, *, ?, [a-z], and [^a-z], with Divider's escapes.
    /// @return A compiled Glob pattern.
    /// @throws ParseError If syntax is invalid or compiled range limits are exceeded.
    [[nodiscard]] static auto fromGlob(const AnyString &pattern) -> StringPattern;
    /// Create a full-string glob directly from typed elements.
    /// No elements matches only empty text. Divider elements are rejected at compile time.
    /// @tparam Args Supported typed pattern element types.
    /// @param elements The literal, character, set and any number of Star elements.
    /// @return A compiled Glob pattern.
    /// @throws ParseError If typed storage limits are exceeded or an element is invalid.
    template <pattern::AnyElement... Args>
        requires((!std::is_same_v<std::remove_cvref_t<Args>, pattern::Divider>) && ...)
    [[nodiscard]] static auto fromGlob(const Args &...elements) -> StringPattern;
    /// Compile a full-string glob using the existing Python-style dialect.
    /// @param pattern Text of any supported width: *, ?, [abc], [a-z], [!a-z]; backslashes and unmatched brackets
    /// are literal.
    /// @return A compiled PosixGlob; stars include separators/newlines, and an empty glob matches only empty text.
    /// @throws ParseError If characters are invalid or the compiled representation exceeds 65535 ranges.
    [[nodiscard]] static auto fromPosixGlob(const AnyString &pattern) -> StringPattern;

private:
    /// Prepare an empty value for a named factory.
    StringPattern() noexcept = default;

private:
    std::shared_ptr<const impl::StringPatternData> _data; ///< The immutable compiled pattern data.
};

}

#include "StringPattern.tpp"
