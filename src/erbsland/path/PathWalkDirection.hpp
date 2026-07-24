// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::path {

/// The direction for walking paths.
enum class PathWalkDirection : uint8_t {
    /// Walk from root to leaf.
    /// All files and subdirectories of a directory are processed before descending into subdirectories.
    RootToLeaf,
    /// Walk from leaf to root.
    /// Processing the leaf files first, then processing the parent directories.
    /// At the time a directory is encountered, all its contents were processed before.
    LeafToRoot,
};

}
