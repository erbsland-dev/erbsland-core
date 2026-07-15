// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path.hpp"
#include "PathAccessInfo.hpp"
#include "PathAttribute.hpp"
#include "PathInfoParts.hpp"
#include "PathType.hpp"

#include "impl/PathInfoData.hpp"

#include "../system/GroupId.hpp"
#include "../system/GroupName.hpp"
#include "../system/UserId.hpp"
#include "../system/UserName.hpp"
#include "../time/DateTime.hpp"
#include "../unit/ByteLength.hpp"

namespace erbsland::path {

/// Information about a file or directory.
/// Caching:
/// - All information for the given path is cached for performance reasons.
/// - A call of `reload()` invalidates cached information immediately and triggers path resolving.
/// - Without manually calling `reload()`, the cache is automatically invalidated after one second.
/// Empty/Unresolved/Non-Existing Behavior:
/// - exists() returns `false`.
/// - resolvedPath() returns an empty path.
/// - type() returns `PathType::Unknown` and therefore all related `is...()` functions return `false`.
/// - fileSize() returns `0`.
/// - All time functions return `DateTime::isValid() == false`.
/// - All owner/group functions either throw an error or return an empty typed value.
/// Changing State between Calls:
/// - If at a point an existing path vanishes or gets inaccessible (causing an error while looking up new
///   information), the path info invalidates the cached information and behaves like an unresolved path.
/// - Only `reload()` can bring a path info instance back to a valid state.
/// Symlinks:
/// - The path information never follows symbolic links.
/// - It uses `PathResolveMode::PhysicalNoFinalSymlink` to resolve the path.
/// - If you need to follow symbolic links, resolve the path yourself before accessing the path info.
/// @tested{PathInfoTest PosixPathInfoTest WindowsPathInfoTest}
class PathInfo final {
public:
    /// Creates an empty path info instance.
    PathInfo() = default;

    /// Create a new path info instance for the given path.
    /// The given path is resolved to a canonical absolute path if this is not already the case.
    /// If resolving fails, the path info behaves like empty path info.
    /// Yet `path()` returns the original path passed to the constructor, and therefore calling `reload()` may
    /// resolve the path again.
    /// @param path The path to get information about.
    /// @param parts The parts to initially request and cache.
    explicit PathInfo(const Path &path, PathInfoParts parts = PathInfoPart::Default) noexcept;

