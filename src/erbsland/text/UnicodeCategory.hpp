// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text {

/// A Unicode general category.
/// @seedoc{/reference/text/char_range}
enum class UnicodeCategory : uint8_t {
    UppercaseLetter = 0x00,      ///< An uppercase letter.
    LowercaseLetter = 0x01,      ///< A lowercase letter.
    TitlecaseLetter = 0x02,      ///< A titlecase letter.
    ModifierLetter = 0x03,       ///< A modifier letter.
    OtherLetter = 0x04,          ///< A letter of other kind.
    NonspacingMark = 0x10,       ///< A nonspacing combining mark.
    SpacingMark = 0x11,          ///< A spacing combining mark.
    EnclosingMark = 0x12,        ///< An enclosing combining mark.
    DecimalNumber = 0x20,        ///< A decimal digit.
    LetterNumber = 0x21,         ///< A letter-like numeric character.
    OtherNumber = 0x22,          ///< A numeric character of other kind.
    ConnectorPunctuation = 0x30, ///< A connector punctuation mark.
    DashPunctuation = 0x31,      ///< A dash or hyphen punctuation mark.
    OpenPunctuation = 0x32,      ///< An opening punctuation mark.
    ClosePunctuation = 0x33,     ///< A closing punctuation mark.
    InitialPunctuation = 0x34,   ///< An initial quotation mark.
    FinalPunctuation = 0x35,     ///< A final quotation mark.
    OtherPunctuation = 0x36,     ///< A punctuation mark of other kind.
    MathSymbol = 0x40,           ///< A mathematical symbol.
    CurrencySymbol = 0x41,       ///< A currency symbol.
    ModifierSymbol = 0x42,       ///< A modifier symbol.
    OtherSymbol = 0x43,          ///< A symbol of other kind.
    SpaceSeparator = 0x50,       ///< A space separator.
    LineSeparator = 0x51,        ///< A line separator.
    ParagraphSeparator = 0x52,   ///< A paragraph separator.
    Control = 0x60,              ///< A control code.
    Format = 0x61,               ///< A format control character.
    Surrogate = 0x62,            ///< A surrogate code point.
    PrivateUse = 0x63,           ///< A private-use code point.
    Unassigned = 0x64,           ///< An unassigned or reserved code point.
};

}
