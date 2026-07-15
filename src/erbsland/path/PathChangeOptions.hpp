// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymlinkMode.hpp"

namespace erbsland::path {

/// Options for changing path metadata.
/// @tested{PathOperationsTest}
class PathChangeOptions final {
public:
    PathChangeOptions() = default;

public: // accessors
    /// Apply the change recursively.
    [[nodiscard]] auto recursive() const noexcept -> bool { return _recursive; }
    /// Set whether to apply the change recursively.
    auto setRecursive(const bool value) noexcept -> PathChangeOptions & {
        _recursive = value;
        return *this;
    }
    /// Ignore errors while applying changes.
    [[nodiscard]] auto ignoreErrors() const noexcept -> bool { return _ignoreErrors; }
    /// Set whether to ignore errors while applying changes.
    auto setIgnoreErrors(const bool value) noexcept -> PathChangeOptions & {
        _ignoreErrors = value;
        return *this;
    }
    /// How to handle symbolic links.
    [[nodiscard]] auto symlinkMode() const noexcept -> SymlinkMode { return _symlinkMode; }
    /// Set how to handle symbolic links.
    auto setSymlinkMode(const SymlinkMode value) noexcept -> PathChangeOptions & {
        _symlinkMode = value;
        return *this;
    }

private:
    bool _recursive{false};
    bool _ignoreErrors{false};
    SymlinkMode _symlinkMode{SymlinkMode::Use};
};

}
