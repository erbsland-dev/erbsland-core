// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32Encoding.hpp"
#include "U32StringDataView.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../Char.hpp"
#include "../../CharSet.hpp"

#include <string>
#include <string_view>

namespace erbsland::text::impl {

/// A view into string data with a specific code-point range.
/// This helper class provides all read-only algorithms on the string data, shared by `U32String` and `U32StringView`.
/// It's meant to be used inline like `U32StringDataView{dataView()}.doSomething()`.
/// @tested{U32StringTest}
class U32StringReadTools final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    explicit constexpr U32StringReadTools(const U32StringDataView &data) noexcept : _data{data} {}

public: // raw accessors
    /// Access the string data.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char32_t> { return _data.data(); }
    /// Access the UTF-32 code-unit range for this view.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::CpRange { return _data.range(); }

public: // tests
    /// Test if the view is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.data().empty() || _data.range().isEmpty(); }
    /// Test if the view contains valid UTF-32.
    [[nodiscard]] auto isValidUtf32() const noexcept -> bool;

public: // find
    /// Find the first decoded character from the given character set.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the first decoded character from the given character set at or after the given UTF-32 code-unit index.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Find the first decoded character not from the given character set.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the first decoded character not from the given character set at or after the given UTF-32 data index.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::CpIndex start) const noexcept -> unit::CpIndex;
    /// Find the last decoded character from the given character set.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the last decoded character from the given character set before the given end UTF-32 data index.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;
    /// Find the last decoded character not from the given character set.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::CpIndex;
    /// Find the last decoded character not from the given character set before the given end UTF-32 data index.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::CpIndex end) const noexcept -> unit::CpIndex;

public: // read
    /// Get the UTF-32 code-unit length of the view.
    [[nodiscard]] auto length() const noexcept -> unit::CpLength;
    /// Get the approximate display width of the decoded text.
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Access the character at the given start UTF-32 code-unit position.
    [[nodiscard]] auto charAt(unit::CpIndex startIndex) const noexcept -> Char;
    /// Read the character at the given index and advance the index.
    /// If the index is out of bounds, returns a signal character and does not advance the index.
    [[nodiscard]] auto read(unit::CpIndex &index) const noexcept -> Char;
    /// Read the character before the given index and retreat the index.
    /// If the index is out of bounds, returns a signal character and does not retreat the index.
    [[nodiscard]] auto readAndRetreat(unit::CpIndex &index) const noexcept -> Char;
    /// Access the character at the given start UTF-32 code-unit position or throw if no valid character is found.
    [[nodiscard]] auto charAtOrThrow(unit::CpIndex startIndex) const -> Char;
    /// Read the character at the given index and advance the index.
    /// Throw on encoding errors or out-of-bound reads.
    [[nodiscard]] auto readOrThrow(unit::CpIndex &index) const -> Char;
    /// Advance the given UTF-32 code-unit index to the start of the next character.
    auto advance(unit::CpIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// Retreat the given UTF-32 code-unit index to the start of the previous character.
    auto retreat(unit::CpIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// Convert a relative UTF-32 code-unit range for this view into a bounded range for the backing data.
    [[nodiscard]] auto sliceRange(unit::CpRange range) const noexcept -> unit::CpRange;

public: // conversion
    /// Create a copy as `std::string`.
    [[nodiscard]] auto toStdString() const noexcept -> std::string;
    /// Create a copy as `std::u8string`.
    [[nodiscard]] auto toStdU8String() const noexcept -> std::u8string;
    /// Create a copy as `std::u16string`.
    [[nodiscard]] auto toStdU16String() const noexcept -> std::u16string;
    /// Create a copy as `std::u32string`.
    [[nodiscard]] auto toStdU32String() const noexcept -> std::u32string;
    /// Create a copy as `std::wstring`.
    [[nodiscard]] auto toStdWString() const noexcept -> std::wstring;

private:
    /// Find the first decoded character that matches or does not match the character set.
    [[nodiscard]] auto findFirstOfCharacterSet(
        const CharacterSet &characters, unit::CpIndex start, bool isMatching) const noexcept -> unit::CpIndex;
    /// Find the last decoded character that matches or does not match the character set.
    [[nodiscard]] auto findLastOfCharacterSet(
        const CharacterSet &characters, unit::CpIndex end, bool isMatching) const noexcept -> unit::CpIndex;

private:
    U32StringDataView _data;
};

}
