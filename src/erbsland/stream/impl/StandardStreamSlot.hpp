// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::stream::impl {

/// Identifies one or both process standard streams.
/// @tested{StandardStreamsTest}
enum class StandardStreamSlot : uint8_t {
    Out, ///< The standard output stream.
    Err, ///< The standard error stream.
    Both ///< Both standard streams.
};

}
