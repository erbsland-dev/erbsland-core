// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::text {

/// Binary encodings supported by UTF-8 string byte conversion APIs.
/// Multi-byte encodings use either the explicit byte order named by the enum value,
/// or require a byte order mark to indicate the byte order when decoding
/// @seedoc{/reference/text/string_converter}
/// @tested{U8StringEncodingTest}
enum class StringEncoding : uint8_t {
    Utf8,              ///< UTF-8 bytes without a byte order mark.
    Utf16,             ///< UTF-16 bytes, detect byte order from byte order mark, encode as LE.
    Utf16LittleEndian, ///< UTF-16 bytes, least significant byte first.
    Utf16BigEndian,    ///< UTF-16 bytes, most significant byte first.
    Utf32,             ///< UTF-32 bytes, detect byte order from byte order mark, encode as LE.
    Utf32LittleEndian, ///< UTF-32 bytes, least significant byte first.
    Utf32BigEndian,    ///< UTF-32 bytes, most significant byte first.
};

}
