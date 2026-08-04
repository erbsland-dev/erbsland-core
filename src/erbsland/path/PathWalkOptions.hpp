// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathInfoParts.hpp"
#include "PathType.hpp"
#include "PathWalkDirection.hpp"
#include "SymlinkMode.hpp"

namespace erbsland::path {

/// Options for the path walk function.
/// @tested{PathWalkerTest PosixPathWalkerTest}
class PathWalkOptions final {
public:
    /// Create path-walk options with their default values.
    PathWalkOptions() = default;

public:
    /// If all errors should be ignored.
    [[nodiscard]] auto ignoreErrors() const noexcept -> bool { return _ignoreErrors; }
    /// Set if all errors should be ignored.
    auto setIgnoreErrors(const bool value) noexcept -> PathWalkOptions & {
        _ignoreErrors = value;
        return *this;
    }
    /// The reported path types while walking.
    /// If directories are not part of this set, they will still be scanned, but the walk function will
    /// not be called for them.
    [[nodiscard]] auto types() const noexcept -> PathTypes { return _types; }
    /// Set the reported path types while walking.
    auto setTypes(const PathTypes value) noexcept -> PathWalkOptions & {
        _types = value;
        return *this;
    }
    /// How to handle symlinks.
    [[nodiscard]] auto symlinkMode() const noexcept -> SymlinkMode { return _symlinkMode; }
    /// Set how to handle symlinks.
    auto setSymlinkMode(const SymlinkMode value) noexcept -> PathWalkOptions & {
        _symlinkMode = value;
        return *this;
    }
    /// The walk direction.
    [[nodiscard]] auto direction() const noexcept -> PathWalkDirection { return _direction; }
    /// Set the walk direction.
    auto setDirection(const PathWalkDirection value) noexcept -> PathWalkOptions & {
        _direction = value;
        return *this;
    }
    /// The info parts to initially request and cache for a walk with path information.
    [[nodiscard]] auto infoParts() const noexcept -> PathInfoParts { return _infoParts; }
    /// Set the info parts to initially request and cache for a walk with path information.
    auto setInfoParts(const PathInfoParts value) noexcept -> PathWalkOptions & {
        _infoParts = value;
        return *this;
    }

private:
    bool _ignoreErrors = false;
    PathTypes _types = PathTypes{PathType::RegularFile, PathType::Directory};
    SymlinkMode _symlinkMode = SymlinkMode::Follow;
    PathWalkDirection _direction = PathWalkDirection::RootToLeaf;
    PathInfoParts _infoParts = PathInfoPart::Default;
};

}
