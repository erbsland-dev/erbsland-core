// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Char.hpp"

namespace erbsland::conf::impl::nc {

// Named characters.
inline constexpr auto newLine = text::Char{U'\n'};
inline constexpr auto carriageReturn = text::Char{U'\r'};
inline constexpr auto space = text::Char{U' '};
inline constexpr auto tab = text::Char{U'\t'};
inline constexpr auto hash = text::Char{U'#'};
inline constexpr auto at = text::Char{U'@'};
inline constexpr auto doubleQuote = text::Char{U'"'};
inline constexpr auto singleQuote = text::Char{U'\''};
inline constexpr auto backtick = text::Char{U'`'};
inline constexpr auto slash = text::Char{U'/'};
inline constexpr auto lessThan = text::Char{U'<'};
inline constexpr auto greaterThan = text::Char{U'>'};
inline constexpr auto underscore = text::Char{U'_'};
inline constexpr auto dollar = text::Char{U'$'};
inline constexpr auto lowercaseA = text::Char{U'a'};
inline constexpr auto lowercaseB = text::Char{U'b'};
inline constexpr auto lowercaseE = text::Char{U'e'};
inline constexpr auto lowercaseF = text::Char{U'f'};
inline constexpr auto lowercaseI = text::Char{U'i'};
inline constexpr auto lowercaseN = text::Char{U'n'};
inline constexpr auto lowercaseR = text::Char{U'r'};
inline constexpr auto lowercaseT = text::Char{U't'};
inline constexpr auto lowercaseU = text::Char{U'u'};
inline constexpr auto lowercaseX = text::Char{U'x'};
inline constexpr auto lowercaseZ = text::Char{U'z'};
inline constexpr auto uppercaseA = text::Char{U'A'};
inline constexpr auto uppercaseB = text::Char{U'B'};
inline constexpr auto uppercaseE = text::Char{U'E'};
inline constexpr auto uppercaseF = text::Char{U'F'};
inline constexpr auto uppercaseI = text::Char{U'I'};
inline constexpr auto uppercaseN = text::Char{U'N'};
inline constexpr auto uppercaseR = text::Char{U'R'};
inline constexpr auto uppercaseT = text::Char{U'T'};
inline constexpr auto uppercaseU = text::Char{U'U'};
inline constexpr auto uppercaseX = text::Char{U'X'};
inline constexpr auto uppercaseZ = text::Char{U'Z'};
inline constexpr auto digit0 = text::Char{U'0'};
inline constexpr auto digit1 = text::Char{U'1'};
inline constexpr auto digit9 = text::Char{U'9'};
inline constexpr auto colon = text::Char{U':'};
inline constexpr auto equal = text::Char{U'='};
inline constexpr auto comma = text::Char{U','};
inline constexpr auto fullStop = text::Char{U'.'};
inline constexpr auto backslash = text::Char{U'\\'};
inline constexpr auto openingCurlyBracket = text::Char{U'{'};
inline constexpr auto closingCurlyBracket = text::Char{U'}'};
inline constexpr auto openingSquareBracket = text::Char{U'['};
inline constexpr auto closingSquareBracket = text::Char{U']'};
inline constexpr auto plus = text::Char{U'+'};
inline constexpr auto minus = text::Char{U'-'};
inline constexpr auto asterisk = text::Char{U'*'};
inline constexpr auto microSign = text::Char{U'µ'};
inline constexpr auto questionMark = text::Char{U'?'};
inline constexpr auto pipe = text::Char{U'|'};

// Language meanings.
inline constexpr auto commentStart = hash;
inline constexpr auto decimalPoint = fullStop;
inline constexpr auto timeSeparator = colon;
inline constexpr auto dateSeparator = minus;
inline constexpr auto namePathSeparator = fullStop;
inline constexpr auto valueListSeparator = comma;
inline constexpr auto digitSeparator = singleQuote;

}
