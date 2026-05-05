// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::mem {

/// The byte order used to read or write multi-byte integer values.
/// @tested{ByteReaderWriterTest}
enum class Endianness : uint8_t {
    Little, ///< Lowest bytes first.
    Big,    ///< Highest bytes first.
};

}
