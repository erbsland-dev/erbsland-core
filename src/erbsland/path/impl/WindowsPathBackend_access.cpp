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
#include "../../system/impl/WindowsErrorContext.hpp"
#include "../../system/UserId.hpp"
#include "../../text/impl/PlatformU16StringAccess.hpp"
#include "../../text/impl/UnsafeU16StringBuffer.hpp"
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

auto WindowsPathBackend::openByteInputStreamOrThrow(const Path &path, const PathReadDataOptions options) const
    -> stream::ByteInputStreamPtr {
    const auto handle = openInputHandleOrThrow(path, options.symlinkMode());
    auto native = std::make_shared<stream::impl::WindowsNativeStream>(
        handle, stream::impl::NativeStreamOwnership::Owned, path.toString());
    return stream::impl::createBufferedByteInputStream(std::move(native), options.streamSettings());
}

auto WindowsPathBackend::openInputHandleOrThrow(const Path &path, const SymlinkMode symlinkMode) -> void * {
    if (symlinkMode == SymlinkMode::Follow) {
        const auto pathText = pathTextOrThrow(path);
        const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
        const auto handle = CreateFileW(
            pathTextAccess.nullTerminatedWideCharPtr(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            return handle;
        }
        throwSystemError(
            "File could not be opened for reading"_el,
            "The operating system could not open the file for reading."_el,
            path,
            GetLastError());
    }

    const auto absolutePath = path.toAbsoluteOrThrow();
    const auto elements = absolutePath.elements();
    auto currentPath = Path{elements.first()};
    auto handle = INVALID_HANDLE_VALUE;
    for (auto index = unit::ItemIndex::one(); index.isWithin(elements.count()); ++index) {
        currentPath /= elements.get(index);
        const auto pathText = pathTextOrThrow(currentPath);
        const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
        const auto isFinal = !index.advanced(unit::ItemCount::one()).isWithin(elements.count());
        handle = CreateFileW(
            pathTextAccess.nullTerminatedWideCharPtr(),
            isFinal ? GENERIC_READ : FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
            nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            throwSystemError(
                "File could not be opened for reading"_el,
                "The path could not be opened without following symbolic links."_el,
                path,
                GetLastError());
        }
        auto attributeInfo = FILE_ATTRIBUTE_TAG_INFO{};
        if (GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &attributeInfo, sizeof(attributeInfo)) == 0) {
            const auto errorCode = GetLastError();
            CloseHandle(handle);
            throwSystemError(
                "File could not be opened for reading"_el,
                "The opened path component could not be inspected."_el,
                path,
                errorCode);
        }
        if ((attributeInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
            CloseHandle(handle);
            throwSystemError(
                "File could not be opened for reading"_el,
                "The path contains a symbolic link or reparse point."_el,
                path,
                ERROR_CANT_ACCESS_FILE);
        }
        if (!isFinal) {
            if ((attributeInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0U) {
                CloseHandle(handle);
                throwSystemError(
                    "File could not be opened for reading"_el,
                    "An intermediate path component is not a directory."_el,
                    path,
                    ERROR_DIRECTORY);
            }
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }
    if (handle != INVALID_HANDLE_VALUE) {
        return handle;
    }
    throwSystemError(
        "File could not be opened for reading"_el,
        "A filesystem root has no readable file content."_el,
        path,
        ERROR_ACCESS_DENIED);
}

void WindowsPathBackend::setAccessProfileOrThrow(
    const Path &path, const PathAccessProfile profile, [[maybe_unused]] const PathChangeOptions options) const {
    if (profile == PathAccessProfile::Default) {
        return;
    }
    const auto resolvedPath = resolveOrThrow(path, PathResolveMode::PhysicalNoFinalSymlink);
    const auto pathText = pathTextOrThrow(resolvedPath);
    const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
    auto security = WindowsAccessProfileSecurity{resolvedPath, profile};
    const auto securityStatus = SetNamedSecurityInfoW(
        const_cast<LPWSTR>(pathTextAccess.nullTerminatedWideCharPtr()),
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
    const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
    const auto currentAttributes = GetFileAttributesW(pathTextAccess.nullTerminatedWideCharPtr());
    if (currentAttributes == INVALID_FILE_ATTRIBUTES) {
        throwSystemError(
            "File attributes are unavailable"_el,
            "The operating system could not read the current file attributes."_el,
            resolvedPath,
            GetLastError());
    }
    const auto newAttributes = currentAttributes | windowsAttributesFromPathAttributes(attributes);
    if (SetFileAttributesW(pathTextAccess.nullTerminatedWideCharPtr(), newAttributes) == 0) {
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
    const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
    const auto currentAttributes = GetFileAttributesW(pathTextAccess.nullTerminatedWideCharPtr());
    if (currentAttributes == INVALID_FILE_ATTRIBUTES) {
        throwSystemError(
            "File attributes are unavailable"_el,
            "The operating system could not read the current file attributes."_el,
            resolvedPath,
            GetLastError());
    }
    const auto newAttributes = currentAttributes & ~windowsAttributesFromPathAttributes(attributes);
    if (SetFileAttributesW(pathTextAccess.nullTerminatedWideCharPtr(), newAttributes) == 0) {
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
    const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
    auto security = std::unique_ptr<WindowsAccessProfileSecurity>{};
    auto *securityAttributes = static_cast<SECURITY_ATTRIBUTES *>(nullptr);
    if (options.accessProfile() != PathAccessProfile::Default) {
        security = std::make_unique<WindowsAccessProfileSecurity>(path, options.accessProfile());
        securityAttributes = security->securityAttributes();
    }
    const auto handle = CreateFileW(
        pathTextAccess.nullTerminatedWideCharPtr(),
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
        // Preserve the stream setup error; this handle has no remaining owner that could recover it.
        CloseHandle(handle);
        throw;
    }
}

}
