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
    if (const auto environmentPath = system::EnvironmentVariables{}.get("TMPDIR"_el);
        environmentPath.has_value() && !environmentPath->isEmpty()) {
        const auto path = Path::fromPosix(*environmentPath);
        if (!path.isEmpty()) {
            const auto absolutePath = path.toAbsolute();
            if (!absolutePath.isEmpty()) {
                const auto pathText = pathTextOrThrow(absolutePath);
                const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
                struct stat info{};
                if (::stat(pathAccess.nullTerminatedCharPtr(), &info) == 0 && S_ISDIR(info.st_mode) != 0) {
                    return absolutePath;
                }
            }
        }
    }
    const auto fallback = Path::fromPosix("/tmp"_el);
    const auto fallbackText = pathTextOrThrow(fallback);
    const auto fallbackAccess = text::impl::PlatformU8StringAccess{fallbackText};
    struct stat fallbackInfo{};
    errno = 0;
    const auto fallbackResult = ::stat(fallbackAccess.nullTerminatedCharPtr(), &fallbackInfo);
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
    const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.nullTerminatedCharPtr(), &info) != 0) {
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
        const auto dateTimeFromTimespec = [](const timespec &value) noexcept -> time::DateTime {
            return time::DateTime::fromTicks(
                time::Seconds{value.tv_sec}, time::Nanoseconds{value.tv_nsec}, time::TimeEpoch::Posix)
                .value_or(time::DateTime{});
        };
#ifdef ERBSLAND_OS_MACOS
        result.lastModified = dateTimeFromTimespec(info.st_mtimespec);
        result.lastAccessed = dateTimeFromTimespec(info.st_atimespec);
        result.birthTime = dateTimeFromTimespec(info.st_birthtimespec);
        result.lastMetadataChange = dateTimeFromTimespec(info.st_ctimespec);
#elif defined(ERBSLAND_OS_LINUX)
        result.lastModified = dateTimeFromTimespec(info.st_mtim);
        result.lastAccessed = dateTimeFromTimespec(info.st_atim);
        result.lastMetadataChange = dateTimeFromTimespec(info.st_ctim);
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
        const auto fileDescriptor = ::open(pathAccess.nullTerminatedCharPtr(), O_RDONLY | O_NONBLOCK);
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
    const auto fileDescriptor = openInputFileDescriptorOrThrow(path, options.symlinkMode());
    auto native = std::make_shared<stream::impl::PosixNativeStream>(
        fileDescriptor, stream::impl::NativeStreamOwnership::Owned, path.toString());
    return stream::impl::createBufferedByteInputStream(std::move(native), options.streamSettings());
}

auto PosixPathBackend::openInputFileDescriptorOrThrow(const Path &path, const SymlinkMode symlinkMode) -> int {
    if (symlinkMode == SymlinkMode::Follow) {
        const auto pathText = pathTextOrThrow(path);
        const auto pathAccess = text::impl::PlatformU8StringAccess{pathText};
        const auto fileDescriptor = ::open(pathAccess.nullTerminatedCharPtr(), O_RDONLY | O_CLOEXEC);
        if (fileDescriptor >= 0) {
            return fileDescriptor;
        }
        throwSystemError(
            "File could not be opened for reading"_el,
            "The operating system could not open the file for reading."_el,
            path,
            errno);
    }

    const auto absolutePath = path.toAbsoluteOrThrow();
    auto descriptor = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) {
        throwSystemError(
            "File could not be opened for reading"_el,
            "The filesystem root could not be opened for a symbolic-link-safe traversal."_el,
            path,
            errno);
    }
    const auto elements = absolutePath.elements();
    for (auto index = unit::ItemIndex::one(); index.isWithin(elements.count()); ++index) {
        const auto name = elements.get(index);
        const auto nameAccess = text::impl::PlatformU8StringAccess{name};
        const auto isFinal = !index.advanced(unit::ItemCount::one()).isWithin(elements.count());
        const auto flags = O_RDONLY | O_CLOEXEC | O_NOFOLLOW | (isFinal ? 0 : O_DIRECTORY);
        const auto next = ::openat(descriptor, nameAccess.nullTerminatedCharPtr(), flags);
        const auto errorCode = errno;
        ::close(descriptor);
        if (next < 0) {
            throwSystemError(
                "File could not be opened for reading"_el,
                "The path could not be opened without following symbolic links."_el,
                path,
                errorCode);
        }
        descriptor = next;
    }
    return descriptor;
}

}
