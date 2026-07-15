// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path {

/// The mode how to resolve a path.
enum class PathResolveMode : uint8_t {
    /// Only normalize the path without touching the file system.
    /// - Removing redundant path elements, such as `.`, `..`, `//`.
    Lexical,
    /// Resolve the path to its physical location as far as possible.
    /// - Removing redundant path elements, such as `.`, `..`, `//`.
    /// - Following symbolic links.
    /// - Normalize non-existing path elements.
    Weak,
    /// Fully resolve the path but don't follow the final symbolic link.
    /// - Requires the path to exist.
    /// - Removing redundant path elements, such as `.`, `..`, `//`.
    /// - Following symbolic links, except the final one.
    /// This is the mode used by `FileInfo` to resolve the path.
    PhysicalNoFinalSymlink,
    /// Fully resolve the path to its physical location.
    /// - Requires the path to exist.
    /// - Removing redundant path elements, such as `.`, `..`, `//`.
    /// - Following symbolic links.
    Physical,
};

}
