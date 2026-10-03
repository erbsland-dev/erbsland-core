// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16Encoding.hpp"
#include "U16StringDataView.hpp"

#include "../../../unit/CpCount.hpp"
#include "../../../unit/U16Count.hpp"
#include "../../../unit/U16Index.hpp"
#include "../../../unit/U16Range.hpp"
#include "../../Char.hpp"
#include "../../CharSet.hpp"

#include <string>
#include <string_view>

namespace erbsland::text::impl {

/// A view into string data with a specific byte range.
/// This helper class provides all read-only algorithms on the string data, shared by `U16StringEditor` and `U16String`.
/// It's meant to be used inline like `U16StringDataView{dataView()}.doSomething()`.
/// @tested{U16StringTest StringSeparatorSplitTest StringEncodingValidationTest}
class U16StringReadTools final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    /// Create read tools for `data`.
    explicit constexpr U16StringReadTools(const U16StringDataView &data) noexcept : _data{data} {}

public: // raw accessors
    /// Access the string data.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char16_t> { return _data.data(); }
    /// Access the UTF-16 code-unit range for this view.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::U16Range { return _data.range(); }

public: // tests
    /// Test if the view is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.data().empty() || _data.range().isEmpty(); }
    /// Test if the view contains valid UTF-16.
    [[nodiscard]] auto isEncodingValid() const noexcept -> bool;
    /// Validate the selected span, reporting errors relative to its start.
    /// @throws U16EncodingError If malformed UTF-16 is detected.
    void encodingValidOrThrow() const;

public: // find
    /// Find the first decoded character from the given character set.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// Find the first decoded character from the given character set at or after the given UTF-16 code-unit index.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::U16Index start) const noexcept -> unit::U16Index;
    /// Find the first decoded character not from the given character set.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// Find the first decoded character not from the given character set at or after the given UTF-16 data index.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::U16Index start) const noexcept -> unit::U16Index;
    /// Find the last decoded character from the given character set.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// Find the last decoded character from the given character set before the given end UTF-16 data index.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::U16Index end) const noexcept -> unit::U16Index;
    /// Find the last decoded character not from the given character set.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::U16Index;
    /// Find the last decoded character not from the given character set before the given end UTF-16 data index.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::U16Index end) const noexcept -> unit::U16Index;

    /// Find the native range of the first matching decoded character.
    /// @param characters The separator set.
    /// @return The relative match range, or no-range if the set is empty or no character matches.
    [[nodiscard]] auto findFirstRange(const CharSet &characters) const noexcept -> unit::U16Range;
    /// Find the native range of the last matching decoded character.
    /// @param characters The separator set.
    /// @return The relative match range, or no-range if the set is empty or no character matches.
    [[nodiscard]] auto findLastRange(const CharSet &characters) const noexcept -> unit::U16Range;

public: // read
    /// Get the UTF-16 code-unit length of the view.
    [[nodiscard]] auto byteLength() const noexcept -> unit::U16Count;
    /// Get the approximate display width of the decoded text.
    [[nodiscard]] auto displayWidth() const noexcept -> int;
    /// Access the character at the given start UTF-16 code-unit position.
    [[nodiscard]] auto charAt(unit::U16Index startIndex) const noexcept -> Char;
    /// Read the character at the given index and advance the index.
    /// If the index is out of bounds, returns a signal character and does not advance the index.
    [[nodiscard]] auto read(unit::U16Index &index) const noexcept -> Char;
    /// Read the character before the given index and retreat the index.
    /// If the index is out of bounds, returns a signal character and does not retreat the index.
    [[nodiscard]] auto readAndRetreat(unit::U16Index &index) const noexcept -> Char;
    /// Advance the given UTF-16 code-unit index to the start of the next character.
    auto advance(unit::U16Index &index, unit::CpCount count = unit::CpCount::one()) const noexcept -> bool;
    /// Retreat the given UTF-16 code-unit index to the start of the previous character.
    auto retreat(unit::U16Index &index, unit::CpCount count = unit::CpCount::one()) const noexcept -> bool;
    /// Convert a relative UTF-16 code-unit range for this view into a bounded range for the backing data.
    [[nodiscard]] auto sliceRange(unit::U16Range range) const noexcept -> unit::U16Range;

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
        const CharacterSet &characters, unit::U16Index start, bool isMatching) const noexcept -> unit::U16Range;
    /// Find the last decoded character that matches or does not match the character set.
    [[nodiscard]] auto findLastOfCharacterSet(
        const CharacterSet &characters, unit::U16Index end, bool isMatching) const noexcept -> unit::U16Range;

private:
    U16StringDataView _data;
};

}
