// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path.hpp"
#include "TempDirectory_fwd.hpp"

#include "../util/Result.hpp"

namespace erbsland::path {

/// A shared temporary directory cleanup lease.
///
/// Temporary directory instances are intended to be created through `PathOperations`.
/// The implementation will remove the directory recursively when the last shared handle is destroyed, unless
/// `release()` was called or automatic cleanup was disabled.
/// @tested{PathTemporaryTest}
class TempDirectory final {
    friend class PathOperations;

public:
    /// Create an empty temporary directory handle.
    TempDirectory() = default;
    /// Destroy the temporary directory handle.
    ~TempDirectory();

    // defaults
    TempDirectory(const TempDirectory &) = delete;
    TempDirectory(TempDirectory &&) = delete;
    auto operator=(const TempDirectory &) -> TempDirectory & = delete;
    auto operator=(TempDirectory &&) -> TempDirectory & = delete;

public: // accessors
    /// Test if this handle has no temporary directory path.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Access the temporary directory path.
    [[nodiscard]] auto path() const noexcept -> const Path &;
    /// Test if the directory is removed when this handle is destroyed.
    [[nodiscard]] auto removeOnDestroy() const noexcept -> bool;
    /// Set whether to remove the directory when this handle is destroyed.
    void setRemoveOnDestroy(bool value) noexcept;

public:
    /// Disable automatic cleanup and return the directory path.
    [[nodiscard]] auto release() noexcept -> Path;
    /// Remove the temporary directory now.
    /// @return `Result::Success` if the directory was removed, `Result::Failure` otherwise.
    auto remove() noexcept -> util::Result;
    /// Remove the temporary directory now.
    /// @throws PathError if removing the directory failed.
    void removeOrThrow();

private:
    /// Create a temporary directory handle for an existing path.
    TempDirectory(Path path, bool removeOnDestroy);

private:
    Path _path;
    bool _removeOnDestroy{true};
};

}
