// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"

#include "../../unit/ByteCount.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CpCount.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpRange.hpp"

#include <cstdint>
#include <utility>

namespace erbsland::text::cpp {

/// A lexical C++ token retaining shared ownership of its unmodified source slice.
/// @seedoc{/reference/text/cpp_tokenizer}
/// @tested{CppTokenizerTest}
class CppToken final {
public:
    /// The lexical categories recognized by the source scanner.
    enum class Kind : uint8_t {
        End,         ///< End of input; the text is empty.
        Identifier,  ///< An identifier, including keywords.
        Number,      ///< A numeric token, without validating its value or suffix.
        Punctuation, ///< An operator, delimiter, or otherwise unclassified character.
        Literal,     ///< A quoted character, string, or raw string literal.
        Comment,     ///< A line or block comment, including its delimiters.
        Directive,   ///< A complete preprocessing directive, including line continuations.
    };

public:
    /// Create an end token without a source position.
    CppToken() = default;
    /// Create a token from its category and source slice.
    /// @param kind The lexical category.
    /// @param text The unmodified source slice, including delimiters.
    /// @param range The UTF-8 byte range corresponding to the source slice.
    /// @param characterRange The decoded code-point range corresponding to the same source slice.
    CppToken(Kind kind, String text, unit::ByteRange range, unit::CpRange characterRange) :
        _kind{kind}, _text{std::move(text)}, _range{range}, _characterRange{characterRange} {}

public: // accessors
    /// Test whether this token marks the end of input.
    [[nodiscard]] auto isEnd() const noexcept -> bool { return _kind == Kind::End; }
    /// Get the lexical category.
    [[nodiscard]] auto kind() const noexcept -> Kind { return _kind; }
    /// Get the source slice; its storage remains valid independently of the tokenizer.
    [[nodiscard]] auto text() const noexcept -> const String & { return _text; }
    /// Get the UTF-8 byte length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteCount { return _range.length(); }
    /// Get the UTF-8 byte offset, or no-index for a default-constructed token.
    [[nodiscard]] auto position() const noexcept -> unit::ByteIndex { return _range.beginIndex(); }
    /// Get the UTF-8 byte range of the original source slice.
    /// A tokenizer end token has an empty range at end of input; a default token has no-range.
    [[nodiscard]] auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Get the precomputed decoded code-point length in constant time.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpCount { return _characterRange.length(); }
    /// Get the decoded code-point offset, or no-index for a default-constructed token.
    [[nodiscard]] auto characterPosition() const noexcept -> unit::CpIndex { return _characterRange.beginIndex(); }
    /// Get the decoded code-point range, including its precomputed length.
    /// A tokenizer end token has an empty range at end of input; a default token has no-range.
    [[nodiscard]] auto characterRange() const noexcept -> unit::CpRange { return _characterRange; }

private:
    Kind _kind = Kind::End;                                   ///< Lexical category.
    String _text;                                             ///< Shared source slice.
    unit::ByteRange _range = unit::ByteRange::noRange();      ///< Native range in the UTF-8 source.
    unit::CpRange _characterRange = unit::CpRange::noRange(); ///< Decoded range, retained without recounting.
};

}
