// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"
#include "PathAccessProfile.hpp"
#include "PathAttribute.hpp"
#include "PathChangeOptions.hpp"
#include "PathCopyOptions.hpp"
#include "PathCreateDirectoryOptions.hpp"
#include "PathCreateFileOptions.hpp"
#include "PathError_fwd.hpp"
#include "PathMoveOptions.hpp"
#include "PathOperations_fwd.hpp"
#include "PathProgress.hpp"
#include "PathRemoveOptions.hpp"
#include "PathTempDirectoryOptions.hpp"
#include "PathTempFileOptions.hpp"
#include "PathWriteTextOptions.hpp"
#include "TempDirectory_fwd.hpp"

#include "impl/PathOperations_fwd.hpp"

#include "../stream/TempByteOutputStream_fwd.hpp"
#include "../stream/TempTextOutputStream_fwd.hpp"
#include "../time/DateTime_fwd.hpp"
#include "../util/Result.hpp"

namespace erbsland::path {

/// A class for performing operations on paths.
/// @tested{PathOperationsTest PathTemporaryTest}
class PathOperations final {
public:
    /// Create an empty/invalid path operations instance.
    PathOperations();
    /// Create a path operations instance for the given path.
    explicit PathOperations(const Path &path);
    /// dtor
    ~PathOperations();

    // defaults/deletions
    PathOperations(const PathOperations &) = delete;
    PathOperations(PathOperations &&) noexcept;

