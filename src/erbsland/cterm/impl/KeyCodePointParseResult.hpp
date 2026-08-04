// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyParseStatus.hpp"

#include "../../text/Char.hpp"
#include "../../unit/ByteIndex.hpp"

namespace erbsland::cterm::impl {

/// Store the outcome of parsing one UTF-8 code point prefix.
class KeyCodePointParseResult final {
public:
    /// Create an invalid or incomplete character parse result.
    KeyCodePointParseResult(
        const KeyParseStatus status = KeyParseStatus::Invalid,
        const unit::ByteIndex consumedByteCount = unit::ByteIndex{}) noexcept :
        _status{status}, _consumedByteCount{consumedByteCount} {}

    /// Create a parsed character result.
    KeyCodePointParseResult(
        const KeyParseStatus status, const text::Char character, const unit::ByteIndex consumedByteCount) noexcept :
        _status{status}, _character{character}, _consumedByteCount{consumedByteCount} {}

    /// Access the parse status.
    [[nodiscard]] auto status() const noexcept -> KeyParseStatus { return _status; }
    /// Access the parsed character.
    [[nodiscard]] auto character() const noexcept -> text::Char { return _character; }
    /// Access the consumed byte count.
    [[nodiscard]] auto consumedByteCount() const noexcept -> unit::ByteIndex { return _consumedByteCount; }

private:
    KeyParseStatus _status{KeyParseStatus::Invalid}; ///< The parsing status.
    text::Char _character{};                         ///< The parsed character for `Parsed`.
    unit::ByteIndex _consumedByteCount{};            ///< Number of bytes consumed.
};

}
