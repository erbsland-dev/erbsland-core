// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::text {

/// The mode for handling byte order marks (BOM) in string encoding and decoding.
/// @tested{U8StringEncodingTest}
enum class StringBomMode : uint8_t {
    /// Accepts a BOM at the start of decoded byte data.
    /// For `StringEncoding::Utf16` and `StringEncoding::Utf32` change the byte order if a BOM is found,
    /// if no BOM is found, expect little endian byte order.
    /// For the `StringEncoding::Utf16LittleEndian`, `StringEncoding::Utf16BigEndian`,
    /// `StringEncoding::Utf32LittleEndian`, `StringEncoding::Utf32BigEndian` modes,
    /// throw an exception if a BOM is found with the opposite byte order.
    /// Never add a BOM when encoding UTF-8 strings, always add a BOM when encoding UTF-16 and UTF-32 strings.
    Automatic,
    /// Require a BOM at the start of decoded byte data.
    /// Throw an exception if no BOM is found.
    /// For `StringEncoding::Utf16` and `StringEncoding::Utf32` change the byte order if a BOM is found,
    /// For the `StringEncoding::Utf16LittleEndian`, `StringEncoding::Utf16BigEndian`,
    /// `StringEncoding::Utf32LittleEndian`, `StringEncoding::Utf32BigEndian` modes,
    /// throw an exception if a BOM is found with the opposite byte order.
    /// Always add a BOM when encoding strings.
    Require,
    /// Reject a BOM at the start of decoded byte data.
    /// Throw an exception if a BOM is found.
    /// Never add a BOM when encoding strings.
    Reject,
};

}
