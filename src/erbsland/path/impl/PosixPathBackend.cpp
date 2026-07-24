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
#include "../../system/GroupId.hpp"
#include "../../system/UserId.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../text/impl/UnsafeU8StringEditorAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../time/impl/PosixTimeConverter.hpp"
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

#include <algorithm>
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

auto PosixPathBackend::currentDirectoryOrThrow() const -> Path {
    auto bufferSize = std::size_t{1024U};
    while (true) {
        auto buffer = text::impl::UnsafeU8StringBuffer{bufferSize};
        if (::getcwd(buffer.data(), buffer.dataSize()) != nullptr) {
            const auto length = unit::ByteLength::fromSizeT(std::strlen(buffer.data()));
            return Path::fromPosixOrThrow(buffer.take(length));
        }
        if (errno != ERANGE) {
            throwSystemError(
                "Current directory is unavailable"_el,
                "The operating system could not determine the application's current directory."_el,
                {},
                errno);
        }
        bufferSize *= 2U;
    }
}

auto PosixPathBackend::userHomeDirectoryOrThrow() const -> Path {
    const auto configuredBufferSize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    auto bufferSize =
        configuredBufferSize > 1024L ? static_cast<std::size_t>(configuredBufferSize) : std::size_t{1024U};
    while (true) {
        auto buffer = std::make_unique<char[]>(bufferSize);
        auto password = passwd{};
        auto *result = static_cast<passwd *>(nullptr);
        const auto status = ::getpwuid_r(::geteuid(), &password, buffer.get(), bufferSize, &result);
        if (status == ERANGE) {
            bufferSize *= 2U;
            continue;
        }
        if (status != 0) {
            throwSystemError(
                "User home directory is unavailable"_el,
                "The operating system could not resolve the effective user's account."_el,
                {},
                status);
        }
        if (result == nullptr || result->pw_dir == nullptr || result->pw_dir[0] == '\0') {
            throwSystemError(
                "User home directory is unavailable"_el,
                "The effective user's account does not provide a home directory."_el,
                {},
                ENOENT);
        }
        const auto home = Path::fromPosix(text::String{std::string_view{result->pw_dir}});
        if (home.isEmpty() || !home.isAbsolute()) {
            throw PathError{PathErrorContext{
                "User home directory is unavailable"_el,
                "The effective user's account contains an invalid home-directory path."_el}};
        }
        return home;
    }
}

auto PosixPathBackend::systemTempDirectoryOrThrow() const -> Path {
    if (const auto *environmentPath = std::getenv("TMPDIR"); environmentPath != nullptr && environmentPath[0] != '\0') {
        const auto path = Path::fromPosix(text::String{std::string_view{environmentPath}});
        if (!path.isEmpty()) {
            const auto absolutePath = path.toAbsolute();
            if (!absolutePath.isEmpty()) {
                const auto pathText = pathTextOrThrow(absolutePath);
                const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
                struct stat info{};
                if (::stat(pathAccess.data(), &info) == 0 && S_ISDIR(info.st_mode) != 0) {
                    return absolutePath;
                }
            }
        }
    }
    const auto fallback = Path::fromPosix("/tmp"_el);
    const auto fallbackText = pathTextOrThrow(fallback);
    const auto fallbackAccess = text::impl::UnsafeU8StringAccess{fallbackText};
    struct stat fallbackInfo{};
    errno = 0;
    const auto fallbackResult = ::stat(fallbackAccess.data(), &fallbackInfo);
    if (fallbackResult == 0 && S_ISDIR(fallbackInfo.st_mode) != 0) {
        return fallback;
    }
    const auto errorCode = fallbackResult == 0 ? ENOTDIR : errno;
    throwSystemError(
        "Temporary directory is unavailable"_el,
        "No usable system temporary directory could be found."_el,
        fallback,
        errorCode);
}

auto PosixPathBackend::resolveOrThrow(const Path &path, const PathResolveOptions options) const -> Path {
    auto absolutePath = absoluteLexicalPathOrThrow(path);
    switch (options.mode()) {
    case PathResolveMode::Lexical:
        return absolutePath;
    case PathResolveMode::Weak:
        return weakPathOrThrow(absolutePath);
    case PathResolveMode::PhysicalNoFinalSymlink:
        return physicalNoFinalSymlinkPathOrThrow(absolutePath);
    case PathResolveMode::Physical:
        return physicalPathOrThrow(absolutePath);
    }
    throw PathError{
        PathErrorContext{"Path cannot be resolved"_el, "The selected path resolution mode is not supported."_el}
            .setSourcePath(path.toString())
            .setHelp("Select one of the supported path resolution modes."_el)};
}

auto PosixPathBackend::loadInfoOrThrow(const Path &path, const PathInfoParts parts) const -> PathInfoData {
    return loadResolvedInfoOrThrow(path, resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink), parts);
}

