// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Token_fwd.hpp"
#include "TokenKind.hpp"

#include "impl/Reader_fwd.hpp"

#include "../../text/String.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CodeLocation.hpp"

namespace erbsland::conf::syntax {

/// An owning, unmodified source slice with its lexical classification.
/// @tested{SyntaxParserTest}
class Token final {
    friend class impl::Reader;

public: // accessors
    /// Get the lexical classification.
    [[nodiscard]] auto kind() const noexcept -> TokenKind { return _kind; }
    /// Get the source text, including delimiters.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get the half-open original byte range.
    [[nodiscard]] auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Get the original starting location.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _location; }
    /// Get decoded text for names and textual literals; empty for other tokens.
    [[nodiscard]] auto decodedText() const noexcept -> const text::String & { return _decodedText; }

private:
    TokenKind _kind{TokenKind::EndOfData}; ///< Classification.
    text::String _text;                    ///< Original source slice.
    text::String _decodedText;             ///< Decoded textual contents.
    unit::ByteRange _range;                ///< Original byte range.
    unit::CodeLocation _location;          ///< Original source location.
};

}