    // defaults/deletions
    auto operator=(const PathOperations &) -> PathOperations & = delete;
    /// Move another path-operations instance into this instance.
    auto operator=(PathOperations &&) noexcept -> PathOperations &;

public: // attributes
    /// Test if the path is empty.
    [[nodiscard]] auto isEmpty() const -> bool;
    /// Access the underlying path.
    [[nodiscard]] auto path() const -> const Path &;

public:
    /// Remove a file or directory.
    /// @param options Options for the remove operation.
    /// @param progressFn Optional progress callback.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto remove(PathRemoveOptions options = {}, const PathProgressFn &progressFn = {}) noexcept -> util::Result;
    /// Remove a file or directory.
    /// @param options Options for the remove operation.
    /// @param progressFn Optional progress callback.
    /// @throws PathError if the operation failed, unless `IgnoreErrors` is set.
    void removeOrThrow(PathRemoveOptions options = {}, const PathProgressFn &progressFn = {});
    /// Copy a file or directory.
    /// @param destination The destination path.
    /// @param options Options for the copy operation.
    /// @param progressFn Optional progress callback.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto copyTo(
        const Path &destination, PathCopyOptions options = {}, const PathProgressFn &progressFn = {}) const noexcept
        -> util::Result;
    /// Copy a file or directory.
    /// @param destination The destination path.
    /// @param options Options for the copy operation.
    /// @param progressFn Optional progress callback.
    /// @throws PathError if the operation failed, unless `IgnoreErrors` is set.
    void copyToOrThrow(
        const Path &destination, PathCopyOptions options = {}, const PathProgressFn &progressFn = {}) const;
    /// Moves/renames a path.
    /// @param destination The destination path. Must be on the same filesystem.
    /// @param options Options for the move operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto moveTo(const Path &destination, PathMoveOptions options = {}) const noexcept -> util::Result;
    /// Moves/renames a path.
    /// @param destination The destination path. Must be on the same filesystem.
    /// @param options Options for the move operation.
    /// @throws PathError if the operation failed, unless `IgnoreErrors` is set.
    void moveToOrThrow(const Path &destination, PathMoveOptions options = {}) const;
    /// Create an empty file.
    /// @param options Options for the operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto createFile(PathCreateFileOptions options = {}) const noexcept -> util::Result;
    /// Create an empty file.
    /// @param options Options for the operation.
    /// @throws PathError if the operation failed, unless `IgnoreErrors` is set.
    void createFileOrThrow(PathCreateFileOptions options = {}) const;
    /// Create a directory.
    /// @param options Options for the operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto createDirectory(PathCreateDirectoryOptions options = {}) const noexcept -> util::Result;
    /// Create a directory.
    /// @param options Options for the operation.
    /// @throws PathError if the operation failed, unless `IgnoreErrors` is set.
    void createDirectoryOrThrow(PathCreateDirectoryOptions options = {}) const;
    /// Create a temporary directory under this path.
    /// @param options Options for the operation.
    /// @return A shared temporary directory handle, or `nullptr` on error.
    [[nodiscard]] auto createTempDirectory(PathTempDirectoryOptions options = {}) const noexcept -> TempDirectoryPtr;
    /// Create a temporary directory under this path.
    /// @param options Options for the operation.
    /// @return A shared temporary directory handle.
    /// @throws PathError if the operation failed.
    [[nodiscard]] auto createTempDirectoryOrThrow(PathTempDirectoryOptions options = {}) const -> TempDirectoryPtr;
    /// Create and open a temporary byte output stream under this path.
    /// @param options Options for the operation.
    /// @return A temporary byte output stream, or `nullptr` on error.
    [[nodiscard]] auto openTempByteOutputStream(PathTempFileOptions options = {}) const noexcept
        -> stream::TempByteOutputStreamPtr;
    /// Create and open a temporary byte output stream under this path.
    /// @param options Options for the operation.
    /// @return A temporary byte output stream.
    /// @throws PathError if the operation failed.
    [[nodiscard]] auto openTempByteOutputStreamOrThrow(PathTempFileOptions options = {}) const
        -> stream::TempByteOutputStreamPtr;
    /// Create and open a temporary text output stream under this path.
    /// @param temporaryOptions Options for creating the temporary file.
    /// @param writeOptions Options for writing encoded text.
    /// @return A temporary text output stream, or `nullptr` on error.
    [[nodiscard]] auto openTempTextOutputStream(
        PathTempFileOptions temporaryOptions = {}, PathWriteTextOptions writeOptions = {}) const noexcept
        -> stream::TempTextOutputStreamPtr;
    /// Create and open a temporary text output stream under this path.
    /// @param temporaryOptions Options for creating the temporary file.
    /// @param writeOptions Options for writing encoded text.
    /// @return A temporary text output stream.
    /// @throws PathError if the operation failed.
    [[nodiscard]] auto openTempTextOutputStreamOrThrow(
        PathTempFileOptions temporaryOptions = {}, PathWriteTextOptions writeOptions = {}) const
        -> stream::TempTextOutputStreamPtr;
    /// Set a portable access profile for this path.
    /// @param profile The access profile to apply.
    /// @param options Options for the operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto setAccessProfile(PathAccessProfile profile, PathChangeOptions options = {}) const noexcept -> util::Result;
    /// Set a portable access profile for this path.
    /// @param profile The access profile to apply.
    /// @param options Options for the operation.
    /// @throws PathError if the operation failed.
    void setAccessProfileOrThrow(PathAccessProfile profile, PathChangeOptions options = {}) const;
    /// Set the last-modification time for this path.
    /// @param value The new modification time.
    /// @param options Options for the operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto setLastModified(const time::DateTime &value, PathChangeOptions options = {}) const noexcept -> util::Result;
    /// Set the last-modification time for this path.
    /// @param value The new modification time.
    /// @param options Options for the operation.
    /// @throws PathError if the time is invalid or the operation failed.
    void setLastModifiedOrThrow(const time::DateTime &value, PathChangeOptions options = {}) const;
    /// Add native path attributes.
    /// @param attributes The attributes to add.
    /// @param options Options for the operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto addAttributes(PathAttributes attributes, PathChangeOptions options = {}) const noexcept -> util::Result;
    /// Add native path attributes.
    /// @param attributes The attributes to add.
    /// @param options Options for the operation.
    /// @throws PathError if the operation failed.
    void addAttributesOrThrow(PathAttributes attributes, PathChangeOptions options = {}) const;
    /// Clear native path attributes.
    /// @param attributes The attributes to clear.
    /// @param options Options for the operation.
    /// @return `Result::Success` if the operation was successful, `Result::Failure` otherwise.
    auto clearAttributes(PathAttributes attributes, PathChangeOptions options = {}) const noexcept -> util::Result;
    /// Clear native path attributes.
    /// @param attributes The attributes to clear.
    /// @param options Options for the operation.
    /// @throws PathError if the operation failed.
    void clearAttributesOrThrow(PathAttributes attributes, PathChangeOptions options = {}) const;

private:
    /// Test whether an error reports an existing path.
    [[nodiscard]] static auto isAlreadyExistsError(const PathError &error) noexcept -> bool;

private:
    impl::PathOperationsPtr _impl;
};

}
