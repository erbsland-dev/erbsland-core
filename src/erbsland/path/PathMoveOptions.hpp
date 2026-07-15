// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathCollisionMode.hpp"

namespace erbsland::path {

/// The options for moving a path.
/// @tested{PathOperationsTest}
class PathMoveOptions final {
public:
    PathMoveOptions() = default;

public:
    /// Ignore all errors.
    [[nodiscard]] auto ignoreErrors() const noexcept -> bool { return _ignoreErrors; }
    /// Set whether to ignore all errors.
    auto setIgnoreErrors(const bool value) noexcept -> PathMoveOptions & {
        _ignoreErrors = value;
        return *this;
    }
    /// Behavior on collision.
    [[nodiscard]] auto collisionMode() const noexcept -> PathCollisionMode { return _collisionMode; }
    /// Set the behavior on collision.
    auto setCollisionMode(const PathCollisionMode value) noexcept -> PathMoveOptions & {
        _collisionMode = value;
        return *this;
    }
    /// Create parent directories if they do not exist.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createParents; }
    /// Set whether to create parent directories if they do not exist.
    auto setCreateParents(const bool value) noexcept -> PathMoveOptions & {
        _createParents = value;
        return *this;
    }

private:
    bool _ignoreErrors = false;
    PathCollisionMode _collisionMode = PathCollisionMode::Stop;
    bool _createParents = false;
};

}
