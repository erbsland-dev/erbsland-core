// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::stream {

/// The reference point for moving a stream position.
enum class StreamPositionOrigin : uint8_t {
    Start,   ///< Move relative to byte position zero.
    Current, ///< Move relative to the logical current position.
    End,     ///< Move relative to the current end of the stream.
};

}
