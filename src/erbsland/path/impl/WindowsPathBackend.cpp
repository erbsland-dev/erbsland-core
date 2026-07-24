// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsPathBackend.hpp"

#include "PathInfoData.hpp"
#include "WindowsAccessProfileSecurity.hpp"

#include "../Path.hpp"
#include "../PathCreateMode.hpp"
#include "../PathError.hpp"
#include "../PathInfoParts.hpp"
#include "../PathResolveMode.hpp"
#include "../PathWindowsFormat.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../stream/impl/BufferedByteOutputStream.hpp"
#include "../../stream/impl/InputStreamFactory.hpp"
#include "../../stream/impl/NativeOutputStream.hpp"
#include "../../stream/impl/WindowsNativeStream.hpp"
#include "../../system/GroupId.hpp"
#include "../../system/UserId.hpp"
#include "../../system/WindowsErrorContext.hpp"
#include "../../text/impl/UnsafeU16StringAccess.hpp"
#include "../../text/impl/UnsafeU16StringBuffer.hpp"
#include "../../text/impl/UnsafeU16StringEditorAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/u16/U16StringEditor.hpp"
#include "../../time/impl/WindowsTimeConverter.hpp"
#include "../../unit/U16DataLength.hpp"

#include <aclapi.h>
#include <sddl.h>
#include <userenv.h>

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <string_view>
#include <vector>

namespace erbsland::path::impl {

using namespace text::literals;

auto WindowsPathBackend::currentDirectoryOrThrow() const -> Path {
    const auto length = GetCurrentDirectoryW(0, nullptr);
    if (length == 0U) {
        throwSystemError(
            "Current directory is unavailable"_el,
            "The operating system could not determine the application's current directory."_el,
            {},
            GetLastError());
    }
    auto buffer = text::impl::UnsafeU16StringBuffer{static_cast<std::size_t>(length)};
    const auto actualLength = GetCurrentDirectoryW(length, buffer.dataAsWide());
    if (actualLength == 0U || actualLength >= length) {
        throwSystemError(
            "Current directory is unavailable"_el,
            "The operating system could not determine the application's current directory."_el,
            {},
            GetLastError());
    }
    return Path::fromWindowsOrThrow(buffer.takeAsUtf8(unit::U16DataLength::fromSizeT(actualLength)));
}

auto WindowsPathBackend::userHomeDirectoryOrThrow() const -> Path {
    auto token = HANDLE{};
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == 0) {
        throwSystemError(
            "User home directory is unavailable"_el,
            "The current process token could not be opened."_el,
            {},
            GetLastError());
    }
    const auto closeToken = std::unique_ptr<void, decltype(&CloseHandle)>{token, &CloseHandle};

    auto length = DWORD{0};
    static_cast<void>(GetUserProfileDirectoryW(token, nullptr, &length));
    const auto lengthError = GetLastError();
    if (length == 0U || lengthError != ERROR_INSUFFICIENT_BUFFER) {
        throwSystemError(
            "User home directory is unavailable"_el,
            "The operating system could not determine the current user's profile-directory size."_el,
            {},
            lengthError);
    }
    auto buffer = text::impl::UnsafeU16StringBuffer{static_cast<std::size_t>(length)};
    if (GetUserProfileDirectoryW(token, buffer.dataAsWide(), &length) == 0 || length <= 1U) {
        throwSystemError(
            "User home directory is unavailable"_el,
            "The operating system could not determine the current user's profile directory."_el,
            {},
            GetLastError());
    }
    const auto home = Path::fromWindows(buffer.takeAsUtf8(unit::U16DataLength::fromSizeT(length - 1U)));
    if (home.isEmpty() || !home.isAbsolute()) {
        throw PathError{PathErrorContext{
            "User home directory is unavailable"_el,
            "The current user's profile directory contains an invalid path."_el}};
    }
    return home;
}

