// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixPathBackend.hpp"

#include "BackendFactory.hpp"
#include "PathInfoData.hpp"

#include "../Path.hpp"
#include "../PathCreateMode.hpp"
#include "../PathError.hpp"
#include "../PathInfo.hpp"
#include "../PathInfoParts.hpp"
#include "../PathResolveMode.hpp"

#include "../../core/Definitions.hpp"
#include "../../stream/impl/BufferedByteOutputStream.hpp"
#include "../../stream/impl/InputStreamFactory.hpp"
#include "../../stream/impl/NativeOutputStream.hpp"
#include "../../stream/impl/PosixNativeStream.hpp"
#include "../../system/EnvironmentVariables.hpp"
#include "../../system/GroupId.hpp"
#include "../../system/UserId.hpp"
#include "../../text/impl/PlatformU8StringAccess.hpp"
#include "../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../time/DateTime.hpp"
#include "../../unit/ByteLength.hpp"

#include <dirent.h>
#include <fcntl.h>

#ifdef ERBSLAND_OS_LINUX
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace erbsland::path::impl {
using namespace text::literals;

void PosixPathBackend::setAccessProfileOrThrow(
    const Path &path, const PathAccessProfile profile, [[maybe_unused]] const PathChangeOptions options) const {
    const auto resolvedPath = resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink);
    const auto pathText = pathTextOrThrow(resolvedPath);
    const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.nullTerminatedCharPtr(), &info) != 0) {
        throwSystemError(
            "Path information is unavailable"_el,
            "The file permissions could not be changed because the path type is unavailable."_el,
            resolvedPath,
            errno);
    }
    const auto type = typeFromMode(info.st_mode);
    if (type == PathType::Symlink) {
        throw PathError{PathErrorContext{
            "Symbolic-link permissions could not be changed"_el,
            "Changing permissions directly on symbolic links is not supported."_el}
                .setSourcePath(resolvedPath.toString())
                .setHelp("Change the permissions on the symbolic link target instead."_el)};
    }
    if (::chmod(pathAccess.nullTerminatedCharPtr(), profileMode(profile, type)) != 0) {
        throwSystemError(
            "File permissions could not be changed"_el,
            "The operating system rejected the requested permission change."_el,
            resolvedPath,
            errno);
    }
    invalidateInfo(path);
}

void PosixPathBackend::addAttributesOrThrow(
    const Path &path, const PathAttributes attributes, [[maybe_unused]] const PathChangeOptions options) const {
    applyAttributesOrThrow(resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink), attributes, true);
    invalidateInfo(path);
}

void PosixPathBackend::clearAttributesOrThrow(
    const Path &path, const PathAttributes attributes, [[maybe_unused]] const PathChangeOptions options) const {
    applyAttributesOrThrow(resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink), attributes, false);
    invalidateInfo(path);
}

auto PosixPathBackend::openByteOutputStreamWithExistingContentOrThrow(
    const Path &path, const PathWriteDataOptions options) const -> PathByteOutputStreamOpenResult {
    if (options.createParents()) {
        createParentDirectoriesOrThrow(path);
    }

    auto flags = O_WRONLY | O_CREAT;
    switch (options.creationMode()) {
    case PathCreateMode::CreateNew:
        flags |= O_EXCL;
        break;
    case PathCreateMode::CreateOrOverwrite:
        flags |= O_TRUNC;
        break;
    case PathCreateMode::CreateOrAppend:
        flags |= O_APPEND;
        break;
    }

    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
    const auto existed = existingPath(path);
    const auto fileDescriptor =
        ::open(pathAccess.nullTerminatedCharPtr(), flags, profileMode(options.accessProfile(), PathType::RegularFile));
    if (fileDescriptor < 0) {
        throwSystemError(
            "File could not be opened for writing"_el,
            "The operating system could not open or create the file for writing."_el,
            path,
            errno);
    }
    try {
        if (!existed) {
            applyAccessProfileToDescriptorOrThrow(fileDescriptor, path, options.accessProfile(), PathType::RegularFile);
        }
        const auto hasExistingContent = fileDescriptorHasContentOrThrow(fileDescriptor, path);
        return {
            std::make_shared<stream::impl::BufferedByteOutputStream>(
                std::make_shared<stream::impl::PosixNativeStream>(
                    fileDescriptor, stream::impl::NativeStreamOwnership::Owned, path.toString()),
                options.streamSettings()),
            hasExistingContent,
        };
    } catch (...) {
        // Preserve the setup error; a close failure cannot safely be retried.
        ::close(fileDescriptor);
        throw;
    }
}

auto PosixPathBackend::pathTextOrThrow(const Path &path) -> text::String {
    const auto pathText = path.toPosix();
    if (pathText.isEmpty()) {
        throw PathError{PathErrorContext{
            "Path cannot be used on this platform"_el, "The path cannot be represented in the POSIX path format."_el}
                .setSourcePath(path.toString())
                .setHelp("Use a path compatible with this operating system."_el)};
    }
    return pathText;
}

