// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::path {

/// Options for removing a file or directory tree.
/// @tested{PathOperationsTest}
class PathRemoveOptions final {
public:
    /// Create path-removal options with their default values.
    PathRemoveOptions() = default;

public:
    /// Remove all directories and files recursively.
    [[nodiscard]] auto recursive() const noexcept -> bool { return _recursive; }
    /// Set whether to remove all directories and files recursively.
    auto setRecursive(const bool value) noexcept -> PathRemoveOptions & {
        _recursive = value;
        return *this;
    }
    /// Ignore all errors when removing the path.
    /// This will leave files and directories that cause errors in place.
    [[nodiscard]] auto ignoreErrors() const noexcept -> bool { return _ignoreErrors; }
    /// Set whether to ignore all errors when removing the path.
    auto setIgnoreErrors(const bool value) noexcept -> PathRemoveOptions & {
        _ignoreErrors = value;
        return *this;
    }
    /// Keep any base directory in place.
    [[nodiscard]] auto keepBase() const noexcept -> bool { return _keepBase; }
    /// Set whether to keep any base directory in place.
    auto setKeepBase(const bool value) noexcept -> PathRemoveOptions & {
        _keepBase = value;
        return *this;
    }
    /// Prescan the directory for recursive removal to get a better progress estimate (slower).
    [[nodiscard]] auto prescan() const noexcept -> bool { return _prescan; }
    /// Set whether to prescan the directory for recursive removal to get a better progress estimate (slower).
    auto setPrescan(const bool value) noexcept -> PathRemoveOptions & {
        _prescan = value;
        return *this;
    }

private:
    bool _recursive = false;
    bool _ignoreErrors = false;
    bool _keepBase = false;
    bool _prescan = false;
};

}
