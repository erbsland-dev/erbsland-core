// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsFileLock.hpp"

#include "../Path.hpp"
#include "../PathError.hpp"
#include "../PathErrorContext.hpp"

#include "../../system/impl/WindowsErrorContext.hpp"
#include "../../text/impl/PlatformU16StringAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringConverter.hpp"

#include <memory>

namespace erbsland::path::impl {

using namespace text::literals;

WindowsFileLock::WindowsFileLock(const Path &path, const Path &lockPath) {
    const auto lockPathText = text::StringConverter{lockPath.toWindows()}.toU16String();
    const auto lockPathAccess = text::impl::PlatformU16StringAccess{lockPathText};
    const auto handle = CreateFileW(
        lockPathAccess.nullTerminatedWideCharPtr(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throwLockError(path, lockPath, GetLastError());
    }
    auto overlapped = OVERLAPPED{};
    if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &overlapped) ==
        0) {
        const auto errorCode = GetLastError();
        CloseHandle(handle);
        throwLockError(path, lockPath, errorCode);
    }
    _handle = handle;
}

WindowsFileLock::~WindowsFileLock() {
    if (_handle != nullptr) {
        auto overlapped = OVERLAPPED{};
        UnlockFileEx(_handle, 0, MAXDWORD, MAXDWORD, &overlapped);
        CloseHandle(_handle);
    }
}

void WindowsFileLock::throwLockError(const Path &path, const Path &lockPath, const unsigned long errorCode) {
    throw PathError{PathErrorContext{
        "File lock could not be acquired"_el,
        "The operating system could not acquire an exclusive lock for this path."_el}
            .setSourcePath(path.toString())
            .setTargetPath(lockPath.toString())
            .setPlatformContext(system::impl::WindowsErrorContext::fromErrorCode(errorCode))};
}

auto FileLock::create(const Path &path, const Path &lockPath) -> std::unique_ptr<FileLock> {
    return std::make_unique<WindowsFileLock>(path, lockPath);
}

}