void PosixPathBackend::createParentDirectoriesOrThrow(const Path &path) {
    const auto parent = path.parent();
    if (parent.isEmpty() || parent.isRoot()) {
        return;
    }

    auto directories = parent.parents();
    directories.reverse();
    directories.append(parent);
    for (const auto &directory : directories) {
        if (directory.isRoot()) {
            continue;
        }
        const auto directoryText = pathTextOrThrow(directory);
        const auto directoryAccess = text::impl::PlatformU8StringAccess{directoryText};
        if (::mkdir(directoryAccess.nullTerminatedCharPtr(), static_cast<mode_t>(0777)) == 0) {
            continue;
        }
        auto errorCode = errno;
        if (errorCode == EEXIST) {
            struct stat info{};
            if (::stat(directoryAccess.nullTerminatedCharPtr(), &info) == 0 && S_ISDIR(info.st_mode)) {
                continue;
            }
            if (errno != 0) {
                errorCode = errno;
            }
        }
        throwSystemError(
            "Directory could not be created"_el,
            "A parent directory required for the target file could not be created."_el,
            directory,
            errorCode);
    }
}

auto PosixPathBackend::fileDescriptorHasContentOrThrow(const int fileDescriptor, const Path &path) -> bool {
    struct stat info{};
    if (::fstat(fileDescriptor, &info) != 0) {
        throwSystemError(
            "File information is unavailable"_el,
            "The operating system could not inspect the opened file."_el,
            path,
            errno);
    }
    return S_ISREG(info.st_mode) && info.st_size > 0;
}

auto PosixPathBackend::typeFromMode(const mode_t mode) noexcept -> PathType {
    if (S_ISDIR(mode)) {
        return PathType::Directory;
    }
    if (S_ISREG(mode)) {
        return PathType::RegularFile;
    }
    if (S_ISLNK(mode)) {
        return PathType::Symlink;
    }
    if (S_ISCHR(mode) || S_ISBLK(mode)) {
        return PathType::Device;
    }
    if (S_ISSOCK(mode)) {
        return PathType::Socket;
    }
    if (S_ISFIFO(mode)) {
        return PathType::Pipe;
    }
    return PathType::Unknown;
}

auto PosixPathBackend::typeFromDirectoryEntry(const unsigned char type) noexcept -> PathType {
    switch (type) {
    case DT_DIR:
        return PathType::Directory;
    case DT_REG:
        return PathType::RegularFile;
    case DT_LNK:
        return PathType::Symlink;
    case DT_CHR:
    case DT_BLK:
        return PathType::Device;
    case DT_SOCK:
        return PathType::Socket;
    case DT_FIFO:
        return PathType::Pipe;
    default:
        return PathType::Unknown;
    }
}

auto PosixPathBackend::accessInfoFromStatus(const struct stat &info, const PathType type) -> PathAccessInfo {
    const auto ownerRights = rightsFromMode(info.st_mode, 6U);
    const auto groupRights = rightsFromMode(info.st_mode, 3U);
    const auto otherRights = rightsFromMode(info.st_mode, 0U);
    auto currentRights = PathAccessRights{};

    const auto effectiveUserId = ::geteuid();
    if (effectiveUserId == 0) {
        currentRights.set(PathAccessRight::Read);
        currentRights.set(PathAccessRight::Write);
        if (type == PathType::Directory || ownerRights.isSet(PathAccessRight::Execute) ||
            groupRights.isSet(PathAccessRight::Execute) || otherRights.isSet(PathAccessRight::Execute)) {
            currentRights.set(PathAccessRight::Execute);
        }
    } else if (effectiveUserId == info.st_uid) {
        currentRights = ownerRights;
    } else {
        auto groups = std::vector<gid_t>(static_cast<std::size_t>(::getgroups(0, nullptr)));
        if (!groups.empty()) {
            const auto groupCount = ::getgroups(static_cast<int>(groups.size()), groups.data());
            if (groupCount >= 0) {
                groups.resize(static_cast<std::size_t>(groupCount));
            } else {
                groups.clear();
            }
        }
        const auto effectiveGroupId = ::getegid();
        const auto isGroupMember =
            effectiveGroupId == info.st_gid || std::ranges::find(groups, info.st_gid) != groups.end();
        currentRights = isGroupMember ? groupRights : otherRights;
    }

    return PathAccessInfo{}
        .setCurrentProcessRights(currentRights)
        .setHasPortableRights(true)
        .setOwnerRights(ownerRights)
        .setGroupRights(groupRights)
        .setOtherRights(otherRights);
}

auto PosixPathBackend::rightsFromMode(const mode_t mode, const unsigned int shift) noexcept -> PathAccessRights {
    const auto bits = static_cast<unsigned int>((mode >> shift) & 0x07U);
    auto result = PathAccessRights{};
    if ((bits & 0x04U) != 0U) {
        result.set(PathAccessRight::Read);
    }
    if ((bits & 0x02U) != 0U) {
        result.set(PathAccessRight::Write);
    }
    if ((bits & 0x01U) != 0U) {
        result.set(PathAccessRight::Execute);
    }
    return result;
}

