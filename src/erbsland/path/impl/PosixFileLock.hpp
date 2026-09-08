// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileLock.hpp"

namespace erbsland::path::impl {

/// A POSIX advisory file lock.
/// @tested{PathFileLockTest}
class PosixFileLock final : public FileLock {
public:
    /// Open and exclusively lock a sidecar file.
    /// @param path The filesystem path protected by the lock.
    /// @param lockPath The persistent sidecar file to lock.
    /// @throws PathError If the sidecar cannot be opened or locked.
    PosixFileLock(const Path &path, const Path &lockPath);
    /// Release the lock and close its file descriptor.
    ~PosixFileLock() override;

    // defaults/deletions
    PosixFileLock(const PosixFileLock &) = delete;
    PosixFileLock(PosixFileLock &&) = delete;
    auto operator=(const PosixFileLock &) -> PosixFileLock & = delete;
    auto operator=(PosixFileLock &&) -> PosixFileLock & = delete;

private:
    /// Throw a path error for a native locking failure.
    /// @param path The protected filesystem path.
    /// @param lockPath The sidecar path.
    /// @param errorCode The captured POSIX error number.
    [[noreturn]] static void throwLockError(const Path &path, const Path &lockPath, int errorCode);

private:
    int _descriptor{-1}; ///< The locked sidecar file descriptor.
};

}
