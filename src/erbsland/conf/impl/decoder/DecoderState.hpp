// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Char.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/CodeLocation.hpp"

namespace erbsland::conf::impl {

/// A lightweight checkpoint for restoring a decoder after speculative parsing.
/// The two byte indexes delimit the current character and the next unread character in the decoder's line buffer.
/// @tested{FastNameDecoderTest TokenDecoderTest}
class DecoderState final {
public:
    /// Create an empty decoder checkpoint.
    constexpr DecoderState() noexcept = default;
    /// Create a decoder checkpoint.
    constexpr DecoderState(
        text::Char character,
        unit::ByteIndex characterIndex,
        unit::ByteIndex nextByteIndex,
        unit::CodeLocation location) noexcept :
        _character{character}, _characterIndex{characterIndex}, _nextByteIndex{nextByteIndex}, _location{location} {}

public:
    /// Access the current character.
    [[nodiscard]] constexpr auto character() const noexcept -> text::Char { return _character; }
    /// Access the byte index where the current character starts.
    [[nodiscard]] constexpr auto characterIndex() const noexcept -> unit::ByteIndex { return _characterIndex; }
    /// Access the byte index of the next unread character.
    [[nodiscard]] constexpr auto nextByteIndex() const noexcept -> unit::ByteIndex { return _nextByteIndex; }
    /// Access the code location of the current character.
    [[nodiscard]] constexpr auto location() const noexcept -> unit::CodeLocation { return _location; }

private:
    text::Char _character{text::Char::endOfData()}; ///< The current decoded character.
    unit::ByteIndex _characterIndex;                ///< The start of the current character.
    unit::ByteIndex _nextByteIndex;                 ///< The next unread byte in the current line.
    unit::CodeLocation _location;                   ///< The current source location.
};

}