    // defaults
    ~PathInfo() = default;
    PathInfo(const PathInfo &) = default;
    PathInfo(PathInfo &&) = default;
    auto operator=(const PathInfo &) -> PathInfo & = default;
    auto operator=(PathInfo &&) -> PathInfo & = default;

public: // main attributes
    /// Test if the path behind this info is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Access the original path passed to the constructor.
    [[nodiscard]] auto path() const noexcept -> const Path &;
    /// Test if the path info is in a valid state.
    /// The path info is valid if the path exists and could be resolved, and the cached information is valid.
    /// Access the resolved physical absolute path.
    /// Empty if the path does not exist and resolving failed.
    [[nodiscard]] auto resolvedPath() const noexcept -> const Path &;

public: // tests
    /// Test if this path exists.
    [[nodiscard]] auto exists() const noexcept -> bool;
    /// Test if this path is a directory.
    [[nodiscard]] auto isDirectory() const noexcept -> bool { return type() == PathType::Directory; }
    /// Test if this is a regular file.
    [[nodiscard]] auto isRegularFile() const noexcept -> bool { return type() == PathType::RegularFile; }
    /// Test if this is a symlink.
    [[nodiscard]] auto isSymlink() const noexcept -> bool { return type() == PathType::Symlink; }
    /// Test if this is a device.
    [[nodiscard]] auto isDevice() const noexcept -> bool { return type() == PathType::Device; }
    /// Test if this is a socket.
    [[nodiscard]] auto isSocket() const noexcept -> bool { return type() == PathType::Socket; }
    /// Test if this is a pipe.
    [[nodiscard]] auto isPipe() const noexcept -> bool { return type() == PathType::Pipe; }
    /// Test if this is a reparse point.
    [[nodiscard]] auto isReparsePoint() const noexcept -> bool { return type() == PathType::ReparsePoint; }
    /// Test if the current process can read this path.
    [[nodiscard]] auto isReadable() const noexcept -> bool;
    /// Test if the current process can write this path.
    [[nodiscard]] auto isWritable() const noexcept -> bool;
    /// Test if the current process can execute or traverse this path.
    [[nodiscard]] auto isExecutable() const noexcept -> bool;

public: // attributes
    /// Get the type of the resource behind this path.
    [[nodiscard]] auto type() const noexcept -> PathType;
    /// The size of the file in bytes.
    /// @return The size of the file in bytes, or zero if the file does not exist or the path is no file.
    [[nodiscard]] auto fileSize() const noexcept -> unit::ByteLength;
    /// Get the last modified time.
    /// It is available on all platforms.
    /// The returned time is always in the UTC time zone.
    /// @return The date/time, or an invalid date/time if the file/directory does not exist or the attributes
    ///    cannot be obtained.
    [[nodiscard]] auto lastModified() const noexcept -> time::DateTime;
    /// Get the last accessed time.
    /// It is available on all platforms.
    /// The returned time is always in the UTC time zone.
    /// @return The date/time, or an invalid date/time if the file/directory does not exist or the attributes
    ///    cannot be obtained.
    [[nodiscard]] auto lastAccessed() const noexcept -> time::DateTime;
    /// Get the creation time, when the file/directory originally was created.
    /// It is not available on all Linux filesystems/kernel-versions.
    /// The returned time is always in the UTC time zone.
    /// @return The date/time, or an invalid date/time if the file/directory does not exist or the attributes
    ///    cannot be obtained.
    [[nodiscard]] auto birthTime() const noexcept -> time::DateTime;
    /// Get the time of the last metadata change.
    /// This is not available on Windows.
    /// The returned time is always in the UTC time zone.
    /// @return The date/time, or an invalid date/time if the file/directory does not exist or the attributes
    ///    cannot be obtained.
    [[nodiscard]] auto lastMetadataChange() const noexcept -> time::DateTime;
    /// Get the best effort "create" time.
    /// This function tries to get the creation time of the file/directory.
    /// If this time isn't available for this platform, it tries the next best equivalent:
    /// falling back to the last metadata change, falling back to the last modified time.
    /// @return The date/time, or an invalid date/time if the file/directory does not exist or the attributes
    ///    cannot be obtained.
    [[nodiscard]] auto creationTime() const noexcept -> time::DateTime;
    /// Get the name of the owner of the file/directory.
    /// Posix: This is the username.
    /// Windows: This is the resolved username with a separate domain.
    /// @return The name of the owner or an empty name if it does not exist or cannot be obtained.
    [[nodiscard]] auto ownerName() const noexcept -> system::UserName;
    /// Get the name of the owner of the file/directory.
    /// @throws PathError If the owner cannot be obtained.
    /// @return The name of the owner
    [[nodiscard]] auto ownerNameOrThrow() const -> system::UserName;
    /// Get the identifier of the owner of the file/directory.
    /// Posix: This is the UID.
    /// Windows: This is the SID.
    /// @return The identifier of the owner or an empty identifier if it does not exist or cannot be obtained.
    [[nodiscard]] auto ownerId() const noexcept -> system::UserId;
    /// Get the identifier of the owner of the file/directory.
    /// @throws PathError If the owner cannot be obtained.
    /// @return The identifier of the owner.
    [[nodiscard]] auto ownerIdOrThrow() const -> system::UserId;
    /// Get the owning group for the file/directory.
    /// Posix: This is the groupname.
    /// Windows: This is the resolved groupname with a separate domain.
    /// @return The name of the group or an empty name if it does not exist or cannot be obtained.
    [[nodiscard]] auto groupName() const noexcept -> system::GroupName;
    /// Get the owning group for the file/directory.
    /// @throws PathError If the group cannot be obtained.
    /// @return The name of the group.
    [[nodiscard]] auto groupNameOrThrow() const -> system::GroupName;
    /// Get the identifier of the owning group of the file/directory.
    /// Posix: This is the GID.
    /// Windows: This is the SID.
    /// @return The identifier of the group or an empty identifier if it does not exist or cannot be obtained.
    [[nodiscard]] auto groupId() const noexcept -> system::GroupId;
    /// Get the identifier of the owning group of the file/directory.
    /// @throws PathError If the group cannot be obtained.
    /// @return The identifier of the group.
    [[nodiscard]] auto groupIdOrThrow() const -> system::GroupId;
    /// Get portable access information.
    [[nodiscard]] auto accessInfo() const noexcept -> PathAccessInfo;
    /// Get portable access information.
    /// @throws PathError If access information cannot be obtained.
    [[nodiscard]] auto accessInfoOrThrow() const -> PathAccessInfo;
    /// Get native path attributes.
    [[nodiscard]] auto attributes() const noexcept -> PathAttributes;
    /// Get native path attributes.
    /// @throws PathError If native attributes cannot be obtained.
    [[nodiscard]] auto attributesOrThrow() const -> PathAttributes;
    /// Test if this path has the given native attribute.
    [[nodiscard]] auto hasAttribute(PathAttribute attribute) const noexcept -> bool;

public:
    /// Reload the information about the file or directory of this path.
    /// This will use the original `path`, retry canonicalization, and refresh the information.
    void reload();
    /// Reload and preload the given information parts.
    /// This will use the original `path`, retry canonicalization, and refresh the information.
    void reload(PathInfoParts parts);

private:
    [[nodiscard]] auto data() const noexcept -> const impl::PathInfoData *;
    [[nodiscard]] auto mutableData() const noexcept -> impl::PathInfoData *;
    void ensureParts(PathInfoParts parts) const noexcept;
    void ensurePartsOrThrow(PathInfoParts parts, bool forceReload = false) const;
    void resolveNames(PathInfoParts parts) const;

private:
    mutable impl::PathInfoDataPtr _data;
};

}
