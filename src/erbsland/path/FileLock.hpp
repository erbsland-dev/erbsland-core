// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileLock_fwd.hpp"
#include "Path.hpp"

#include "impl/FileLock_fwd.hpp"

#include <memory>

namespace erbsland::path {

/// An exclusive process lock associated with a filesystem path.
///
/// Instances are created through `Path::createLock()`. The operating-system lock is held on a persistent sidecar
/// file with `.lock` appended to the protected path. Keeping the lock separate allows the protected file to be
/// replaced atomically without losing mutual exclusion. Destroying or moving from this object releases the lock;
/// the sidecar file intentionally remains to avoid races between lock users.
/// @tested{PathFileLockTest}
class FileLock final {
    friend class Path;

public:
    /// Release the held lock.
    ~FileLock();
    /// Move a held lock from another instance.
    FileLock(FileLock &&other) noexcept;
    /// Release any held lock and move another lock into this instance.
    auto operator=(FileLock &&other) noexcept -> FileLock &;

    // defaults/deletions
    FileLock(const FileLock &) = delete;
    auto operator=(const FileLock &) -> FileLock & = delete;

public: // tests
    /// Test whether this object currently holds a lock.
    [[nodiscard]] auto isLocked() const noexcept -> bool;

public: // accessors
    /// Get the path protected by this lock.
    [[nodiscard]] auto path() const noexcept -> const Path & { return _path; }
    /// Get the sidecar file on which the operating-system lock is held.
    [[nodiscard]] auto lockPath() const noexcept -> const Path & { return _lockPath; }

private:
    /// Acquire a nonblocking exclusive lock for a protected path.
    /// @param path The filesystem path whose users are synchronized.
    /// @throws PathError If the path is invalid, the sidecar cannot be opened, or another process holds the lock.
    explicit FileLock(Path path);
    /// Create the sidecar path for a protected path.
    /// @param path The protected filesystem path.
    /// @return The sidecar path.
    /// @throws PathError If the protected path is invalid.
    [[nodiscard]] static auto createLockPath(const Path &path) -> Path;

private:
    Path _path;                            ///< Protected filesystem path.
    Path _lockPath;                        ///< Persistent sidecar lock-file path.
    std::unique_ptr<impl::FileLock> _impl; ///< Platform-specific native lock implementation.
};

}