auto PosixPathBackend::profileMode(const PathAccessProfile profile, const PathType type) noexcept -> mode_t {
    const auto isDirectory = type == PathType::Directory;
    switch (profile) {
    case PathAccessProfile::Default:
        return isDirectory ? static_cast<mode_t>(0777) : static_cast<mode_t>(0666);
    case PathAccessProfile::UserOnly:
        return isDirectory ? static_cast<mode_t>(0700) : static_cast<mode_t>(0600);
    case PathAccessProfile::UserAndGroup:
        return isDirectory ? static_cast<mode_t>(0770) : static_cast<mode_t>(0660);
    case PathAccessProfile::Everyone:
        return isDirectory ? static_cast<mode_t>(0777) : static_cast<mode_t>(0666);
    }
    return isDirectory ? static_cast<mode_t>(0777) : static_cast<mode_t>(0666);
}

void PosixPathBackend::applyAccessProfileToDescriptorOrThrow(
    const int fileDescriptor, const Path &path, const PathAccessProfile profile, const PathType type) {
    if (profile == PathAccessProfile::Default) {
        return;
    }
    if (::fchmod(fileDescriptor, profileMode(profile, type)) != 0) {
        throwSystemError(
            "File permissions could not be changed"_el,
            "The requested permissions could not be applied to the opened file."_el,
            path,
            errno);
    }
}

void PosixPathBackend::applyAttributesOrThrow(const Path &path, const PathAttributes attributes, const bool set) {
    auto unsupported = attributes;
#ifdef ERBSLAND_OS_MACOS
    unsupported.clear(PathAttribute::Immutable);
    unsupported.clear(PathAttribute::Hidden);
    if (unsupported.hasAny()) {
        throw PathError{PathErrorContext{
            "File attributes could not be changed"_el,
            "One or more requested attributes are not supported on this platform."_el}
                .setSourcePath(path.toString())
                .setHelp("Request only attributes supported by this operating system."_el)};
    }
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.nullTerminatedCharPtr(), &info) != 0) {
        throwSystemError(
            "Path information is unavailable"_el,
            "The file attributes could not be changed because the current path information is unavailable."_el,
            path,
            errno);
    }
    auto flags = info.st_flags;
    if (set) {
        if (attributes.isSet(PathAttribute::Immutable)) {
            flags |= UF_IMMUTABLE;
        }
        if (attributes.isSet(PathAttribute::Hidden)) {
            flags |= UF_HIDDEN;
        }
    } else {
        if (attributes.isSet(PathAttribute::Immutable)) {
            flags &= static_cast<decltype(flags)>(~static_cast<decltype(flags)>(UF_IMMUTABLE));
        }
        if (attributes.isSet(PathAttribute::Hidden)) {
            flags &= static_cast<decltype(flags)>(~static_cast<decltype(flags)>(UF_HIDDEN));
        }
    }
    if (::chflags(pathAccess.nullTerminatedCharPtr(), flags) != 0) {
        throwSystemError(
            "File attributes could not be changed"_el,
            "The operating system rejected the requested attribute change."_el,
            path,
            errno);
    }
#elif defined(ERBSLAND_OS_LINUX)
    unsupported.clear(PathAttribute::Immutable);
    if (unsupported.hasAny()) {
        throw PathError{PathErrorContext{
            "File attributes could not be changed"_el,
            "One or more requested attributes are not supported on this platform."_el}
                .setSourcePath(path.toString())
                .setHelp("Request only attributes supported by this operating system."_el)};
    }
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
    const auto fileDescriptor = ::open(pathAccess.nullTerminatedCharPtr(), O_RDONLY | O_NONBLOCK);
    if (fileDescriptor < 0) {
        throwSystemError(
            "File attributes could not be changed"_el,
            "The path could not be opened for an attribute change."_el,
            path,
            errno);
    }
    auto flags = 0;
    if (::ioctl(fileDescriptor, FS_IOC_GETFLAGS, &flags) != 0) {
        const auto errorCode = errno;
        static_cast<void>(::close(fileDescriptor));
        throwSystemError(
            "File attributes are unavailable"_el,
            "The operating system could not read the current file attributes."_el,
            path,
            errorCode);
    }
    if (set) {
        flags |= FS_IMMUTABLE_FL;
    } else {
        flags &= ~FS_IMMUTABLE_FL;
    }
    if (::ioctl(fileDescriptor, FS_IOC_SETFLAGS, &flags) != 0) {
        const auto errorCode = errno;
        static_cast<void>(::close(fileDescriptor));
        throwSystemError(
            "File attributes could not be changed"_el,
            "The operating system rejected the requested attribute change."_el,
            path,
            errorCode);
    }
    static_cast<void>(::close(fileDescriptor));
#else
    if (unsupported.hasAny()) {
        throw PathError{PathErrorContext{
            "File attributes could not be changed"_el, "Path attributes are not supported on this platform."_el}
                .setSourcePath(path.toString())};
    }
#endif
}
}
