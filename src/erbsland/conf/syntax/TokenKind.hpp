// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ValueType.hpp"

#include "../../text/Char.hpp"
#include "../../text/String.hpp"

#include <cstdint>

namespace erbsland::conf::syntax {

/// Source token classification, independent of decoded values.
/// @tested{SyntaxTokenKindTest TokenTypeTest}
class TokenKind final {
public:
    /// The lexical token classifications.
    enum Value : std::uint8_t {
        EndOfData,                   ///< The end of the data.
        LineBreak,                   ///< A line-break.
        Spacing,                     ///< A block of spacing.
        Indentation,                 ///< A block with indentation.
        Comment,                     ///< A comment.
        RegularName,                 ///< A regular name
        TextName,                    ///< A text name
        MetaName,                    ///< A metaname
        NameValueSeparator,          ///< A value separator `:`
        ValueListSeparator,          ///< A value list separator `,`
        MultiLineValueListSeparator, ///< A multi-line value separator `*`
        NamePathSeparator,           ///< A name path separator. `.`
        Integer,                     ///< An integer literal
        Boolean,                     ///< A boolean literal
        Float,                       ///< A floating point literal
        Text,                        ///< A single line text
        MultiLineTextOpen,           ///< The start of a multi-line text.
        MultiLineTextClose,          ///< The end of a multi-line text.
        MultiLineText,               ///< A line of multi-line text.
        Code,                        ///< A single line code.
        MultiLineCodeOpen,           ///< The start of multi-line code.
        MultiLineCodeLanguage,       ///< The language identifier.
        MultiLineCodeClose,          ///< The end of multi-line code.
        MultiLineCode,               ///< A line of multi-line code.
        RegEx,                       ///< A single line regex.
        MultiLineRegexOpen,          ///< The start of multi-line regex.
        MultiLineRegexClose,         ///< The end of multi-line regex.
        MultiLineRegex,              ///< A line of multi-line regex.
        Bytes,                       ///< A single line block of bytes.
        MultiLineBytesOpen,          ///< The start of multi-line bytes.
        MultiLineBytesFormat,        ///< The format multi-line bytes.
        MultiLineBytesClose,         ///< The end of multi-line bytes.
        MultiLineBytes,              ///< A line of multi-line bytes
        Date,                        ///< A date
        Time,                        ///< A time
        DateTime,                    ///< A date/time
        TimeDelta,                   ///< A time-delta
        SectionMapOpen,              ///< The start of a section map.
        SectionMapClose,             ///< The end of a section map.
        SectionListOpen,             ///< The start of a section list block.
        SectionListClose,            ///< The end of a section list block.
        Error,                       ///< Unrecognized source retained after a line error.
        SectionName,                 ///< A regular name in a section declaration.
        SectionTextName,             ///< A quoted name in a section declaration.
    };

public:
    /// Create an error classification.
    constexpr TokenKind() noexcept = default;
    /// Create a token classification from its raw value.
    /// @param value The lexical classification.
    constexpr TokenKind(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:
    constexpr auto operator==(const TokenKind &other) const noexcept -> bool { return _value == other._value; }
    constexpr auto operator==(const Value other) const noexcept -> bool { return _value == other; }

public: // tests
    /// Test whether this token is a complete single-line literal.
    [[nodiscard]] auto isValue() const noexcept -> bool;
    /// Test whether this token contains multiline literal payload.
    [[nodiscard]] auto isPayload() const noexcept -> bool;

public: // accessors
    /// Get the raw lexical classification.
    [[nodiscard]] constexpr auto raw() const noexcept -> Value { return _value; }

public: // conversion
    /// Map a complete literal or multiline opening delimiter to its value category.
    /// @return The value type, or `ValueType::Undefined` for all other tokens, including multiline payload.
    [[nodiscard]] auto toValueType() const noexcept -> ValueType;
    /// Convert this classification to its standardized diagnostic name.
    /// @return The token name, or empty text for an unknown raw value.
    [[nodiscard]] auto toText() const noexcept -> text::String;

public: // factories
    /// Convert a multi-line opening delimiter character to its classification.
    /// @param character The repeated delimiter character (`"`, backtick, `/` or `<`).
    /// @return The opening token kind, or `EndOfData` for an unrecognized character.
    [[nodiscard]] constexpr static auto fromMultiLineOpen(const text::Char character) noexcept -> TokenKind {
        switch (character.toRawValue()) {
        case U'"':
            return MultiLineTextOpen;
        case U'`':
            return MultiLineCodeOpen;
        case U'/':
            return MultiLineRegexOpen;
        case U'<':
            return MultiLineBytesOpen;
        default:
            return EndOfData;
        }
    }

    /// Convert a multi-line closing delimiter character to its classification.
    /// @param character The repeated delimiter character (`"`, backtick, `/` or `>`).
    /// @return The closing token kind, or `EndOfData` for an unrecognized character.
    [[nodiscard]] constexpr static auto fromMultiLineClose(const text::Char character) noexcept -> TokenKind {
        switch (character.toRawValue()) {
        case U'"':
            return MultiLineTextClose;
        case U'`':
            return MultiLineCodeClose;
        case U'/':
            return MultiLineRegexClose;
        case U'>':
            return MultiLineBytesClose;
        default:
            return EndOfData;
        }
    }

private:
    Value _value{Error}; ///< The lexical classification.
};

}
