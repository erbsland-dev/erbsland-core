// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::text {

/// ASCII-only categories for fast classification without the Unicode database.
enum class AsciiCategory : uint8_t {
    Letter,          ///< An ASCII letter: `[A-Za-z]`.
    LowercaseLetter, ///< An ASCII lowercase letter: `[a-z]`.
    UppercaseLetter, ///< An ASCII uppercase letter: `[A-Z]`.
    Digit,           ///< An ASCII decimal digit: `[0-9]`.
    HexDigit,        ///< An ASCII hexadecimal digit: `[0-9A-Fa-f]`.
    Alphanumeric,    ///< An ASCII letter or decimal digit: `[A-Za-z0-9]`.
    Word,            ///< An ASCII word character: `[_A-Za-z0-9]`.
    WordWithHyphen,  ///< An ASCII word character or hyphen: `[-_A-Za-z0-9]`.
    DottedName,      ///< An ASCII dotted-name character: `[-._A-Za-z0-9]`.
    UrlScheme,       ///< An ASCII URL-scheme character: `[+\-.A-Za-z0-9]`.
    Base64Text,      ///< A standard Base64 alphabet or padding character: `[+/=A-Za-z0-9]`.
    HttpToken,       ///< An HTTP token character defined by RFC 9110.
    Whitespace,      ///< An ASCII whitespace character: space or `[\t-\r]`.
    Blank,           ///< An ASCII horizontal blank: space or tab.
    Control,         ///< An ASCII control character: `[\x00-\x1f\x7f]`.
    Punctuation,     ///< An ASCII punctuation character.
};

}
