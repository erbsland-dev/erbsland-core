// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path {

/// The format of a path.
enum class PathFormat : uint8_t {
    Generic, ///< A generic path that works on all platforms.
    Posix,   ///< A POSIX path that works on macOS and Linux (like `/...`).
    Windows, ///< A Windows path that works on Windows (like `C:/...` or `//server/share/...`).
};

}
