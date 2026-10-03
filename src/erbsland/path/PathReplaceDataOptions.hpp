// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathAccessProfile.hpp"
#include "PathCommitOptions.hpp"

#include "../stream/OutputStreamSettings.hpp"
#include "../time/TimeDelta.hpp"

#include <utility>

namespace erbsland::path {

/// Options for replacing complete data content using a sibling temporary file.
/// @tested{PathReplacementTest}
class PathReplaceDataOptions final {
public:
    /// The default maximum wait for each stream operation.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(60);

public: // commit policies
    /// Get the common commit policies.
    [[nodiscard]] auto commitOptions() const noexcept -> const PathCommitOptions & { return _commitOptions; }
    /// Set the common commit policies.
    auto setCommitOptions(PathCommitOptions value) -> PathReplaceDataOptions & {
        _commitOptions = std::move(value);
        return *this;
    }
    /// Get the destination collision policy.
    [[nodiscard]] auto collisionMode() const noexcept -> PathCollisionMode { return _commitOptions.collisionMode(); }
    /// Set the destination collision policy.
    auto setCollisionMode(PathCollisionMode value) noexcept -> PathReplaceDataOptions & {
        _commitOptions.setCollisionMode(value);
        return *this;
    }
    /// Test whether missing destination parents are created.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _commitOptions.createParents(); }
    /// Set whether missing destination parents are created.
    auto setCreateParents(bool value) noexcept -> PathReplaceDataOptions & {
        _commitOptions.setCreateParents(value);
        return *this;
    }
    /// Get the number of retained backups; zero leaves backups untouched.
    [[nodiscard]] auto backupCount() const noexcept -> unit::ItemCount { return _commitOptions.backupCount(); }
    /// Set the finite number of numbered backups to retain.
    auto setBackupCount(unit::ItemCount value) noexcept -> PathReplaceDataOptions & {
        _commitOptions.setBackupCount(value);
        return *this;
    }
    /// Test whether backup preparation failures are ignored.
    [[nodiscard]] auto ignoreBackupErrors() const noexcept -> bool { return _commitOptions.ignoreBackupErrors(); }
    /// Set whether to ignore backup preparation failures only.
    auto setIgnoreBackupErrors(bool value) noexcept -> PathReplaceDataOptions & {
        _commitOptions.setIgnoreBackupErrors(value);
        return *this;
    }
    /// Get the repeatable synchronous pre-commit check.
    [[nodiscard]] auto beforeCommit() const noexcept -> const PathCommitCheckFn & {
        return _commitOptions.beforeCommit();
    }
    /// Set the repeatable synchronous pre-commit check.
    auto setBeforeCommit(PathCommitCheckFn value) -> PathReplaceDataOptions & {
        _commitOptions.setBeforeCommit(std::move(value));
        return *this;
    }

public: // output settings
    /// Get the access profile applied to the new file.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile { return _accessProfile; }
    /// Set the access profile applied to the new file.
    auto setAccessProfile(PathAccessProfile value) noexcept -> PathReplaceDataOptions & {
        _accessProfile = value;
        return *this;
    }
    /// Get settings for the owned output stream.
    [[nodiscard]] auto streamSettings() const noexcept -> const stream::OutputStreamSettings & {
        return _streamSettings;
    }
    /// Set settings for the owned output stream.
    auto setStreamSettings(stream::OutputStreamSettings value) noexcept -> PathReplaceDataOptions & {
        _streamSettings = value;
        return *this;
    }
    /// Get the maximum wait for one stream operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _streamSettings.timeout(); }
    /// Set the maximum wait for one stream operation.
    auto setTimeout(time::TimeDelta value) noexcept -> PathReplaceDataOptions & {
        _streamSettings.setTimeout(value);
        return *this;
    }

private:
    PathCommitOptions _commitOptions;                                ///< Common replacement policies.
    PathAccessProfile _accessProfile{PathAccessProfile::Default};    ///< New-file access profile.
    stream::OutputStreamSettings _streamSettings{
        stream::OutputStreamSettings{}.setTimeout(cDefaultTimeout)}; ///< Output settings.
};

}
