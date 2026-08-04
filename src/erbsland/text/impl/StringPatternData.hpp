// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPatternData_fwd.hpp"
#include "StringPatternElementKind.hpp"

#include "../Char.hpp"
#include "../CharRange.hpp"
#include "../StringSide.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringEditor.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"
#include "../../unit/U16DataIndex.hpp"
#include "../../unit/U16DataLength.hpp"
#include "../../unit/U16DataRange.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>

namespace erbsland::text::impl {

inline constexpr auto cNoStringPatternDivider = std::numeric_limits<std::size_t>::max(); ///< No divider is present.

/// Store one compiled string-pattern matching rule.
struct StringPatternElement final {
    StringPatternElementKind kind{StringPatternElementKind::Character}; ///< Kind of matching rule.
    Char character{};                                                   ///< Literal character to match.
    std::uint16_t rangeOffset{};                                        ///< Offset of ranges for a set rule.
    std::uint16_t rangeCount{};                                         ///< Number of ranges for a set rule.
};

/// Provide non-owning access to compiled string-pattern data.
struct StringPatternView final {
    std::span<const StringPatternElement> elements{}; ///< Compiled matching elements.
    std::span<const CharRange> ranges{};              ///< Character ranges used by set elements.
    std::size_t divider{cNoStringPatternDivider};     ///< Position of the split divider, if any.
};

/// Match, trim, and split text using compiled string-pattern data.
/// @tested{StringPatternTest}
class StringPatternData {
    /// Store matching positions for a string type.
    template <typename tString>
    struct MatchResult final {
        using Index = decltype(std::declval<tString>().indexAt(StringSide::Front));
        bool matched{};      ///< If the entire pattern matched.
        Index frontEnd{};    ///< First index after the matched prefix.
        Index suffixStart{}; ///< First index of the matched suffix.
    };

public:
    // defaults
    StringPatternData() = default;
    virtual ~StringPatternData() = default;
    StringPatternData(const StringPatternData &) = default;
    StringPatternData(StringPatternData &&) = default;
    auto operator=(const StringPatternData &) -> StringPatternData & = default;
    auto operator=(StringPatternData &&) -> StringPatternData & = default;

public:
    /// Get a non-owning view of the compiled pattern.
    [[nodiscard]] virtual auto view() const noexcept -> StringPatternView = 0;

public:
    /// Test if a UTF-8 string matches this pattern.
    [[nodiscard]] auto matches(const U8String &text) const noexcept -> bool;
    /// Test if a UTF-16 string matches this pattern.
    [[nodiscard]] auto matches(const U16String &text) const noexcept -> bool;
    /// Test if a UTF-32 string matches this pattern.
    [[nodiscard]] auto matches(const U32String &text) const noexcept -> bool;
    /// Trim matching text from a UTF-8 string.
    auto trim(U8String &text) const noexcept -> bool;
    /// Trim matching text from a UTF-16 string.
    auto trim(U16String &text) const noexcept -> bool;
    /// Trim matching text from a UTF-32 string.
    auto trim(U32String &text) const noexcept -> bool;
    /// Trim matching text from a UTF-8 string editor.
    auto trim(U8StringEditor &text) const -> bool;
    /// Trim matching text from a UTF-16 string editor.
    auto trim(U16StringEditor &text) const -> bool;
    /// Trim matching text from a UTF-32 string editor.
    auto trim(U32StringEditor &text) const -> bool;
    /// Return a trimmed copy of a UTF-8 string.
    [[nodiscard]] auto trimmed(const U8String &text) const noexcept -> U8String;
    /// Return a trimmed copy of a UTF-16 string.
    [[nodiscard]] auto trimmed(const U16String &text) const noexcept -> U16String;
    /// Return a trimmed copy of a UTF-32 string.
    [[nodiscard]] auto trimmed(const U32String &text) const noexcept -> U32String;
    /// Split a UTF-8 string at this pattern's divider.
    [[nodiscard]] auto split(const U8String &text) const noexcept -> std::pair<U8String, U8String>;
    /// Split a UTF-16 string at this pattern's divider.
    [[nodiscard]] auto split(const U16String &text) const noexcept -> std::pair<U16String, U16String>;
    /// Split a UTF-32 string at this pattern's divider.
    [[nodiscard]] auto split(const U32String &text) const noexcept -> std::pair<U32String, U32String>;
    /// Get the matched length in a UTF-8 string.
    [[nodiscard]] auto length(const U8String &text) const noexcept -> unit::ByteLength;
    /// Get the matched length in a UTF-16 string.
    [[nodiscard]] auto length(const U16String &text) const noexcept -> unit::U16DataLength;
    /// Get the matched length in a UTF-32 string.
    [[nodiscard]] auto length(const U32String &text) const noexcept -> unit::CpLength;
    /// Get the first matched index in a UTF-8 string.
    [[nodiscard]] auto index(const U8String &text) const noexcept -> unit::ByteIndex;
    /// Get the first matched index in a UTF-16 string.
    [[nodiscard]] auto index(const U16String &text) const noexcept -> unit::U16DataIndex;
    /// Get the first matched index in a UTF-32 string.
    [[nodiscard]] auto index(const U32String &text) const noexcept -> unit::CpIndex;

private:
    /// Calculate a length from the start to an index.
    template <typename tLength, typename tIndex>
    [[nodiscard]] static auto lengthFromStart(tIndex index) noexcept -> tLength;
    /// Calculate a length from an index to the end.
    template <typename tLength, typename tIndex>
    [[nodiscard]] static auto lengthToEnd(tLength length, tIndex index) noexcept -> tLength;
    /// Match a pattern against a string and retain its boundaries.
    template <typename tString>
    [[nodiscard]] auto match(const StringPatternView &patternView, const tString &text) const noexcept
        -> MatchResult<tString>;
    /// Match the prefix of a pattern against a string.
    template <typename tString, typename tIndex>
    [[nodiscard]] auto matchFront(
        const StringPatternView &patternView, const tString &text, std::size_t begin, std::size_t end, tIndex &index)
        const noexcept -> bool;
    /// Match the suffix of a pattern against a string.
    template <typename tString, typename tIndex>
    [[nodiscard]] auto matchBack(
        const StringPatternView &patternView, const tString &text, std::size_t begin, std::size_t end, tIndex &index)
        const noexcept -> bool;
    /// Test whether a character matches one compiled element.
    [[nodiscard]] static auto matchesElement(
        const StringPatternView &patternView, const StringPatternElement &element, Char character) noexcept -> bool;
};

}

#include "StringPatternData.tpp"
