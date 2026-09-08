// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileLock.hpp"

#include "../../core/impl/WindowsApi.hpp"

namespace erbsland::path::impl {

/// A Windows byte-range file lock.
/// @tested{PathFileLockTest}
class WindowsFileLock final : public FileLock {
public:
    /// Open and exclusively lock a sidecar file.
    /// @param path The filesystem path protected by the lock.
    /// @param lockPath The persistent sidecar file to lock.
    /// @throws PathError If the sidecar cannot be opened or locked.
    WindowsFileLock(const Path &path, const Path &lockPath);
    /// Release the lock and close its file handle.
    ~WindowsFileLock() override;

    // defaults/deletions
    WindowsFileLock(const WindowsFileLock &) = delete;
    WindowsFileLock(WindowsFileLock &&) = delete;
    auto operator=(const WindowsFileLock &) -> WindowsFileLock & = delete;
    auto operator=(WindowsFileLock &&) -> WindowsFileLock & = delete;

private:
    /// Throw a path error for a native locking failure.
    /// @param path The protected filesystem path.
    /// @param lockPath The sidecar path.
    /// @param errorCode The captured Windows error code.
    [[noreturn]] static void throwLockError(const Path &path, const Path &lockPath, unsigned long errorCode);

private:
    HANDLE _handle{}; ///< The locked sidecar file handle.
};

}
