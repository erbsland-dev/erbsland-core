// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathAccessRight.hpp"

namespace erbsland::path {

/// Best-effort portable access information for a path.
/// @tested{PathInfoTest PosixPathInfoTest WindowsPathInfoTest}
class PathAccessInfo final {
public:
    /// Create empty access information.
    PathAccessInfo() = default;

public: // accessors
    /// Access rights for the current process.
    [[nodiscard]] auto currentProcessRights() const noexcept -> PathAccessRights { return _currentProcessRights; }
    /// Set access rights for the current process.
    auto setCurrentProcessRights(const PathAccessRights value) noexcept -> PathAccessInfo & {
        _currentProcessRights = value;
        return *this;
    }
    /// Test if owner/group/other rights are available.
    [[nodiscard]] auto hasPortableRights() const noexcept -> bool { return _hasPortableRights; }
    /// Set whether owner/group/other rights are available.
    auto setHasPortableRights(const bool value) noexcept -> PathAccessInfo & {
        _hasPortableRights = value;
        return *this;
    }
    /// Access rights for the owner class.
    [[nodiscard]] auto ownerRights() const noexcept -> PathAccessRights { return _ownerRights; }
    /// Set access rights for the owner class.
    auto setOwnerRights(const PathAccessRights value) noexcept -> PathAccessInfo & {
        _ownerRights = value;
        return *this;
    }
    /// Access rights for the group class.
    [[nodiscard]] auto groupRights() const noexcept -> PathAccessRights { return _groupRights; }
    /// Set access rights for the group class.
    auto setGroupRights(const PathAccessRights value) noexcept -> PathAccessInfo & {
        _groupRights = value;
        return *this;
    }
    /// Access rights for all other users.
    [[nodiscard]] auto otherRights() const noexcept -> PathAccessRights { return _otherRights; }
    /// Set access rights for all other users.
    auto setOtherRights(const PathAccessRights value) noexcept -> PathAccessInfo & {
        _otherRights = value;
        return *this;
    }

private:
    PathAccessRights _currentProcessRights; ///< Rights available to the current process.
    bool _hasPortableRights{false};         ///< If owner/group/other rights are available.
    PathAccessRights _ownerRights;          ///< Owner class rights.
    PathAccessRights _groupRights;          ///< Group class rights.
    PathAccessRights _otherRights;          ///< Other users rights.
};

}