auto WindowsPathBackend::systemTempDirectoryOrThrow() const -> Path {
    auto length = GetTempPath2W(0, nullptr);
    const auto useLegacyFunction = length == 0U;
    if (useLegacyFunction) {
        length = GetTempPathW(0, nullptr);
    }
    if (length == 0U) {
        throwSystemError(
            "Temporary directory is unavailable"_el,
            "The operating system could not determine the temporary directory."_el,
            {},
            GetLastError());
    }
    auto buffer = text::impl::UnsafeU16StringBuffer{static_cast<std::size_t>(length)};
    const auto actualLength =
        useLegacyFunction ? GetTempPathW(length, buffer.dataAsWide()) : GetTempPath2W(length, buffer.dataAsWide());
    if (actualLength == 0U || actualLength >= length) {
        throwSystemError(
            "Temporary directory is unavailable"_el,
            "The operating system could not determine the temporary directory."_el,
            {},
            GetLastError());
    }
    return Path::fromWindowsOrThrow(buffer.takeAsUtf8(unit::U16DataLength::fromSizeT(actualLength)));
}

auto WindowsPathBackend::resolveOrThrow(const Path &path, const PathResolveOptions options) const -> Path {
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

auto WindowsPathBackend::loadInfoOrThrow(const Path &path, const PathInfoParts parts) const -> PathInfoData {
    return loadResolvedInfoOrThrow(path, resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink), parts);
}

auto WindowsPathBackend::loadResolvedInfoOrThrow(
    [[maybe_unused]] const Path &path, const Path &resolvedPath, const PathInfoParts parts) const -> PathInfoData {
    auto result = PathInfoData{};
    result.resolvedPath = resolvedPath;

    const auto pathText = pathTextOrThrow(result.resolvedPath);
    const auto pathTextAccess = text::impl::UnsafeU16StringAccess{pathText};
    const auto handle = CreateFileW(
        pathTextAccess.dataAsWide(),
        FILE_READ_ATTRIBUTES | READ_CONTROL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throwSystemError(
            "Path information is unavailable"_el,
            "The path could not be opened to inspect its information."_el,
            result.resolvedPath,
            GetLastError());
    }

    const auto closeHandle = std::unique_ptr<void, decltype(&CloseHandle)>{handle, &CloseHandle};
    auto fileInfo = BY_HANDLE_FILE_INFORMATION{};
    if (GetFileInformationByHandle(handle, &fileInfo) == 0) {
        throwSystemError(
            "Path information is unavailable"_el,
            "The operating system could not provide information about the path."_el,
            result.resolvedPath,
            GetLastError());
    }

    auto reparseTag = DWORD{0};
    if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
        auto attributeTagInfo = FILE_ATTRIBUTE_TAG_INFO{};
        if (GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &attributeTagInfo, sizeof(attributeTagInfo)) ==
            0) {
            throwSystemError(
                "Path information is unavailable"_el,
                "The operating system could not inspect the Windows reparse point."_el,
                result.resolvedPath,
                GetLastError());
        }
        reparseTag = attributeTagInfo.ReparseTag;
    }

    SetLastError(ERROR_SUCCESS);
    const auto fileType = GetFileType(handle);
    if (fileType == FILE_TYPE_UNKNOWN) {
        const auto errorCode = GetLastError();
        if (errorCode != ERROR_SUCCESS) {
            throwSystemError(
                "Path information is unavailable"_el,
                "The operating system could not determine the native file type."_el,
                result.resolvedPath,
                errorCode);
        }
    }

    result.exists = true;
    result.type = typeFromAttributes(fileInfo.dwFileAttributes, reparseTag, fileType);
    result.loadedParts.set(PathInfoPart::Type);
    if (parts.isSet(PathInfoPart::Size)) {
        const auto size = (static_cast<std::uint64_t>(fileInfo.nFileSizeHigh) << 32U) |
            static_cast<std::uint64_t>(fileInfo.nFileSizeLow);
        if (result.type == PathType::RegularFile) {
            result.fileSize = unit::ByteLength::fromSizeT(static_cast<std::size_t>(size));
        }
        result.loadedParts.set(PathInfoPart::Size);
    }
    if (parts.isSet(PathInfoPart::Times)) {
        result.lastModified = time::impl::WindowsTimeConverter::fromFileTime(fileInfo.ftLastWriteTime);
        result.lastAccessed = time::impl::WindowsTimeConverter::fromFileTime(fileInfo.ftLastAccessTime);
        result.birthTime = time::impl::WindowsTimeConverter::fromFileTime(fileInfo.ftCreationTime);
        result.loadedParts.set(PathInfoPart::Times);
    }
    if (parts.isSet(PathInfoPart::AccessRights)) {
        result.accessInfo = accessInfoFromAttributes(fileInfo.dwFileAttributes);
        result.loadedParts.set(PathInfoPart::AccessRights);
    }
    if (parts.isSet(PathInfoPart::Attributes)) {
        result.attributes = pathAttributesFromWindowsAttributes(fileInfo.dwFileAttributes);
        result.loadedParts.set(PathInfoPart::Attributes);
    }
    if (parts.isSet(PathInfoPart::OwnerId) || parts.isSet(PathInfoPart::GroupId)) {
        auto *ownerSid = static_cast<PSID>(nullptr);
        auto *groupSid = static_cast<PSID>(nullptr);
        auto *securityDescriptor = static_cast<PSECURITY_DESCRIPTOR>(nullptr);
        const auto securityStatus = GetSecurityInfo(
            handle,
            SE_FILE_OBJECT,
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
            &ownerSid,
            &groupSid,
            nullptr,
            nullptr,
            &securityDescriptor);
        const auto freeSecurityDescriptor = std::unique_ptr<void, decltype(&LocalFree)>{securityDescriptor, &LocalFree};
        if (securityStatus != ERROR_SUCCESS) {
            throwSystemError(
                "File owner and group are unavailable"_el,
                "The operating system could not provide owner and group information for the path."_el,
                result.resolvedPath,
                securityStatus);
        }
        if (parts.isSet(PathInfoPart::OwnerId)) {
            result.ownerId = system::UserId{sidString(ownerSid)};
            result.loadedParts.set(PathInfoPart::OwnerId);
        }
        if (parts.isSet(PathInfoPart::GroupId)) {
            result.groupId = system::GroupId{sidString(groupSid)};
            result.loadedParts.set(PathInfoPart::GroupId);
        }
    }
    result.lastRefresh = time::TimePoint::now();
    return result;
}

