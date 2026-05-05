// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm {

/// Describes how a buffer resize should handle existing content.
enum class BufferResizeMode : uint8_t {
    Fast,            ///< Resize using the fastest available path. Existing content order becomes undefined.
    PreserveContent, ///< Preserve the visible content and fill newly created cells with the fill character.
};

}
