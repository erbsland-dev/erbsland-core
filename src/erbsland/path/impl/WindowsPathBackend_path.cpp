// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsPathBackend.hpp"

#include "PathInfoData.hpp"

#include "../Path.hpp"
#include "../PathCreateMode.hpp"
#include "../PathError.hpp"
#include "../PathInfoParts.hpp"
#include "../PathResolveMode.hpp"
#include "../PathWindowsFormat.hpp"

#include "../../core/impl/WindowsApi.hpp"
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

#include <algorithm>
#include <cstdint>
#include <cwchar>
#include <memory>
#include <string_view>
#include <vector>

namespace erbsland::path::impl {

using namespace text::literals;

auto WindowsPathBackend::pathTextOrThrow(const Path &path) -> text::U16String {
    const auto pathText = path.toWindows(PathWindowsFormat::Extended);
    if (pathText.isEmpty()) {
        throw PathError{PathErrorContext{
            "Path cannot be used on this platform"_el, "The path cannot be represented in the Windows path format."_el}
                .setSourcePath(path.toString())
                .setHelp("Use a path compatible with Windows."_el)};
    }
    return text::StringConverter{pathText}.toU16String();
}

void WindowsPathBackend::createParentDirectoriesOrThrow(const Path &path) {
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
        const auto directoryTextAccess = text::impl::PlatformU16StringAccess{directoryText};
        if (CreateDirectoryW(directoryTextAccess.nullTerminatedWideCharPtr(), nullptr) != 0) {
            continue;
        }
        auto errorCode = GetLastError();
        if (errorCode == ERROR_ALREADY_EXISTS) {
            const auto attributes = GetFileAttributesW(directoryTextAccess.nullTerminatedWideCharPtr());
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
                continue;
            }
            if (attributes == INVALID_FILE_ATTRIBUTES) {
                errorCode = GetLastError();
            }
        }
        throwSystemError(
            "Directory could not be created"_el,
            "A parent directory required for the target file could not be created."_el,
            directory,
            errorCode);
    }
}

auto WindowsPathBackend::handleHasContentOrThrow(void *handle, const Path &path) -> bool {
    auto fileSize = LARGE_INTEGER{};
    if (GetFileSizeEx(static_cast<HANDLE>(handle), &fileSize) == 0) {
        throwSystemError(
            "File information is unavailable"_el,
            "The operating system could not inspect the opened file."_el,
            path,
            GetLastError());
    }
    return fileSize.QuadPart > 0;
}

auto WindowsPathBackend::typeFromAttributes(
    const unsigned long attributes, const unsigned long reparseTag, const unsigned long fileType) noexcept -> PathType {
    if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
        return reparseTag == IO_REPARSE_TAG_SYMLINK ? PathType::Symlink : PathType::ReparsePoint;
    }
    if (fileType == FILE_TYPE_CHAR) {
        return PathType::Device;
    }
    if (fileType == FILE_TYPE_PIPE) {
        return PathType::Pipe;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
        return PathType::Directory;
    }
    if (fileType == FILE_TYPE_DISK) {
        return PathType::RegularFile;
    }
    return PathType::Unknown;
}

auto WindowsPathBackend::accessInfoFromAttributes(const unsigned long attributes) noexcept -> PathAccessInfo {
    auto rights = PathAccessRights{PathAccessRight::Read, PathAccessRight::Execute};
    if ((attributes & FILE_ATTRIBUTE_READONLY) == 0U) {
        rights.set(PathAccessRight::Write);
    }
    return PathAccessInfo{}.setCurrentProcessRights(rights);
}

auto WindowsPathBackend::pathAttributesFromWindowsAttributes(const unsigned long attributes) noexcept
    -> PathAttributes {
    auto result = PathAttributes{};
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0U) {
        result.set(PathAttribute::ReadOnly);
    }
    if ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0U) {
        result.set(PathAttribute::Hidden);
    }
    if ((attributes & FILE_ATTRIBUTE_ARCHIVE) != 0U) {
        result.set(PathAttribute::Archive);
    }
    if ((attributes & FILE_ATTRIBUTE_SYSTEM) != 0U) {
        result.set(PathAttribute::System);
    }
    return result;
}

auto WindowsPathBackend::windowsAttributesFromPathAttributes(const PathAttributes attributes) -> unsigned long {
    auto unsupported = attributes;
    unsupported.clear(PathAttribute::ReadOnly);
    unsupported.clear(PathAttribute::Hidden);
    unsupported.clear(PathAttribute::Archive);
    if (unsupported.hasAny()) {
        throw PathError{PathErrorContext{
            "File attributes could not be changed"_el,
            "One or more requested attributes are not supported on Windows."_el}
                .setHelp("Request only attributes supported by Windows."_el)};
    }
    auto result = DWORD{0};
    if (attributes.isSet(PathAttribute::ReadOnly)) {
        result |= FILE_ATTRIBUTE_READONLY;
    }
    if (attributes.isSet(PathAttribute::Hidden)) {
        result |= FILE_ATTRIBUTE_HIDDEN;
    }
    if (attributes.isSet(PathAttribute::Archive)) {
        result |= FILE_ATTRIBUTE_ARCHIVE;
    }
    return result;
}

