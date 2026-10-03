// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// The textual representation produced when rendering a terminal buffer.
enum class RenderFormat : uint8_t {
    Text, ///< Plain text without terminal control sequences.
    Ansi, ///< Text with ANSI SGR presentation sequences.
};

}
