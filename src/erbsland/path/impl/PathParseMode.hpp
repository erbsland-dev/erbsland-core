// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path::impl {

/// The input format used for parsing a path string.
enum class PathParseMode : uint8_t {
    Generic, ///< Auto-detect roots and otherwise parse a generic relative path.
    Posix,   ///< Parse exactly as POSIX text, where only slash is a separator.
    Windows, ///< Parse as Windows text, where slash and backslash are separators.
    Native,  ///< Parse using the current platform's native path format.
};

}
