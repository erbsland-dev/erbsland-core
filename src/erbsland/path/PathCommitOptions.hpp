// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathCollisionMode.hpp"
#include "PathCommitCheckFn.hpp"

#include "../unit/ItemCount.hpp"

#include <utility>

namespace erbsland::path {

/// Policies for committing a completed temporary file, optionally retaining overwritten content.
/// @seedoc{/reference/path/file_system_paths}
/// @tested{PathReplacementTest}
class PathCommitOptions final {
public: // policies
    /// Get the collision policy; only Stop and Overwrite are supported.
    [[nodiscard]] auto collisionMode() const noexcept -> PathCollisionMode { return _collisionMode; }
    /// Set the collision policy.
    auto setCollisionMode(PathCollisionMode value) noexcept -> PathCommitOptions & {
        _collisionMode = value;
        return *this;
    }
    /// Test whether missing destination parents are created.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createParents; }
    /// Set whether missing destination parents are created.
    auto setCreateParents(bool value) noexcept -> PathCommitOptions & {
        _createParents = value;
        return *this;
    }
    /// Get the finite number of numbered backups; zero leaves all backups untouched.
    [[nodiscard]] auto backupCount() const noexcept -> unit::ItemCount { return _backupCount; }
    /// Set the number of backups to retain.
    auto setBackupCount(unit::ItemCount value) noexcept -> PathCommitOptions & {
        _backupCount = value;
        return *this;
    }
    /// Test whether backup failures are ignored while replacement is still attempted.
    [[nodiscard]] auto ignoreBackupErrors() const noexcept -> bool { return _ignoreBackupErrors; }
    /// Set whether to ignore errors in backup preparation only.
    auto setIgnoreBackupErrors(bool value) noexcept -> PathCommitOptions & {
        _ignoreBackupErrors = value;
        return *this;
    }
    /// Get the repeatable synchronous check, called after finalization and again after backup preparation.
    [[nodiscard]] auto beforeCommit() const noexcept -> const PathCommitCheckFn & { return _beforeCommit; }
    /// Set a check whose runtime failure cancels committing; it must not modify the temporary stream.
    auto setBeforeCommit(PathCommitCheckFn value) -> PathCommitOptions & {
        _beforeCommit = std::move(value);
        return *this;
    }

private:
    PathCollisionMode _collisionMode{PathCollisionMode::Overwrite}; ///< Destination collision policy.
    bool _createParents{false};                                     ///< Create missing parent directories.
    unit::ItemCount _backupCount{};                                 ///< Requested retained generations.
    bool _ignoreBackupErrors{false};                                ///< Best-effort backup policy.
    PathCommitCheckFn _beforeCommit;                                ///< Caller validation or cancellation check.
};

}