auto WindowsPathBackend::openByteInputStreamOrThrow(const Path &path, const PathReadDataOptions options) const
    -> stream::ByteInputStreamPtr {
    const auto pathText = pathTextOrThrow(path);
    const auto pathTextAccess = text::impl::UnsafeU16StringAccess{pathText};
    const auto handle = CreateFileW(
        pathTextAccess.dataAsWide(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throwSystemError(
            "File could not be opened for reading"_el,
            "The operating system could not open the file for reading."_el,
            path,
            GetLastError());
    }
    auto native = std::make_shared<stream::impl::WindowsNativeStream>(
        handle, stream::impl::NativeStreamOwnership::Owned, path.toString());
    return stream::impl::createBufferedByteInputStream(std::move(native), options.streamSettings());
}

void WindowsPathBackend::setAccessProfileOrThrow(
    const Path &path, const PathAccessProfile profile, [[maybe_unused]] const PathChangeOptions options) const {
    if (profile == PathAccessProfile::Default) {
        return;
    }
    const auto resolvedPath = resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink);
    const auto pathText = pathTextOrThrow(resolvedPath);
    const auto pathTextAccess = text::impl::UnsafeU16StringAccess{pathText};
    auto security = WindowsAccessProfileSecurity{resolvedPath, profile};
    const auto securityStatus = SetNamedSecurityInfoW(
        const_cast<LPWSTR>(pathTextAccess.dataAsWide()),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        nullptr,
        nullptr,
        security.acl(),
        nullptr);
    if (securityStatus != ERROR_SUCCESS) {
        throwSystemError(
            "File permissions could not be changed"_el,
            "The operating system rejected the requested permission change."_el,
            resolvedPath,
            securityStatus);
    }
    invalidateInfo(path);
}