auto PosixPathBackend::loadResolvedInfoOrThrow(
    [[maybe_unused]] const Path &path, const Path &resolvedPath, const PathInfoParts parts) const -> PathInfoData {
    auto result = PathInfoData{};
    result.resolvedPath = resolvedPath;

    const auto pathText = pathTextOrThrow(result.resolvedPath);
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.data(), &info) != 0) {
        throwSystemError(
            "Path information is unavailable"_el,
            "The operating system could not provide information about the path."_el,
            result.resolvedPath,
            errno);
    }

    result.exists = true;
    result.type = typeFromMode(info.st_mode);
    result.loadedParts.set(PathInfoPart::Type);
    if (parts.isSet(PathInfoPart::Size)) {
        if (result.type == PathType::RegularFile && info.st_size >= 0) {
            result.fileSize = unit::ByteLength::fromSizeT(static_cast<std::size_t>(info.st_size));
        }
        result.loadedParts.set(PathInfoPart::Size);
    }
    if (parts.isSet(PathInfoPart::Times)) {
#ifdef ERBSLAND_OS_MACOS
        result.lastModified = time::impl::PosixTimeConverter::fromTimespec(info.st_mtimespec);
        result.lastAccessed = time::impl::PosixTimeConverter::fromTimespec(info.st_atimespec);
        result.birthTime = time::impl::PosixTimeConverter::fromTimespec(info.st_birthtimespec);
        result.lastMetadataChange = time::impl::PosixTimeConverter::fromTimespec(info.st_ctimespec);
#elif defined(ERBSLAND_OS_LINUX)
        result.lastModified = time::impl::PosixTimeConverter::fromTimespec(info.st_mtim);
        result.lastAccessed = time::impl::PosixTimeConverter::fromTimespec(info.st_atim);
        result.lastMetadataChange = time::impl::PosixTimeConverter::fromTimespec(info.st_ctim);
#endif
        result.loadedParts.set(PathInfoPart::Times);
    }
    if (parts.isSet(PathInfoPart::OwnerId)) {
        result.ownerId = system::UserId{text::String::fromInteger(static_cast<unsigned long>(info.st_uid))};
        result.loadedParts.set(PathInfoPart::OwnerId);
    }
    if (parts.isSet(PathInfoPart::GroupId)) {
        result.groupId = system::GroupId{text::String::fromInteger(static_cast<unsigned long>(info.st_gid))};
        result.loadedParts.set(PathInfoPart::GroupId);
    }
    if (parts.isSet(PathInfoPart::AccessRights)) {
        result.accessInfo = accessInfoFromStatus(info, result.type);
        result.loadedParts.set(PathInfoPart::AccessRights);
    }
    if (parts.isSet(PathInfoPart::Attributes)) {
#ifdef ERBSLAND_OS_MACOS
        if ((info.st_flags & UF_IMMUTABLE) != 0U) {
            result.attributes.set(PathAttribute::Immutable);
        }
        if ((info.st_flags & UF_HIDDEN) != 0U) {
            result.attributes.set(PathAttribute::Hidden);
        }
#elif defined(ERBSLAND_OS_LINUX)
        const auto fileDescriptor = ::open(pathAccess.data(), O_RDONLY | O_NONBLOCK);
        if (fileDescriptor >= 0) {
            auto flags = 0;
            if (::ioctl(fileDescriptor, FS_IOC_GETFLAGS, &flags) == 0 && (flags & FS_IMMUTABLE_FL) != 0) {
                result.attributes.set(PathAttribute::Immutable);
            }
            static_cast<void>(::close(fileDescriptor));
        }
#endif
        result.loadedParts.set(PathInfoPart::Attributes);
    }
    result.lastRefresh = time::TimePoint::now();
    return result;
}

auto PosixPathBackend::openByteInputStreamOrThrow(const Path &path, const PathReadDataOptions options) const
    -> stream::ByteInputStreamPtr {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    const auto fileDescriptor = ::open(pathAccess.data(), O_RDONLY);
    if (fileDescriptor < 0) {
        throwSystemError(
            "File could not be opened for reading"_el,
            "The operating system could not open the file for reading."_el,
            path,
            errno);
    }
    auto native = std::make_shared<stream::impl::PosixNativeStream>(
        fileDescriptor, stream::impl::NativeStreamOwnership::Owned, path.toString());
    return stream::impl::createBufferedByteInputStream(std::move(native), options.streamSettings());
}

void PosixPathBackend::setAccessProfileOrThrow(
    const Path &path, const PathAccessProfile profile, [[maybe_unused]] const PathChangeOptions options) const {
    const auto resolvedPath = resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink);
    const auto pathText = pathTextOrThrow(resolvedPath);
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.data(), &info) != 0) {
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
    if (::chmod(pathAccess.data(), profileMode(profile, type)) != 0) {
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
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    const auto existed = existingPath(path);
    const auto fileDescriptor =
        ::open(pathAccess.data(), flags, profileMode(options.accessProfile(), PathType::RegularFile));
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
        static_cast<void>(::close(fileDescriptor));
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
        const auto directoryAccess = text::impl::UnsafeU8StringAccess{directoryText};
        if (::mkdir(directoryAccess.data(), static_cast<mode_t>(0777)) == 0) {
            continue;
        }
        auto errorCode = errno;
        if (errorCode == EEXIST) {
            struct stat info{};
            if (::stat(directoryAccess.data(), &info) == 0 && S_ISDIR(info.st_mode)) {
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
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.data(), &info) != 0) {
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
    if (::chflags(pathAccess.data(), flags) != 0) {
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
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    const auto fileDescriptor = ::open(pathAccess.data(), O_RDONLY | O_NONBLOCK);
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
