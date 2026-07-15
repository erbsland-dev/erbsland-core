// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathCollisionMode.hpp"
#include "SymlinkMode.hpp"

namespace erbsland::path {

/// The options for a copy operation.
/// @tested{PathOperationsTest}
class PathCopyOptions final {
public:
    PathCopyOptions() = default;

public:
    /// Whether filesystem errors are ignored while copying remaining entries.
    [[nodiscard]] auto ignoreErrors() const noexcept -> bool { return _ignoreErrors; }
    /// Set whether filesystem errors are ignored while copying remaining entries.
    auto setIgnoreErrors(const bool value) noexcept -> PathCopyOptions & {
        _ignoreErrors = value;
        return *this;
    }
    /// Whether directory contents are copied recursively.
    [[nodiscard]] auto recursive() const noexcept -> bool { return _recursive; }
    /// Set whether directory contents are copied recursively.
    auto setRecursive(const bool value) noexcept -> PathCopyOptions & {
        _recursive = value;
        return *this;
    }
    /// The behavior when the exact destination path already exists.
    [[nodiscard]] auto collisionMode() const noexcept -> PathCollisionMode { return _collisionMode; }
    /// Set the behavior when the exact destination path already exists.
    auto setCollisionMode(const PathCollisionMode value) noexcept -> PathCopyOptions & {
        _collisionMode = value;
        return *this;
    }
    /// The behavior for symbolic links encountered by the copy operation.
    [[nodiscard]] auto symlinkMode() const noexcept -> SymlinkMode { return _symlinkMode; }
    /// Set the behavior for symbolic links encountered by the copy operation.
    auto setSymlinkMode(const SymlinkMode value) noexcept -> PathCopyOptions & {
        _symlinkMode = value;
        return *this;
    }
    /// Whether missing destination parent directories are created.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createParents; }
    /// Set whether missing destination parent directories are created.
    auto setCreateParents(const bool value) noexcept -> PathCopyOptions & {
        _createParents = value;
        return *this;
    }
    /// Whether the source is scanned first to calculate an exact progress total.
    [[nodiscard]] auto prescan() const noexcept -> bool { return _prescan; }
    /// Set whether the source is scanned first to calculate an exact progress total.
    auto setPrescan(const bool value) noexcept -> PathCopyOptions & {
        _prescan = value;
        return *this;
    }

private:
    bool _ignoreErrors = false;
    bool _recursive = false;
    PathCollisionMode _collisionMode = PathCollisionMode::Stop;
    SymlinkMode _symlinkMode = SymlinkMode::Follow;
    bool _createParents = false;
    bool _prescan = false;
};

}
