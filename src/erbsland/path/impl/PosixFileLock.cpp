// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixFileLock.hpp"

#include "../Path.hpp"
#include "../PathError.hpp"
#include "../PathErrorContext.hpp"

#include "../../system/impl/PosixErrorContext.hpp"
#include "../../text/impl/PlatformU8StringAccess.hpp"
#include "../../text/Literals.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <cerrno>
#include <memory>

namespace erbsland::path::impl {

using namespace text::literals;

PosixFileLock::PosixFileLock(const Path &path, const Path &lockPath) {
    const auto lockPathText = lockPath.toPosix();
    const auto lockPathAccess = text::impl::PlatformU8StringAccess{lockPathText};
    const auto descriptor = ::open(lockPathAccess.nullTerminatedCharPtr(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (descriptor < 0) {
        throwLockError(path, lockPath, errno);
    }
    if (::flock(descriptor, LOCK_EX | LOCK_NB) != 0) {
        const auto errorCode = errno;
        ::close(descriptor);
        throwLockError(path, lockPath, errorCode);
    }
    _descriptor = descriptor;
}

PosixFileLock::~PosixFileLock() {
    if (_descriptor >= 0) {
        ::flock(_descriptor, LOCK_UN);
        ::close(_descriptor);
    }
}

void PosixFileLock::throwLockError(const Path &path, const Path &lockPath, const int errorCode) {
    throw PathError{PathErrorContext{
        "File lock could not be acquired"_el,
        "The operating system could not acquire an exclusive lock for this path."_el}
            .setSourcePath(path.toString())
            .setTargetPath(lockPath.toString())
            .setPlatformContext(system::impl::PosixErrorContext::fromErrorCode(errorCode))};
}

auto FileLock::create(const Path &path, const Path &lockPath) -> std::unique_ptr<FileLock> {
    return std::make_unique<PosixFileLock>(path, lockPath);
}

}
