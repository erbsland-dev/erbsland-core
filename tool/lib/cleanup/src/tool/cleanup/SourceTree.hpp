// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>

#include <memory>

namespace tool::cleanup {

/// Include policy and lookup roots for one non-overlapping C++ source tree.
struct SourceTree {
    el::Path directory;         ///< Absolute, checked input directory.
    bool publicIncludes = true; ///< Use canonical public Core includes in consumers.
    el::PathList includeRoots;  ///< Additional local compiler include roots.
};

/// Shared immutable source policy used by cleanup jobs.
using SourceTreePtr = std::shared_ptr<const SourceTree>;

}
