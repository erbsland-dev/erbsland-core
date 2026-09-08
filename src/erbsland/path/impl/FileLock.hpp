// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FileLock_fwd.hpp"

#include "../Path_fwd.hpp"

#include <memory>

namespace erbsland::path::impl {

/// A platform-specific native file lock.
/// @tested{PathFileLockTest}
class FileLock {
public:
    // defaults/deletions
    virtual ~FileLock() = default;
    FileLock(const FileLock &) = delete;
    FileLock(FileLock &&) = delete;
    auto operator=(const FileLock &) -> FileLock & = delete;
    auto operator=(FileLock &&) -> FileLock & = delete;

public: // factories
    /// Acquire the platform-specific native lock.
    /// @param path The filesystem path protected by the lock.
    /// @param lockPath The persistent sidecar file to lock.
    /// @return The acquired native lock.
    /// @throws PathError If the sidecar cannot be opened or locked.
    [[nodiscard]] static auto create(const Path &path, const Path &lockPath) -> std::unique_ptr<FileLock>;

protected:
    /// Create the base for a native file lock.
    FileLock() = default;
};

}
