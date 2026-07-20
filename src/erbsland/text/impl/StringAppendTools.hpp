// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char_fwd.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u8/U8String_fwd.hpp"

#include "../../unit/CpLength_fwd.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Width-agnostic interface for low-level appends into string storage.
/// Each operation returns the number of code points appended to the underlying storage.
/// @tested{ByteFormatTest U8StringModifierTest U16StringTest U32StringTest}
class StringAppendTools {
public:
    // defaults
    virtual ~StringAppendTools() = default;

public:
    /// Append a single character.
    /// @param character The character to append.
    /// @return The number of code points appended.
    virtual auto append(Char character) -> unit::CpLength = 0;
    /// Append text from the given string.
    /// @param text The text to append.
    /// @return The number of code points appended.
    virtual auto append(const U8String &text) -> unit::CpLength = 0;
    /// @overload
    virtual auto append(const U16String &text) -> unit::CpLength = 0;
    /// @overload
    virtual auto append(const U32String &text) -> unit::CpLength = 0;

protected:
    /// Count decoded characters in UTF-8 data, treating invalid sequences as replacement characters.
    [[nodiscard]] static auto countDecodedCharacters(std::span<const char> source) noexcept -> unit::CpLength;
    /// Count decoded characters in UTF-16 data, treating invalid sequences as replacement characters.
    [[nodiscard]] static auto countDecodedCharacters(std::span<const char16_t> source) noexcept -> unit::CpLength;
    /// Count decoded characters in UTF-32 data, treating invalid code points as replacement characters.
    [[nodiscard]] static auto countDecodedCharacters(std::span<const char32_t> source) noexcept -> unit::CpLength;
    /// Calculate the decoded length of repeated text.
    /// @throws OverflowError if the result exceeds the finite code-point length range.
    [[nodiscard]] static auto repeatedCharacterCount(unit::CpLength characterCount, std::size_t count)
        -> unit::CpLength;
};

}
