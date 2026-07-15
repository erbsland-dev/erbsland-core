// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathResolveMode.hpp"

namespace erbsland::path {

/// Options for resolving paths.
class PathResolveOptions final {
public:
    PathResolveOptions() = default;
    /// Create options from a resolve mode.
    PathResolveOptions(const PathResolveMode mode) noexcept : _mode{mode} {}

public:
    /// The mode for resolving paths.
    [[nodiscard]] auto mode() const noexcept -> PathResolveMode { return _mode; }
    /// Set the mode for resolving paths.
    auto setMode(const PathResolveMode mode) noexcept -> PathResolveOptions & {
        _mode = mode;
        return *this;
    }

private:
    PathResolveMode _mode = PathResolveMode::Physical;
};

}
