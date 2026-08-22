// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CommonPathBackend.hpp"

#include "../PathAccessInfo.hpp"
#include "../PathAccessProfile.hpp"
#include "../PathType.hpp"

#include "../../text/StringEditor.hpp"
#include "../../text/StringList.hpp"

#include <sys/stat.h>
#include <sys/types.h>

namespace erbsland::path::impl {

/// The POSIX path backend.
/// @tested{PosixPathOperationsTest}
class PosixPathBackend : public CommonPathBackend {
public:
    /// Create the POSIX path backend.
    PosixPathBackend() = default;

public: // implement PathBackend
    [[nodiscard]] auto currentDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto userHomeDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto systemTempDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto resolveOrThrow(const Path &path, PathResolveOptions options) const -> Path override;
    [[nodiscard]] auto loadInfoOrThrow(const Path &path, PathInfoParts parts) const -> PathInfoData override;
    [[nodiscard]] auto loadResolvedInfoOrThrow(const Path &path, const Path &resolvedPath, PathInfoParts parts) const
        -> PathInfoData override;
    [[nodiscard]] auto directoryEntriesOrThrow(const Path &path, const Path &resolvedPath) const
        -> std::vector<Path> override;
    void createDirectoryEntryOrThrow(const Path &path, PathAccessProfile profile) const override;
    void removeEntryOrThrow(const Path &path) const override;
    void copyFileEntryOrThrow(const Path &source, const Path &destination) const override;
    void moveEntryOrThrow(const Path &source, const Path &destination) const override;
    [[nodiscard]] auto readSymlinkOrThrow(const Path &path) const -> Path override;
    void createSymlinkOrThrow(const Path &target, const Path &path, bool targetIsDirectory) const override;
    [[nodiscard]] auto openByteInputStreamOrThrow(const Path &path, PathReadDataOptions options) const
        -> stream::ByteInputStreamPtr override;
    void setAccessProfileOrThrow(const Path &path, PathAccessProfile profile, PathChangeOptions options) const override;
    void addAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const override;
    void clearAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const override;

private:
    /// Convert a path to native POSIX text.
    [[nodiscard]] static auto pathTextOrThrow(const Path &path) -> text::String;
    /// Create all missing parent directories for a path.
    static void createParentDirectoriesOrThrow(const Path &path);
    /// Test if an open file descriptor has readable content.
    [[nodiscard]] static auto fileDescriptorHasContentOrThrow(int fileDescriptor, const Path &path) -> bool;
    /// Open an input descriptor using the configured symbolic-link policy.
    [[nodiscard]] static auto openInputFileDescriptorOrThrow(const Path &path, SymlinkMode symlinkMode) -> int;
    /// Map POSIX file mode bits to a path type.
    [[nodiscard]] static auto typeFromMode(mode_t mode) noexcept -> PathType;
    /// Map a POSIX directory entry type to a path type.
    [[nodiscard]] static auto typeFromDirectoryEntry(unsigned char type) noexcept -> PathType;
    /// Create access information from POSIX status data.
    [[nodiscard]] static auto accessInfoFromStatus(const struct stat &info, PathType type) -> PathAccessInfo;
    /// Extract access rights from a POSIX file mode.
    [[nodiscard]] static auto rightsFromMode(mode_t mode, unsigned int shift) noexcept -> PathAccessRights;
    /// Convert a path access profile to POSIX mode bits.
    [[nodiscard]] static auto profileMode(PathAccessProfile profile, PathType type) noexcept -> mode_t;
    /// Apply an access profile to an open file descriptor.
    static void applyAccessProfileToDescriptorOrThrow(
        int fileDescriptor, const Path &path, PathAccessProfile profile, PathType type);
    /// Set or clear the requested POSIX attributes.
    static void applyAttributesOrThrow(const Path &path, PathAttributes attributes, bool set);
    /// Resolve a path physically, including its final symbolic link.
    [[nodiscard]] static auto physicalPathOrThrow(const Path &path) -> Path;
    /// Test whether a path currently exists.
    [[nodiscard]] static auto existingPath(const Path &path) -> bool;
    /// Resolve a path without requiring its final component to exist.
    [[nodiscard]] static auto weakPathOrThrow(const Path &path) -> Path;
    /// Resolve a path while preserving its final symbolic link.
    [[nodiscard]] static auto physicalNoFinalSymlinkPathOrThrow(const Path &path) -> Path;
    /// Throw a path error for one affected path.
    [[noreturn]] static void throwSystemError(
        const text::String &title, const text::String &description, const Path &path, int errorCode);
    /// Throw a path error for a source and destination path.
    [[noreturn]] static void throwSystemError(
        const text::String &title,
        const text::String &description,
        const Path &source,
        const Path &destination,
        int errorCode);

protected:
    [[nodiscard]] auto openByteOutputStreamWithExistingContentOrThrow(
        const Path &path, PathWriteDataOptions options) const -> PathByteOutputStreamOpenResult override;
};

}
