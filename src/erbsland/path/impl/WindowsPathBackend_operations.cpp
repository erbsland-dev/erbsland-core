// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsPathBackend.hpp"

#include "PathInfoData.hpp"
#include "WindowsAccessProfileSecurity.hpp"

#include "../Path.hpp"
#include "../PathError.hpp"
#include "../PathWindowsFormat.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/impl/PlatformU16StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../time/impl/WindowsTimeConverter.hpp"
#include "../../unit/ByteLength.hpp"

#include <winioctl.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace erbsland::path::impl {

using namespace text::literals;

auto WindowsPathBackend::directoryEntriesOrThrow(const Path &path, const Path &resolvedPath) const
    -> std::vector<Path> {
    auto searchPath = text::StringConverter{pathTextOrThrow(resolvedPath)}.toStdWString();
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath.push_back(L'\\');
    }
    searchPath.push_back(L'*');

    auto findData = WIN32_FIND_DATAW{};
    const auto findHandle = FindFirstFileExW(
        searchPath.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (findHandle == INVALID_HANDLE_VALUE) {
        throwSystemError(
            "Directory could not be read"_el,
            "The operating system could not open the directory for reading."_el,
            path,
            GetLastError());
    }
    auto closeFindHandle = std::unique_ptr<void, decltype(&FindClose)>{findHandle, &FindClose};
    auto result = std::vector<Path>{};
    const auto refreshTime = time::TimePoint::now();
    const auto logicalPathIsResolved = path == resolvedPath;
    while (true) {
        const auto name = std::wstring_view{findData.cFileName};
        if (name != L"." && name != L"..") {
            const auto pathName = text::StringConverter{name}.toString();
            auto child = directoryEntryPath(path, pathName);
            if (child.isEmpty()) {
                throw PathError{PathErrorContext{
                    "Directory entry is invalid"_el,
                    "The operating system returned a directory-entry name that cannot be represented as a path."_el}
                        .setSourcePath(path.toString())};
            }
            auto info = PathInfoData{};
            info.resolvedPath =
                logicalPathIsResolved ? pathWithoutInfo(child) : directoryEntryPath(resolvedPath, pathName);
            info.lastRefresh = refreshTime;
            info.exists = true;
            info.type = typeFromAttributes(findData.dwFileAttributes, findData.dwReserved0, FILE_TYPE_DISK);
            info.loadedParts.set(PathInfoPart::Type);
            const auto size = (static_cast<std::uint64_t>(findData.nFileSizeHigh) << 32U) |
                static_cast<std::uint64_t>(findData.nFileSizeLow);
            if (info.type == PathType::RegularFile) {
                info.fileSize = unit::ByteLength::fromSizeT(static_cast<std::size_t>(size));
            }
            info.loadedParts.set(PathInfoPart::Size);
            info.lastModified = time::impl::windows_time_converter::fromFileTime(findData.ftLastWriteTime);
            info.lastAccessed = time::impl::windows_time_converter::fromFileTime(findData.ftLastAccessTime);
            info.birthTime = time::impl::windows_time_converter::fromFileTime(findData.ftCreationTime);
            info.loadedParts.set(PathInfoPart::Times);
            info.accessInfo = accessInfoFromAttributes(findData.dwFileAttributes);
            info.loadedParts.set(PathInfoPart::AccessRights);
            info.attributes = pathAttributesFromWindowsAttributes(findData.dwFileAttributes);
            info.loadedParts.set(PathInfoPart::Attributes);
            preloadInfo(child, std::move(info));
            result.emplace_back(std::move(child));
        }
        if (FindNextFileW(findHandle, &findData) != 0) {
            continue;
        }
        const auto errorCode = GetLastError();
        if (errorCode != ERROR_NO_MORE_FILES) {
            throwSystemError(
                "Directory could not be read"_el,
                "The operating system could not read all directory entries."_el,
                path,
                errorCode);
        }
        break;
    }
    const auto handle = closeFindHandle.release();
    if (FindClose(handle) == 0) {
        throwSystemError(
            "Directory could not be read"_el,
            "The operating system could not finalize the directory search."_el,
            path,
            GetLastError());
    }
    return result;
}

