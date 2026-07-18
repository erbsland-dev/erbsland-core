// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixPathBackend.hpp"

#include "BackendFactory.hpp"
#include "PathInfoData.hpp"

#include "../Path.hpp"
#include "../PathCreateMode.hpp"
#include "../PathError.hpp"
#include "../PathInfoParts.hpp"
#include "../PathResolveMode.hpp"

#include "../../core/Definitions.hpp"
#include "../../stream/impl/NativeOutputStream.hpp"
#include "../../stream/impl/PosixNativeStream.hpp"
#include "../../system/GroupId.hpp"
#include "../../system/PosixErrorContext.hpp"
#include "../../system/UserId.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../text/impl/UnsafeU8StringEditorAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"
#include "../../time/impl/PosixTimeConverter.hpp"
#include "../../unit/ByteLength.hpp"

#include <fcntl.h>

#ifdef ERBSLAND_OS_LINUX
#include <linux/fs.h>
#include <sys/ioctl.h>
#endif
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

auto PosixPathBackend::physicalPathOrThrow(const Path &path) -> Path {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    errno = 0;
    auto resolved = std::unique_ptr<char, decltype(&std::free)>{::realpath(pathAccess.data(), nullptr), &std::free};
    if (resolved == nullptr) {
        throwSystemError(
            "Path could not be resolved"_el,
            "The path could not be resolved to its physical location."_el,
            path,
            errno);
    }
    return Path::fromPosixOrThrow(text::String{std::string_view{resolved.get()}});
}

auto PosixPathBackend::existingPath(const Path &path) -> bool {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.data(), &info) == 0) {
        return true;
    }
    if (errno == ENOENT || errno == ENOTDIR) {
        return false;
    }
    throwSystemError(
        "Path information is unavailable"_el,
        "The operating system could not provide information about the path."_el,
        path,
        errno);
}

auto PosixPathBackend::weakPathOrThrow(const Path &path) -> Path {
    for (auto count = path.elementCount(); !count.isZero(); --count) {
        auto const [front, back] = path.splitAfter(count);
        if (!existingPath(front)) {
            continue;
        }
        const auto physicalFront = physicalPathOrThrow(front);
        return lexicalPath(physicalFront / back);
    }
    return path;
}

auto PosixPathBackend::physicalNoFinalSymlinkPathOrThrow(const Path &path) -> Path {
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

void PosixPathBackend::throwSystemError(
    const text::String &title, const text::String &description, const Path &path, const int errorCode) {
    throw PathError{PathErrorContext{title, description}
            .setSourcePath(path.toString())
            .setPlatformContext(system::PosixErrorContext::fromErrorCode(errorCode))};
}

void PosixPathBackend::throwSystemError(
    const text::String &title,
    const text::String &description,
    const Path &source,
    const Path &destination,
    const int errorCode) {
    throw PathError{PathErrorContext{title, description}
            .setSourcePath(source.toString())
            .setTargetPath(destination.toString())
            .setPlatformContext(system::PosixErrorContext::fromErrorCode(errorCode))};
}

[[nodiscard]] auto createPathBackend() noexcept -> PathBackendPtr {
    return std::make_unique<PosixPathBackend>();
}

}
