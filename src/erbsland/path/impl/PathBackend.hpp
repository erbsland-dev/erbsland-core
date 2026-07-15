// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Path_fwd.hpp"
#include "../PathAccessProfile.hpp"
#include "../PathAttribute.hpp"
#include "../PathChangeOptions.hpp"
#include "../PathInfoParts.hpp"
#include "../PathReadDataOptions.hpp"
#include "../PathReadTextOptions.hpp"
#include "../PathResolveOptions.hpp"
#include "../PathWriteDataOptions.hpp"
#include "../PathWriteTextOptions.hpp"

#include "../../stream/ByteInputStream_fwd.hpp"
#include "../../stream/ByteOutputStream_fwd.hpp"
#include "../../stream/TextInputStream_fwd.hpp"
#include "../../stream/TextOutputStream_fwd.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace erbsland::path::impl {

class PathBackend;
using PathBackendPtr = std::unique_ptr<PathBackend>;
class PathInfoData;

/// The abstract base class for all path backends.
/// It is responsible for implementing OS-specific path handling and validation.
class PathBackend {
public:
    virtual ~PathBackend() = default;

public:
    /// Return the current working directory.
    /// @return The absolute current working directory.
    /// @throws PathError if the directory cannot be determined or converted.
    [[nodiscard]] virtual auto currentDirectoryOrThrow() const -> Path = 0;
    /// Return the system directory for temporary files and directories.
    /// @return The absolute system temporary directory.
    /// @throws PathError if the directory cannot be determined or converted.
    [[nodiscard]] virtual auto systemTempDirectoryOrThrow() const -> Path = 0;
    /// Resolve a path using the configured resolve mode.
    /// @param path The path to resolve.
    /// @param options The resolve options.
    /// @return The resolved absolute path.
    /// @throws PathError if resolving fails.
    [[nodiscard]] virtual auto resolveOrThrow(const Path &path, PathResolveOptions options) const -> Path = 0;
    /// Convert a path to an absolute path.
    /// @param path The path to convert.
    /// @param base The optional absolute base path.
    /// @return The absolute path.
    /// @throws PathError if conversion fails.
    [[nodiscard]] virtual auto toAbsoluteOrThrow(const Path &path, std::optional<Path> base) const -> Path = 0;
    /// Convert a path to a relative path.
    /// @param path The path to convert.
    /// @param base The optional absolute base path.
    /// @return The relative path.
    /// @throws PathError if conversion fails.
    [[nodiscard]] virtual auto toRelativeOrThrow(const Path &path, std::optional<Path> base) const -> Path = 0;
    /// Test if a path is relative to a base path.
    /// @param path The path to test.
    /// @param base The optional absolute base path.
    /// @return `true` if the path is relative to the base path.
    [[nodiscard]] virtual auto isRelativeTo(const Path &path, std::optional<Path> base) const noexcept -> bool = 0;
    /// Return the common ancestor of a path and a base path.
    /// @param path The path to test.
    /// @param base The optional absolute base path.
    /// @return The common ancestor, or an empty path if there is none.
    [[nodiscard]] virtual auto commonAncestor(const Path &path, std::optional<Path> base) const noexcept -> Path = 0;
    /// Load information for a path.
    /// @param path The path to inspect.
    /// @param parts The requested information parts.
    /// @return The loaded path information data.
    /// @throws PathError if resolving or native metadata access fails.
    [[nodiscard]] virtual auto loadInfoOrThrow(const Path &path, PathInfoParts parts) const -> PathInfoData = 0;
    /// List the direct children of a directory.
    [[nodiscard]] virtual auto directoryEntriesOrThrow(const Path &path) const -> std::vector<Path> = 0;
    /// Create one directory. Parent directories must already exist.
    virtual void createDirectoryEntryOrThrow(const Path &path, PathAccessProfile profile) const = 0;
    /// Remove one empty directory, file, or symbolic link.
    virtual void removeEntryOrThrow(const Path &path) const = 0;
    /// Copy one regular file without following symbolic links.
    virtual void copyFileEntryOrThrow(const Path &source, const Path &destination) const = 0;
    /// Move one path on the same filesystem.
    virtual void moveEntryOrThrow(const Path &source, const Path &destination) const = 0;
    /// Read the stored target of a symbolic link.
    [[nodiscard]] virtual auto readSymlinkOrThrow(const Path &path) const -> Path = 0;
    /// Create a symbolic link with the stored target.
    virtual void createSymlinkOrThrow(const Path &target, const Path &path, bool targetIsDirectory) const = 0;
    /// Open a path as byte input stream.
    /// @param path The path to open.
    /// @param options Options for reading byte data.
    /// @return The opened byte input stream.
    /// @throws PathError if the path cannot be opened for reading.
    [[nodiscard]] virtual auto openByteInputStreamOrThrow(const Path &path, PathReadDataOptions options) const
        -> stream::ByteInputStreamPtr = 0;
    /// Open a path as text input stream.
    /// @param path The path to open.
    /// @param options Options for reading text.
    /// @return The opened text input stream.
    /// @throws PathError if the path cannot be opened for reading.
    [[nodiscard]] virtual auto openTextInputStreamOrThrow(const Path &path, PathReadTextOptions options) const
        -> stream::TextInputStreamPtr = 0;
    /// Open a path as byte output stream.
    /// @param path The path to open.
    /// @param options Options for writing byte data.
    /// @return The opened byte output stream.
    /// @throws PathError if the path cannot be opened for writing.
    [[nodiscard]] virtual auto openByteOutputStreamOrThrow(const Path &path, PathWriteDataOptions options) const
        -> stream::ByteOutputStreamPtr = 0;
    /// Open a path as text output stream.
    /// @param path The path to open.
    /// @param options Options for writing text.
    /// @return The opened text output stream.
    /// @throws PathError if the path cannot be opened for writing.
    [[nodiscard]] virtual auto openTextOutputStreamOrThrow(const Path &path, PathWriteTextOptions options) const
        -> stream::TextOutputStreamPtr = 0;
    /// Set the portable access profile for a path.
    /// @throws PathError if the change fails.
    virtual void setAccessProfileOrThrow(
        const Path &path, PathAccessProfile profile, PathChangeOptions options) const = 0;
    /// Add native attributes for a path.
    /// @throws PathError if the change fails.
    virtual void addAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const = 0;
    /// Clear native attributes for a path.
    /// @throws PathError if the change fails.
    virtual void clearAttributesOrThrow(
        const Path &path, PathAttributes attributes, PathChangeOptions options) const = 0;
};

}
