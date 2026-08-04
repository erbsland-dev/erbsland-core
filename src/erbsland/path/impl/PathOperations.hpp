// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathOperations_fwd.hpp"

#include "../Path.hpp"
#include "../PathAccessProfile.hpp"
#include "../PathAttribute.hpp"
#include "../PathChangeOptions.hpp"
#include "../PathCopyOptions.hpp"
#include "../PathCreateDirectoryOptions.hpp"
#include "../PathCreateFileOptions.hpp"
#include "../PathMoveOptions.hpp"
#include "../PathProgress.hpp"
#include "../PathRemoveOptions.hpp"

#include <functional>
#include <utility>

namespace erbsland::path::impl {

/// Implements file-system operations for one path.
/// @tested{PathOperationsTest}
class PathOperations {
public:
    /// Create operations for an empty path.
    PathOperations() = default;
    /// Create operations for `path`.
    explicit PathOperations(Path path) : _path{std::move(path)} {}

public:
    /// Access the path operated on by this instance.
    [[nodiscard]] auto path() const noexcept -> const Path & { return _path; }

public:
    /// Remove the path entry.
    void removeOrThrow(PathRemoveOptions options, const PathProgressFn &progressFn);
    /// Copy the path entry to a destination.
    void copyToOrThrow(const Path &destination, PathCopyOptions options, const PathProgressFn &progressFn) const;
    /// Move the path entry to a destination.
    void moveToOrThrow(const Path &destination, PathMoveOptions options) const;
    /// Create a file at the path.
    void createFileOrThrow(PathCreateFileOptions options) const;
    /// Create a directory at the path.
    void createDirectoryOrThrow(PathCreateDirectoryOptions options) const;
    /// Apply an access profile to the path.
    auto setAccessProfile(PathAccessProfile profile, PathChangeOptions options) const -> bool;
    /// Add attributes to the path.
    auto addAttributes(PathAttributes attributes, PathChangeOptions options) const -> bool;
    /// Clear attributes from the path.
    auto clearAttributes(PathAttributes attributes, PathChangeOptions options) const -> bool;

private:
    /// Count entries that can contribute to progress reporting.
    [[nodiscard]] auto countForProgress(SymlinkMode symlinkMode) const -> unit::ItemCount;
    /// Create any missing parent directories for a path.
    static void createParentsOrThrow(const Path &path);
    /// Remove an existing path entry.
    static void removeExistingOrThrow(const Path &path);
    /// Report the current progress of an operation.
    static void reportProgress(
        const PathProgressFn &progressFn,
        PathProgressStatus status,
        unit::ItemCount total,
        unit::ItemCount processed,
        unit::ItemCount errors);
    /// Apply one change and report whether it altered the path.
    [[nodiscard]] auto applyChange(
        const PathChangeOptions &options, const std::function<void(const Path &)> &changeFn) const -> bool;

private:
    Path _path;
};

}
