// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NamedChars.hpp"

#include "../../../text/AsciiCategory.hpp"

#include <cstdint>

namespace erbsland::conf::impl {

/// Character classes to simplify lexer tests.
enum class CharClass : uint8_t {
    Spacing,                  ///< Space or tab.
    LineBreak,                ///< Carriage return or newline.
    NameStart,                ///< Start of a name (a-z, @ or double quote).
    Letter,                   ///< A letter (a-z).
    LetterOrDigit,            ///< Either a letter (a-z) or digit (0-9).
    DecimalDigit,             ///< A decimal digit (0-9).
    HexDigit,                 ///< A hexadecimal digit (0-9, a-f, A-F).
    NameValueSeparator,       ///< A value separator (either : or =).
    OpeningBracket,           ///< Any opening bracket of the language (", `, /, <).
    SectionStart,             ///< Any character that may start a section (- *, [).
    EndOfLineStart,           ///< Either a space, hash (comment) or line-break character.
    LetterA,                  ///< The letter A, case-insensitive
    LetterB,                  ///< The letter B, case-insensitive
    LetterF,                  ///< The letter F, case-insensitive
    LetterI,                  ///< The letter I, case-insensitive
    LetterN,                  ///< The letter N, case-insensitive
    LetterT,                  ///< The letter T, case-insensitive
    LetterX,                  ///< The letter X, case-insensitive
    LetterZ,                  ///< The letter Z, case-insensitive
    NumberStart,              ///< Digit `0`-`9` or `+`/`-`
    TimeStart,                ///< Digit `0`-`9` or `t`/`T`
    FloatLiteralStart,        ///< `+`, `-`, `i`/`I` or `n`/`N`
    ExponentStart,            ///< The letter `e`/`E`
    BinaryDigit,              ///< Digit `0` or `1`
    PlusOrMinus,              ///< `+` or `-`
    SectionNameStart,         ///< `a-z`, `A-Z` or `"`
    FormatIdentifierChar,     ///< `a-z`, `A-Z`, `0-9`, `_` or `-`
    IntegerSuffixChar,        ///< `a-z`, `A-Z`, or `µ`
    LineBreakOrEnd,           ///< new-line, carriage-return or end-of-data
    ValidAfterValue,          ///< tab, space, new-line, carriage-return, `#`, end-of-data, `,`
    ValidLang,                ///< All valid characters for a configuration document.
    FilePathSeparator,        ///< `/` or `\`
    InvalidWindowsServerName, ///< All invalid characters for a Windows server name in a UNC path.
};

/// Test if a character is part of the given configuration-language character class.
/// @param character The character to test.
/// @param charClass The character class.
/// @return `true` if the character is part of the class.
[[nodiscard]] constexpr auto operator==(const text::Char character, const CharClass charClass) noexcept -> bool {
    switch (charClass) {
    case CharClass::Spacing:
        return character.isAsciiBlank();
    case CharClass::LineBreak:
        return character == nc::newLine || character == nc::carriageReturn;
    case CharClass::NameStart:
        return character.isAsciiLetter() || character == nc::at || character == nc::doubleQuote;
    case CharClass::Letter:
        return character.isAsciiLetter();
    case CharClass::LetterOrDigit:
        return character.isAsciiAlphanumeric();
    case CharClass::DecimalDigit:
        return character.isAsciiDigit();
    case CharClass::HexDigit:
        return character.isAsciiHexDigit();
    case CharClass::NameValueSeparator:
        return character == nc::colon || character == nc::equal;
    case CharClass::OpeningBracket:
        return character == nc::doubleQuote || character == nc::backtick || character == nc::slash ||
            character == nc::lessThan;
    case CharClass::SectionStart:
        return character == nc::minus || character == nc::asterisk || character == nc::openingSquareBracket;
    case CharClass::EndOfLineStart:
        return character.isAsciiBlank() || character == nc::newLine || character == nc::carriageReturn ||
            character == nc::commentStart || character.isEndOfData();
    case CharClass::LetterA:
        return character == nc::lowercaseA || character == nc::uppercaseA;
    case CharClass::LetterB:
        return character == nc::lowercaseB || character == nc::uppercaseB;
    case CharClass::LetterF:
        return character == nc::lowercaseF || character == nc::uppercaseF;
    case CharClass::LetterI:
        return character == nc::lowercaseI || character == nc::uppercaseI;
    case CharClass::LetterN:
        return character == nc::lowercaseN || character == nc::uppercaseN;
    case CharClass::LetterT:
        return character == nc::lowercaseT || character == nc::uppercaseT;
    case CharClass::LetterX:
        return character == nc::lowercaseX || character == nc::uppercaseX;
    case CharClass::LetterZ:
        return character == nc::lowercaseZ || character == nc::uppercaseZ;
    case CharClass::NumberStart:
        return character.isAsciiDigit() || character == nc::plus || character == nc::minus;
    case CharClass::TimeStart:
        return character.isAsciiDigit() || character == nc::lowercaseT || character == nc::uppercaseT;
    case CharClass::FloatLiteralStart:
        return character == nc::plus || character == nc::minus || character == nc::lowercaseN ||
            character == nc::uppercaseN || character == nc::lowercaseI || character == nc::uppercaseI;
    case CharClass::ExponentStart:
        return character == nc::lowercaseE || character == nc::uppercaseE;
    case CharClass::BinaryDigit:
        return character == nc::digit0 || character == nc::digit1;
    case CharClass::PlusOrMinus:
        return character == nc::plus || character == nc::minus;
    case CharClass::SectionNameStart:
        return character.isAsciiLetter() || character == nc::doubleQuote;
    case CharClass::FormatIdentifierChar:
        return character.isAsciiCategory(text::AsciiCategory::WordWithHyphen);
    case CharClass::IntegerSuffixChar:
        return character.isAsciiLetter() || character == nc::microSign;
    case CharClass::LineBreakOrEnd:
        return character == nc::newLine || character == nc::carriageReturn || character.isEndOfData();
    case CharClass::ValidAfterValue:
        return character.isAsciiBlank() || character == nc::newLine || character == nc::carriageReturn ||
            character == nc::commentStart || character.isEndOfData() || character == nc::valueListSeparator;
    case CharClass::ValidLang:
        return character == nc::tab || character == nc::newLine || character == nc::carriageReturn ||
            (character.isValidUnicode() && character.toRawValue() > 0x1FU &&
                !(character.toRawValue() >= 0x7FU && character.toRawValue() <= 0xA0U));
    case CharClass::FilePathSeparator:
        return character == nc::backslash || character == nc::slash;
    case CharClass::InvalidWindowsServerName:
        return character.toRawValue() <= 0x1FU || character.toRawValue() > 0x7FU || character == nc::asterisk ||
            character == nc::questionMark || character == nc::pipe || character == nc::doubleQuote ||
            character == nc::lessThan;
    }
    return false;
}

}