void WindowsPathBackend::createDirectoryEntryOrThrow(const Path &path, const PathAccessProfile profile) const {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::PlatformU16StringAccess{pathText};
    auto security = std::unique_ptr<WindowsAccessProfileSecurity>{};
    auto *securityAttributes = static_cast<SECURITY_ATTRIBUTES *>(nullptr);
    if (profile != PathAccessProfile::Default) {
        security = std::make_unique<WindowsAccessProfileSecurity>(path, profile);
        securityAttributes = security->securityAttributes();
    }
    if (CreateDirectoryW(pathAccess.nullTerminatedWideCharPtr(), securityAttributes) == 0) {
        throwSystemError(
            "Directory could not be created"_el,
            "The operating system could not create the directory."_el,
            path,
            GetLastError());
    }
    invalidateInfo(path);
}

void WindowsPathBackend::removeEntryOrThrow(const Path &path) const {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::PlatformU16StringAccess{pathText};
    const auto attributes = GetFileAttributesW(pathAccess.nullTerminatedWideCharPtr());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        throwSystemError(
            "Path could not be removed"_el,
            "The operating system could not inspect the path before removing it."_el,
            path,
            GetLastError());
    }
    const auto result = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0U
        ? RemoveDirectoryW(pathAccess.nullTerminatedWideCharPtr())
        : DeleteFileW(pathAccess.nullTerminatedWideCharPtr());
    if (result == 0) {
        throwSystemError(
            "Path could not be removed"_el, "The operating system could not remove the path."_el, path, GetLastError());
    }
    invalidateInfo(path);
}

void WindowsPathBackend::copyFileEntryOrThrow(const Path &source, const Path &destination) const {
    const auto sourceText = pathTextOrThrow(source);
    const auto destinationText = pathTextOrThrow(destination);
    const auto sourceAccess = text::impl::PlatformU16StringAccess{sourceText};
    const auto destinationAccess = text::impl::PlatformU16StringAccess{destinationText};
    if (CopyFileW(sourceAccess.nullTerminatedWideCharPtr(), destinationAccess.nullTerminatedWideCharPtr(), TRUE) == 0) {
        throwSystemError(
            "File could not be copied"_el,
            "The operating system could not copy the file."_el,
            source,
            destination,
            GetLastError());
    }
    invalidateInfo(destination);
}

