// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"
#include "U8StringDataView.hpp"

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../Char.hpp"
#include "../../CharSet.hpp"

#include <string>
#include <string_view>

namespace erbsland::text::impl {

/// A view into string data with a specific byte range.
/// This helper class provides all read-only algorithms on the string data, shared by `U8String` and `U8StringView`.
/// It's meant to be used inline like `U8StringDataView{dataView()}.doSomething()`.
/// @tested{U8StringReadToolsTest}
class U8StringReadTools final {
public:
    /// A set of Unicode code-points for decoded character lookups.
    using CharacterSet = CharSet;

public:
    explicit constexpr U8StringReadTools(const U8StringDataView &data) noexcept : _data{data} {}

public: // raw accessors
    /// Access the string data.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char> { return _data.data(); }
    /// Access the byte range for this view.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::ByteRange { return _data.range(); }

public: // tests
    /// Test if the view is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.data().empty() || _data.range().isEmpty(); }
    /// Test if the view contains valid UTF-8.
    [[nodiscard]] auto isValidUtf8() const noexcept -> bool;

public: // find
    /// Find the first decoded character from the given character set.
    [[nodiscard]] auto findFirstOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the first decoded character from the given character set at or after the given byte index.
    [[nodiscard]] auto findFirstOf(const CharSet &characters, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// Find the first decoded character not from the given character set.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the first decoded character not from the given character set at or after the given byte index.
    [[nodiscard]] auto findFirstNotOf(const CharSet &characters, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// Find the last decoded character from the given character set.
    [[nodiscard]] auto findLastOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character from the given character set before the given end byte index.
    [[nodiscard]] auto findLastOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character not from the given character set.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character not from the given character set before the given end byte index.
    [[nodiscard]] auto findLastNotOf(const CharSet &characters, unit::ByteIndex end) const noexcept -> unit::ByteIndex;

public: // read
    /// Get the byte length of the view.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength;
    /// Access the character at the given start byte position.
    [[nodiscard]] auto charAt(unit::ByteIndex startIndex) const noexcept -> Char;
    /// Read the character at the given index and advance the index.
    /// If the index is out of bounds, returns a signal character and does not advance the index.
    [[nodiscard]] auto read(unit::ByteIndex &index) const noexcept -> Char;
    /// Access the character at the given start byte position or throw if no valid character is found.
    [[nodiscard]] auto charAtOrThrow(unit::ByteIndex startIndex) const -> Char;
    /// Read the character at the given index and advance the index.
    /// Throw on encoding errors or out-of-bound reads.
    [[nodiscard]] auto readOrThrow(unit::ByteIndex &index) const -> Char;
    /// Advance the given byte index to the start of the next character.
    auto advance(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// Retreat the given byte index to the start of the previous character.
    auto retreat(unit::ByteIndex &index, unit::CpLength count = unit::CpLength::one()) const noexcept -> bool;
    /// Convert a relative byte range for this view into a bounded byte range for the backing data.
    [[nodiscard]] auto sliceRange(unit::ByteRange range) const noexcept -> unit::ByteRange;

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
        const CharacterSet &characters, unit::ByteIndex start, bool isMatching) const noexcept -> unit::ByteIndex;
    /// Find the last decoded character that matches or does not match the character set.
    [[nodiscard]] auto findLastOfCharacterSet(
        const CharacterSet &characters, unit::ByteIndex end, bool isMatching) const noexcept -> unit::ByteIndex;

private:
    U8StringDataView _data;
};

}
