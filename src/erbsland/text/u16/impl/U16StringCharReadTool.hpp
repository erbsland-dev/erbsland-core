// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../../unit/U16DataIndex.hpp"
#include "../../../unit/U16DataRange.hpp"
#include "../../Char.hpp"
#include "../../CharSet.hpp"

#include <span>

namespace erbsland::text::impl {

/// A character-indexed view into string data with a specific byte range.
/// This helper class provides code-point-indexed read algorithms for `U16String`.
/// @tested{U16StringTest}
class U16StringCharReadTool final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    /// Create a character-read tool for `data`.
    explicit constexpr U16StringCharReadTool(const U16StringDataView &data) noexcept : _data{data} {}

public: // read
    /// Get the character length of the view.
    [[nodiscard]] auto charLength() const noexcept -> unit::CpLength;
    /// Access the character at the given character position.
    [[nodiscard]] auto charAt(unit::CpIndex index) const noexcept -> Char;
    /// Get the start UTF-16 data index for the given code-point index.
    [[nodiscard]] auto byteIndexAt(unit::CpIndex index) const noexcept -> unit::U16DataIndex;
    /// Get the code-point index for the given UTF-16 data index.
    [[nodiscard]] auto charIndexAt(unit::U16DataIndex index) const noexcept -> unit::CpIndex;
    /// Convert a relative character range for this view into a bounded byte range for the backing data.
    [[nodiscard]] auto sliceRange(unit::CpRange range) const noexcept -> unit::U16DataRange;

public: // find
    /// Find the first decoded character from the given character set.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the first decoded character from the given character set at or after the given character index.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Find the first decoded character not from the given character set.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the first decoded character not from the given character set at or after the given character index.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Find the last decoded character from the given character set.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the last decoded character from the given character set before the given end character index.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Find the last decoded character not from the given character set.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the last decoded character not from the given character set before the given end character index.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Find the first decoded character sequence.
    [[nodiscard]] auto find(const U16StringDataView &text) const noexcept -> unit::CpIndex;
    /// Find the first decoded character sequence at or after the given character index.
    [[nodiscard]] auto find(const U16StringDataView &text, unit::CpIndex start) const noexcept -> unit::CpIndex;

private:
    /// Find the first decoded character that matches or does not match the character set.
    [[nodiscard]] auto findFirstOfCharacterSet(
        const CharacterSet &characters, unit::CpIndex start, bool isMatching) const noexcept -> unit::CpIndex;
    /// Find the last decoded character that matches or does not match the character set.
    [[nodiscard]] auto findLastOfCharacterSet(
        const CharacterSet &characters, unit::CpIndex end, bool isMatching) const noexcept -> unit::CpIndex;
    /// Test if the haystack at the candidate start matches the decoded needle.
    [[nodiscard]] static auto matchesCharacterSequence(
        std::span<const char16_t> haystack,
        unit::U16DataIndex candidateStart,
        std::span<const char16_t> needle) noexcept -> bool;

private:
    U16StringDataView _data;
};

}