auto WindowsPathBackend::sidString(void *sid) -> text::String {
    auto *sidText = static_cast<wchar_t *>(nullptr);
    if (sid == nullptr || ConvertSidToStringSidW(sid, &sidText) == 0) {
        throw PathError{PathErrorContext{
            "File owner identifier is unavailable"_el,
            "The Windows security identifier could not be converted to text."_el}};
    }
    const auto freeSidText = std::unique_ptr<void, decltype(&LocalFree)>{sidText, &LocalFree};
    return text::StringConverter{std::wstring_view{sidText, std::wcslen(sidText)}}.toString();
}

auto WindowsPathBackend::physicalPathOrThrow(const Path &path) -> Path {
    const auto pathText = pathTextOrThrow(path);
    const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
    const auto handle = CreateFileW(
        pathTextAccess.nullTerminatedWideCharPtr(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throwSystemError(
            "Path could not be resolved"_el,
            "The path could not be opened to resolve its physical location."_el,
            path,
            GetLastError());
    }

    const auto closeHandle = std::unique_ptr<void, decltype(&CloseHandle)>{handle, &CloseHandle};
    const auto length = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (length == 0U) {
        throwSystemError(
            "Path could not be resolved"_el,
            "The path could not be resolved to its physical location."_el,
            path,
            GetLastError());
    }
    auto buffer = text::impl::UnsafeU16StringBuffer{unit::U16DataLength::fromSizeT(length)};
    const auto actualLength =
        GetFinalPathNameByHandleW(handle, buffer.dataAsWide(), length + 1U, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (actualLength == 0U || actualLength > length) {
        throwSystemError(
            "Path could not be resolved"_el,
            "The path could not be resolved to its physical location."_el,
            path,
            GetLastError());
    }
    return Path::fromWindowsOrThrow(buffer.takeAsUtf8(unit::U16DataLength::fromSizeT(actualLength)));
}

auto WindowsPathBackend::existingPath(const Path &path) -> bool {
    const auto pathText = pathTextOrThrow(path);
    const auto pathTextAccess = text::impl::PlatformU16StringAccess{pathText};
    const auto attributes = GetFileAttributesW(pathTextAccess.nullTerminatedWideCharPtr());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        return true;
    }
    const auto errorCode = GetLastError();
    if (errorCode == ERROR_FILE_NOT_FOUND || errorCode == ERROR_PATH_NOT_FOUND) {
        return false;
    }
    throwSystemError(
        "Path information is unavailable"_el,
        "The operating system could not provide information about the path."_el,
        path,
        errorCode);
}

auto WindowsPathBackend::weakPathOrThrow(const Path &path) -> Path {
    const auto elements = path.elements();
    for (auto count = elements.count(); !count.isZero(); --count) {
        auto prefixElements = elements.prefix(count);
        prefixElements.removeFirst();
        const auto prefix = assemblePath(elements.first(), prefixElements);
        if (!existingPath(prefix)) {
            continue;
        }
        const auto physicalPrefix = physicalPathOrThrow(prefix);
        auto resultElements = nonRootElements(physicalPrefix);
        resultElements.append(elements.suffix(elements.count() - count));
        return lexicalPath(assemblePath(physicalPrefix.root(), resultElements));
    }
    return path;
}

auto WindowsPathBackend::physicalNoFinalSymlinkPathOrThrow(const Path &path) -> Path {
    if (path.isRoot()) {
        return physicalPathOrThrow(path);
    }
    if (!existingPath(path)) {
        throw PathError{
            PathErrorContext{"Path does not exist"_el, "No file or directory exists at the specified path."_el}
                .setSourcePath(path.toString())
                .setHelp("Check that the path is correct and that every parent directory exists."_el)};
    }
    const auto parent = path.parent();
    if (parent.isEmpty()) {
        return path;
    }
    return joinedLexicalPath(physicalPathOrThrow(parent), Path{path.name()});
}

auto WindowsPathBackend::nonRootElements(const Path &path) -> text::StringList {
    auto result = text::StringList{};
    for (const auto &element : path.elements()) {
        if (path.isAbsolute() && element == path.root()) {
            continue;
        }
        result.append(element);
    }
    return result;
}

void WindowsPathBackend::throwSystemError(
    const text::String &title, const text::String &description, const Path &path, const unsigned long errorCode) {
    throw PathError{PathErrorContext{title, description}
            .setSourcePath(path.toString())
            .setPlatformContext(system::impl::WindowsErrorContext::fromErrorCode(errorCode))};
}

void WindowsPathBackend::throwSystemError(
    const text::String &title,
    const text::String &description,
    const Path &source,
    const Path &destination,
    const unsigned long errorCode) {
    throw PathError{PathErrorContext{title, description}
            .setSourcePath(source.toString())
            .setTargetPath(destination.toString())
            .setPlatformContext(system::impl::WindowsErrorContext::fromErrorCode(errorCode))};
}

[[nodiscard]] auto createPathBackend() noexcept -> PathBackendPtr {
    return std::make_unique<WindowsPathBackend>();
}

}
