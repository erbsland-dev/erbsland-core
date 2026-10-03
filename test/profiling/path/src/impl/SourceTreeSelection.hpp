// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/PathWalkOptions.hpp>

namespace app::path::impl {

/// Test whether an entry is a regular C++ source file.
/// @param path Entry path.
/// @param type Entry type without following symlinks.
/// @return Whether to read the entry.
[[nodiscard]] auto isSourceFile(const el::Path &path, el::PathType type) -> bool;

/// Test whether a directory belongs to the source corpus.
/// @param path Directory path.
/// @return Whether to enter the directory.
[[nodiscard]] auto shouldDescendSourceTree(const el::Path &path) -> bool;

/// Create the corresponding walker options, reporting directories and selected files.
/// @return Options that skip symlinks and prune build/cache directories.
[[nodiscard]] auto sourceTreeWalkOptions() -> el::PathWalkOptions;

}
