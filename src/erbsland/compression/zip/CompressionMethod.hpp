// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::zip {

/// A compression method in a ZIP archive.
enum class CompressionMethod : uint16_t {
    Stored = 0,    ///< APPNOTE method 0, stored without compression.
    Deflate = 8,   ///< APPNOTE method 8, Deflate.
    Bzip2 = 12,    ///< APPNOTE method 12, Bzip2.
    Lzma = 14,     ///< APPNOTE method 14, LZMA.
    Zstandard = 93 ///< APPNOTE method 93, Zstandard.
};

}
