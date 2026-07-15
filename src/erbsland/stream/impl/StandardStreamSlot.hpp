// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::stream::impl {

/// Identifies one or more process standard streams.
enum class StandardStreamSlot : uint8_t {
    In,  ///< The standard input stream.
    Out, ///< The standard output stream.
    Err, ///< The standard error stream.
    Both ///< Both standard output streams.
};

}
