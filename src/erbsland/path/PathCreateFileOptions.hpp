// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathAccessProfile.hpp"
#include "PathCreateMode.hpp"

namespace erbsland::path {

/// Options for creating/opening files for writing.
/// @tested{PathOperationsTest}
class PathCreateFileOptions final {
public:
    PathCreateFileOptions() = default;

public:
    /// Create the parent directories if they do not exist.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createParents; }
    /// Set whether to create the parent directories if they do not exist.
    auto setCreateParents(const bool value) noexcept -> PathCreateFileOptions & {
        _createParents = value;
        return *this;
    }
    /// The creation mode for the file.
    [[nodiscard]] auto creationMode() const noexcept -> PathCreateMode { return _creationMode; }
    /// Set the creation mode for the file.
    auto setCreationMode(const PathCreateMode mode) noexcept -> PathCreateFileOptions & {
        _creationMode = mode;
        return *this;
    }
    /// The access profile for newly created files.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile { return _accessProfile; }
    /// Set the access profile for newly created files.
    auto setAccessProfile(const PathAccessProfile value) noexcept -> PathCreateFileOptions & {
        _accessProfile = value;
        return *this;
    }

private:
    bool _createParents = false;
    PathCreateMode _creationMode = PathCreateMode::CreateNew;
    PathAccessProfile _accessProfile = PathAccessProfile::Default;
};

}
