// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text {

/// Reserved non-character signals stored in `Char` values.
enum class CharSignal : uint8_t {
    EndOfData,
    NoCodePoint,
    /// An internal character-processing failure.
    /// This signal is reserved for internal algorithms and is never returned by public Erbsland Core text APIs.
    Error,
    /// An encoding-independent byte order mark for use at encoded-data boundaries.
    ByteOrderMark,
};

}
