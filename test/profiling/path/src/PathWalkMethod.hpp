// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace app::path {

/// The directory traversal implementation selected for a profiling scenario.
enum class PathWalkMethod : std::uint8_t {
    PathCallback,     ///< PathWalker with the path-only callback.
    PathInfoCallback, ///< PathWalker with the path-information callback.
    StdRecursive,     ///< std::filesystem recursive directory iterator.
};

}