void WindowsPathBackend::moveEntryOrThrow(const Path &source, const Path &destination) const {
    const auto sourceText = pathTextOrThrow(source);
    const auto destinationText = pathTextOrThrow(destination);
    const auto sourceAccess = text::impl::PlatformU16StringAccess{sourceText};
    const auto destinationAccess = text::impl::PlatformU16StringAccess{destinationText};
    if (MoveFileExW(
            sourceAccess.nullTerminatedWideCharPtr(),
            destinationAccess.nullTerminatedWideCharPtr(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
        throwSystemError(
            "Path could not be moved"_el,
            "The operating system could not move the path on the same filesystem."_el,
            source,
            destination,
            GetLastError());
    }
    invalidateInfo(source);
    invalidateInfo(destination);
}

auto WindowsPathBackend::readSymlinkOrThrow(const Path &path) const -> Path {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::PlatformU16StringAccess{pathText};
    const auto handle = CreateFileW(
        pathAccess.nullTerminatedWideCharPtr(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throwSystemError(
            "Symbolic link could not be read"_el,
            "The symbolic link could not be opened for reading."_el,
            path,
            GetLastError());
    }
    const auto closeHandle = std::unique_ptr<void, decltype(&CloseHandle)>{handle, &CloseHandle};
    alignas(WindowsSymbolicLinkReparseData) auto buffer = std::array<std::byte, MAXIMUM_REPARSE_DATA_BUFFER_SIZE>{};
    auto bytesReturned = DWORD{};
    if (DeviceIoControl(
            handle,
            FSCTL_GET_REPARSE_POINT,
            nullptr,
            0,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &bytesReturned,
            nullptr) == 0) {
        throwSystemError(
            "Symbolic link could not be read"_el,
            "The operating system could not read the symbolic-link data."_el,
            path,
            GetLastError());
    }
    const auto pathBufferOffset = offsetof(WindowsSymbolicLinkReparseData, pathBuffer);
    if (bytesReturned < pathBufferOffset) {
        throw PathError{PathErrorContext{
            "Symbolic link could not be read"_el, "The operating system returned malformed symbolic-link data."_el}
                .setSourcePath(path.toString())};
    }
    const auto *reparseData = reinterpret_cast<const WindowsSymbolicLinkReparseData *>(buffer.data());
    if (reparseData->reparseDataLength > bytesReturned - 8U) {
        throw PathError{PathErrorContext{
            "Symbolic link could not be read"_el, "The operating system returned malformed symbolic-link data."_el}
                .setSourcePath(path.toString())};
    }
    if (reparseData->reparseTag != IO_REPARSE_TAG_SYMLINK) {
        throw PathError{
            PathErrorContext{"Symbolic link could not be read"_el, "The path is not a Windows symbolic link."_el}
                .setSourcePath(path.toString())};
    }
    auto nameOffset = reparseData->printNameOffset;
    auto nameLength = reparseData->printNameLength;
    auto usesSubstituteName = false;
    if (nameLength == 0U) {
        nameOffset = reparseData->substituteNameOffset;
        nameLength = reparseData->substituteNameLength;
        usesSubstituteName = true;
    }
    const auto pathBufferBytes = bytesReturned - pathBufferOffset;
    if (nameOffset > pathBufferBytes || nameLength > pathBufferBytes - nameOffset ||
        nameOffset % sizeof(wchar_t) != 0U || nameLength % sizeof(wchar_t) != 0U) {
        throw PathError{PathErrorContext{
            "Symbolic link could not be read"_el, "The operating system returned malformed symbolic-link data."_el}
                .setSourcePath(path.toString())};
    }
    const auto *nameStart = reparseData->pathBuffer + nameOffset / sizeof(wchar_t);
    auto targetText = std::wstring{nameStart, nameLength / sizeof(wchar_t)};
    if ((reparseData->flags & cSymlinkReparseFlagRelative) == 0U && usesSubstituteName) {
        if (targetText.starts_with(L"\\??\\UNC\\")) {
            targetText = L"\\\\" + targetText.substr(8U);
        } else if (targetText.starts_with(L"\\??\\")) {
            targetText.erase(0U, 4U);
        }
    }
    return Path::fromWindowsOrThrow(text::StringConverter{targetText}.toString());
}

void WindowsPathBackend::createSymlinkOrThrow(
    const Path &target, const Path &path, const bool targetIsDirectory) const {
    const auto targetText = pathTextOrThrow(target);
    const auto pathText = pathTextOrThrow(path);
    const auto targetAccess = text::impl::PlatformU16StringAccess{targetText};
    const auto pathAccess = text::impl::PlatformU16StringAccess{pathText};
    auto flags = DWORD{SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE};
    if (targetIsDirectory) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }
    if (CreateSymbolicLinkW(pathAccess.nullTerminatedWideCharPtr(), targetAccess.nullTerminatedWideCharPtr(), flags) !=
        0) {
        invalidateInfo(path);
        return;
    }
    auto errorCode = GetLastError();
    if (errorCode == ERROR_INVALID_PARAMETER) {
        flags &= ~SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
        if (CreateSymbolicLinkW(
                pathAccess.nullTerminatedWideCharPtr(), targetAccess.nullTerminatedWideCharPtr(), flags) != 0) {
            invalidateInfo(path);
            return;
        }
        errorCode = GetLastError();
    }
    throwSystemError(
        "Symbolic link could not be created"_el,
        "The operating system could not create the symbolic link."_el,
        target,
        path,
        errorCode);
}

}
