// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileSystemLoaderOptions.hpp"
#include "Loader.hpp"

#include "../../path/Path_fwd.hpp"

namespace erbsland::text::render {

/// A loader that loads layouts from a directory on the file system.
/// - All directories must be absolute and exist at the time of construction.
/// - It does not follow symlinks.
/// - It does map layout paths 1:1 to relative file paths.
/// - It searches for layouts in the given search directories in the specified order.
/// @seedoc{/reference/text/render}
/// @tested{FileSystemLoaderTest}
class FileSystemLoader : public Loader {
public:
    /// Create a new file system loader, with a single path and the given options.
    /// @param searchPath The path to the directory to search for layouts.
    /// @param options The options for the file system loader.
    /// @return A new file system loader.
    [[nodiscard]] static auto create(path::Path searchPath, FileSystemLoaderOptions options = {}) -> LoaderPtr;

    /// Create a new file system loader, with a single path and the given options.
    /// @param searchPaths The search paths to directories to be searched in this order for layouts.
    /// @param options The options for the file system loader.
    /// @return A new file system loader.
    [[nodiscard]] static auto create(util::List<path::Path> searchPaths, FileSystemLoaderOptions options = {})
        -> LoaderPtr;
};

}