void WindowsPathBackend::addAttributesOrThrow(
    const Path &path, const PathAttributes attributes, [[maybe_unused]] const PathChangeOptions options) const {
    const auto resolvedPath = resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink);
    const auto pathText = pathTextOrThrow(resolvedPath);
    const auto pathTextAccess = text::impl::UnsafeU16StringAccess{pathText};
    const auto currentAttributes = GetFileAttributesW(pathTextAccess.dataAsWide());
    if (currentAttributes == INVALID_FILE_ATTRIBUTES) {
        throwSystemError(
            "File attributes are unavailable"_el,
            "The operating system could not read the current file attributes."_el,
            resolvedPath,
            GetLastError());
    }
    const auto newAttributes = currentAttributes | windowsAttributesFromPathAttributes(attributes);
    if (SetFileAttributesW(pathTextAccess.dataAsWide(), newAttributes) == 0) {
        throwSystemError(
            "File attributes could not be changed"_el,
            "The operating system rejected the requested attribute change."_el,
            resolvedPath,
            GetLastError());
    }
    invalidateInfo(path);
}

void WindowsPathBackend::clearAttributesOrThrow(
    const Path &path, const PathAttributes attributes, [[maybe_unused]] const PathChangeOptions options) const {
    const auto resolvedPath = resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink);
    const auto pathText = pathTextOrThrow(resolvedPath);
    const auto pathTextAccess = text::impl::UnsafeU16StringAccess{pathText};
    const auto currentAttributes = GetFileAttributesW(pathTextAccess.dataAsWide());
    if (currentAttributes == INVALID_FILE_ATTRIBUTES) {
        throwSystemError(
            "File attributes are unavailable"_el,
            "The operating system could not read the current file attributes."_el,
            resolvedPath,
            GetLastError());
    }
    const auto newAttributes = currentAttributes & ~windowsAttributesFromPathAttributes(attributes);
    if (SetFileAttributesW(pathTextAccess.dataAsWide(), newAttributes) == 0) {
        throwSystemError(
            "File attributes could not be changed"_el,
            "The operating system rejected the requested attribute change."_el,
            resolvedPath,
            GetLastError());
    }
    invalidateInfo(path);
}

auto WindowsPathBackend::openByteOutputStreamWithExistingContentOrThrow(
    const Path &path, const PathWriteDataOptions options) const -> PathByteOutputStreamOpenResult {
    if (options.createParents()) {
        createParentDirectoriesOrThrow(path);
    }

    auto desiredAccess = DWORD{GENERIC_WRITE | FILE_READ_ATTRIBUTES};
    auto creationDisposition = DWORD{CREATE_NEW};
    switch (options.creationMode()) {
    case PathCreateMode::CreateNew:
        creationDisposition = CREATE_NEW;
        break;
    case PathCreateMode::CreateOrOverwrite:
        creationDisposition = CREATE_ALWAYS;
        break;
    case PathCreateMode::CreateOrAppend:
        desiredAccess = FILE_APPEND_DATA | FILE_READ_ATTRIBUTES;
        creationDisposition = OPEN_ALWAYS;
        break;
    }

    const auto pathText = pathTextOrThrow(path);
    const auto pathTextAccess = text::impl::UnsafeU16StringAccess{pathText};
    auto security = std::unique_ptr<WindowsAccessProfileSecurity>{};
    auto *securityAttributes = static_cast<SECURITY_ATTRIBUTES *>(nullptr);
    if (options.accessProfile() != PathAccessProfile::Default) {
        security = std::make_unique<WindowsAccessProfileSecurity>(path, options.accessProfile());
        securityAttributes = security->securityAttributes();
    }
    const auto handle = CreateFileW(
        pathTextAccess.dataAsWide(),
        desiredAccess,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        securityAttributes,
        creationDisposition,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throwSystemError(
            "File could not be opened for writing"_el,
            "The operating system could not open or create the file for writing."_el,
            path,
            GetLastError());
    }
    try {
        const auto hasExistingContent = handleHasContentOrThrow(handle, path);
        return {
            std::make_shared<stream::impl::BufferedByteOutputStream>(
                std::make_shared<stream::impl::WindowsNativeStream>(
                    handle,
                    stream::impl::NativeStreamOwnership::Owned,
                    path.toString(),
                    options.creationMode() != PathCreateMode::CreateOrAppend),
                options.streamSettings()),
            hasExistingContent,
        };
    } catch (...) {
        static_cast<void>(CloseHandle(handle));
        throw;
    }
}

}
