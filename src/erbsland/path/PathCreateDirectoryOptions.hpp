// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathAccessProfile.hpp"
#include "PathCreateMode.hpp"

namespace erbsland::path {

/// Options for creating directories.
/// @tested{PathOperationsTest}
class PathCreateDirectoryOptions final {
public:
    /// Create directory-creation options with their default values.
    PathCreateDirectoryOptions() = default;

public:
    /// Create the parent directories if they do not exist.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createParents; }
    /// Set whether to create the parent directories if they do not exist.
    auto setCreateParents(const bool value) noexcept -> PathCreateDirectoryOptions & {
        _createParents = value;
        return *this;
    }
    /// The creation mode for the directory.
    [[nodiscard]] auto creationMode() const noexcept -> PathCreateMode { return _creationMode; }
    /// Set the creation mode for the directory.
    auto setCreationMode(const PathCreateMode mode) noexcept -> PathCreateDirectoryOptions & {
        _creationMode = mode;
        return *this;
    }
    /// The access profile for newly created directories.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile { return _accessProfile; }
    /// Set the access profile for newly created directories.
    auto setAccessProfile(const PathAccessProfile value) noexcept -> PathCreateDirectoryOptions & {
        _accessProfile = value;
        return *this;
    }

private:
    bool _createParents = false;
    PathCreateMode _creationMode = PathCreateMode::CreateNew;
    PathAccessProfile _accessProfile = PathAccessProfile::Default;
};

}
