// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathInfoCache_fwd.hpp"
#include "PathInfoData.hpp"

#include <mutex>

namespace erbsland::path::impl {

/// Lifetime marker for trusting directory-scan metadata during a path walk.
/// @notest{This marker has no behavior.}
class PathInfoCacheTrust final {};

/// The synchronized path-information cache attached to a path value.
/// @tested{PathInfoTest PathWalkerTest}
class PathInfoCache final {
public:
    /// Create an empty path-information cache.
    PathInfoCache() = default;

    // defaults/deletions
    ~PathInfoCache() = default;
    PathInfoCache(const PathInfoCache &) = delete;
    PathInfoCache(PathInfoCache &&) = delete;
    auto operator=(const PathInfoCache &) -> PathInfoCache & = delete;
    auto operator=(PathInfoCache &&) -> PathInfoCache & = delete;

public:
    std::mutex mutex;  ///< Synchronizes refreshes and reads of the cached snapshot.
    PathInfoData data; ///< The cached information snapshot.
};

}
