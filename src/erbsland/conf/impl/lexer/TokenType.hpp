// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../char/NamedChars.hpp"

#include "../../../text/String.hpp"

#include <cstdint>

namespace erbsland::conf::impl {

/// The type of token.
class TokenType final {
public:
    /// Enum with all types.
    enum Value : uint8_t {
        EndOfData,                   ///< The end of the data.               (NoContent)
        LineBreak,                   ///< A line-break.                      (NoContent)
        Spacing,                     ///< A block of spacing.                (NoContent)
        Indentation,                 ///< A block with indentation.          (NoContent)
        Comment,                     ///< A comment.                         (NoContent)
        RegularName,                 ///< A regular name                     (text::String)
        TextName,                    ///< A text name                        (text::String) *without* double quotes
        MetaName,                    ///< A metaname                         (text::String) including leading @
        NameValueSeparator,          ///< A value separator `:`              (NoContent)
        ValueListSeparator,          ///< A value list separator `,`         (NoContent)
        MultiLineValueListSeparator, ///< A multi-line value separator `*`   (NoContent)
        NamePathSeparator,           ///< A name path separator. `.`         (NoContent)
        Integer,                     ///< An integer literal                 (Integer)
        Boolean,                     ///< A boolean literal                  (bool)
        Float,                       ///< A floating point literal           (Float)
        Text,                        ///< A single line text                 (text::String)
        MultiLineTextOpen,           ///< The start of a multi-line text.    (NoContent)
        MultiLineTextClose,          ///< The end of a multi-line text.      (NoContent)
        MultiLineText,               ///< A line of multi-line text.         (text::String, no linebreak)
        Code,                        ///< A single line code.                (text::String)
        MultiLineCodeOpen,           ///< The start of multi-line code.      (NoContent)
        MultiLineCodeLanguage,       ///< The language identifier.           (text::String) = language name.
        MultiLineCodeClose,          ///< The end of multi-line code.        (NoContent)
        MultiLineCode,               ///< A line of multi-line code.         (text::String, no linebreak)
        RegEx,                       ///< A single line regex.               (text::String)
        MultiLineRegexOpen,          ///< The start of multi-line regex.     (NoContent)
        MultiLineRegexClose,         ///< The end of multi-line regex.       (NoContent)
        MultiLineRegex,              ///< A line of multi-line regex.        (text::String, no linebreak, no comment)
        Bytes,                       ///< A single line block of bytes.      (Bytes)
        MultiLineBytesOpen,          ///< The start of multi-line bytes.     (NoContent)
        MultiLineBytesFormat,        ///< The format multi-line bytes.       (NoContent)
        MultiLineBytesClose,         ///< The end of multi-line bytes.       (NoContent)
        MultiLineBytes,              ///< A line of multi-line bytes         (Bytes)
        Date,                        ///< A date                             (Date)
        Time,                        ///< A time                             (Time)
        DateTime,                    ///< A date/time                        (DateTime)
        TimeDelta,                   ///< A time-delta                       (TimeDelta)
        SectionMapOpen,              ///< The start of a section map.        (NoContent)
        SectionMapClose,             ///< The end of a section map.          (NoContent)
        SectionListOpen,             ///< The start of a section list block. (NoContent)
        SectionListClose,            ///< The end of a section list block.   (NoContent)
        Error,                       ///< Error block, for relaxed lexing.   (text::String) = error message.
    };

public:
    /// Create an error token type.
    TokenType() = default;
    /// Create a token type from a given value.
    TokenType(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                      // operators
    /// Test if two token types are equal.
    constexpr auto operator==(const TokenType &other) const noexcept -> bool { return _value == other._value; }
    /// Test if this token type equals a raw value.
    constexpr auto operator==(const Value other) const noexcept -> bool { return _value == other; }
    /// Test if two token types differ.
    constexpr auto operator!=(const TokenType &other) const noexcept -> bool { return _value != other._value; }
    /// Test if this token type differs from a raw value.
    constexpr auto operator!=(const Value other) const noexcept -> bool { return _value != other; }

public: // accessors
    /// Get the raw token-type value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Value { return _value; }

public: // helper
    /// Convert a multi-line opening delimiter to its token type.
    [[nodiscard]] constexpr static auto fromMultiLineOpen(const text::Char character) noexcept -> TokenType {
        switch (character.toRawValue()) {
        case nc::doubleQuote.toRawValue():
            return MultiLineTextOpen;
        case nc::backtick.toRawValue():
            return MultiLineCodeOpen;
        case nc::slash.toRawValue():
            return MultiLineRegexOpen;
        case nc::lessThan.toRawValue():
            return MultiLineBytesOpen;
        default:
            return EndOfData;
        }
    }

    /// Convert a multi-line closing delimiter to its token type.
    [[nodiscard]] constexpr static auto fromMultiLineClose(const text::Char character) noexcept -> TokenType {
        switch (character.toRawValue()) {
        case nc::doubleQuote.toRawValue():
            return MultiLineTextClose;
        case nc::backtick.toRawValue():
            return MultiLineCodeClose;
        case nc::slash.toRawValue():
            return MultiLineRegexClose;
        case nc::greaterThan.toRawValue():
            return MultiLineBytesClose;
        default:
            return EndOfData;
        }
    }

private:
    Value _value{Error};
};

/// Convert a token type into its display name.
[[nodiscard]] inline auto toString(const TokenType tokenType) noexcept -> text::String {
    using namespace text::literals;
    switch (tokenType.raw()) {
    case TokenType::EndOfData:
        return "EndOfData"_el;
    case TokenType::LineBreak:
        return "LineBreak"_el;
    case TokenType::Spacing:
        return "Spacing"_el;
    case TokenType::Indentation:
        return "Indentation"_el;
    case TokenType::Comment:
        return "Comment"_el;
    case TokenType::RegularName:
        return "RegularName"_el;
    case TokenType::TextName:
        return "TextName"_el;
    case TokenType::MetaName:
        return "MetaName"_el;
    case TokenType::NameValueSeparator:
        return "NameValueSeparator"_el;
    case TokenType::ValueListSeparator:
        return "ValueListSeparator"_el;
    case TokenType::MultiLineValueListSeparator:
        return "MultiLineValueListSeparator"_el;
    case TokenType::NamePathSeparator:
        return "NamePathSeparator"_el;
    case TokenType::Integer:
        return "Integer"_el;
    case TokenType::Boolean:
        return "Boolean"_el;
    case TokenType::Float:
        return "Float"_el;
    case TokenType::Text:
        return "Text"_el;
    case TokenType::MultiLineTextOpen:
        return "MultiLineTextOpen"_el;
    case TokenType::MultiLineTextClose:
        return "MultiLineTextClose"_el;
    case TokenType::MultiLineText:
        return "MultiLineText"_el;
    case TokenType::Code:
        return "Code"_el;
    case TokenType::MultiLineCodeOpen:
        return "MultiLineCodeOpen"_el;
    case TokenType::MultiLineCodeLanguage:
        return "MultiLineCodeLanguage"_el;
    case TokenType::MultiLineCodeClose:
        return "MultiLineCodeClose"_el;
    case TokenType::MultiLineCode:
        return "MultiLineCode"_el;
    case TokenType::RegEx:
        return "RegEx"_el;
    case TokenType::MultiLineRegexOpen:
        return "MultiLineRegexOpen"_el;
    case TokenType::MultiLineRegexClose:
        return "MultiLineRegexClose"_el;
    case TokenType::MultiLineRegex:
        return "MultiLineRegex"_el;
    case TokenType::Bytes:
        return "Bytes"_el;
    case TokenType::MultiLineBytesOpen:
        return "MultiLineBytesOpen"_el;
    case TokenType::MultiLineBytesFormat:
        return "MultiLineBytesFormat"_el;
    case TokenType::MultiLineBytesClose:
        return "MultiLineBytesClose"_el;
    case TokenType::MultiLineBytes:
        return "MultiLineBytes"_el;
    case TokenType::Date:
        return "Date"_el;
    case TokenType::Time:
        return "Time"_el;
    case TokenType::DateTime:
        return "DateTime"_el;
    case TokenType::TimeDelta:
        return "TimeDelta"_el;
    case TokenType::SectionMapOpen:
        return "SectionMapOpen"_el;
    case TokenType::SectionMapClose:
        return "SectionMapClose"_el;
    case TokenType::SectionListOpen:
        return "SectionListOpen"_el;
    case TokenType::SectionListClose:
        return "SectionListClose"_el;
    case TokenType::Error:
        return "Error"_el;
    }
    return {};
}

}
