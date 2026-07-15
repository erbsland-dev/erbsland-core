// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixPathBackend.hpp"

#include "../Path.hpp"
#include "../PathError.hpp"

#include "../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../text/impl/UnsafeU8StringViewAccess.hpp"
#include "../../text/Literals.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <memory>
#include <string_view>
#include <vector>

namespace erbsland::path::impl {

using namespace text::literals;

namespace {

struct DirectoryCloser final {
    void operator()(DIR *directory) const noexcept {
        if (directory != nullptr) {
            static_cast<void>(::closedir(directory));
        }
    }
};

}

auto PosixPathBackend::directoryEntriesOrThrow(const Path &path) const -> std::vector<Path> {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringViewAccess{pathText};
    auto *directory = ::opendir(pathAccess.data());
    if (directory == nullptr) {
        throwSystemError(
            "Directory could not be read"_el,
            "The operating system could not open the directory for reading."_el,
            path,
            errno);
    }
    auto closeDirectory = std::unique_ptr<DIR, DirectoryCloser>{directory};
    auto result = std::vector<Path>{};
    errno = 0;
    while (auto *entry = ::readdir(directory)) {
        const auto name = std::string_view{entry->d_name};
        if (name != "." && name != "..") {
            result.emplace_back(path / text::String{name});
        }
        errno = 0;
    }
    if (errno != 0) {
        throwSystemError(
            "Directory could not be read"_el,
            "The operating system could not read all directory entries."_el,
            path,
            errno);
    }
    const auto directoryHandle = closeDirectory.release();
    if (::closedir(directoryHandle) != 0) {
        throwSystemError(
            "Directory could not be read"_el,
            "The operating system could not finalize the directory search."_el,
            path,
            errno);
    }
    return result;
}

void PosixPathBackend::createDirectoryEntryOrThrow(const Path &path, const PathAccessProfile profile) const {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringViewAccess{pathText};
    if (::mkdir(pathAccess.data(), profileMode(profile, PathType::Directory)) != 0) {
        throwSystemError(
            "Directory could not be created"_el,
            "The operating system could not create the directory."_el,
            path,
            errno);
    }
    if (profile != PathAccessProfile::Default &&
        ::chmod(pathAccess.data(), profileMode(profile, PathType::Directory)) != 0) {
        const auto error = errno;
        static_cast<void>(::rmdir(pathAccess.data()));
        throwSystemError(
            "Directory permissions could not be applied"_el,
            "The directory was created, but its requested permissions could not be applied."_el,
            path,
            error);
    }
}

void PosixPathBackend::removeEntryOrThrow(const Path &path) const {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringViewAccess{pathText};
    struct stat info{};
    if (::lstat(pathAccess.data(), &info) != 0) {
        throwSystemError(
            "Path could not be removed"_el,
            "The operating system could not inspect the path before removing it."_el,
            path,
            errno);
    }
    const auto result = S_ISDIR(info.st_mode) != 0 ? ::rmdir(pathAccess.data()) : ::unlink(pathAccess.data());
    if (result != 0) {
        throwSystemError(
            "Path could not be removed"_el, "The operating system could not remove the path."_el, path, errno);
    }
}

void PosixPathBackend::copyFileEntryOrThrow(const Path &source, const Path &destination) const {
    const auto sourceText = pathTextOrThrow(source);
    const auto destinationText = pathTextOrThrow(destination);
    const auto sourceAccess = text::impl::UnsafeU8StringViewAccess{sourceText};
    const auto destinationAccess = text::impl::UnsafeU8StringViewAccess{destinationText};
    const auto sourceDescriptor = ::open(sourceAccess.data(), O_RDONLY);
    if (sourceDescriptor < 0) {
        throwSystemError(
            "File could not be copied"_el, "The source file could not be opened for copying."_el, source, errno);
    }
    const auto destinationDescriptor = ::open(destinationAccess.data(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (destinationDescriptor < 0) {
        const auto error = errno;
        static_cast<void>(::close(sourceDescriptor));
        throwSystemError(
            "File could not be copied"_el,
            "The destination file could not be created for copying."_el,
            source,
            destination,
            error);
    }
    try {
        auto buffer = std::array<char, 64U * 1024U>{};
        while (true) {
            const auto readSize = ::read(sourceDescriptor, buffer.data(), buffer.size());
            if (readSize == 0) {
                break;
            }
            if (readSize < 0) {
                if (errno == EINTR) {
                    continue;
                }
                throwSystemError("File could not be copied"_el, "The source file could not be read."_el, source, errno);
            }
            auto written = ssize_t{0};
            while (written < readSize) {
                const auto writeSize = ::write(
                    destinationDescriptor,
                    buffer.data() + static_cast<std::size_t>(written),
                    static_cast<std::size_t>(readSize - written));
                if (writeSize < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    throwSystemError(
                        "File could not be copied"_el,
                        "The destination file could not be written."_el,
                        source,
                        destination,
                        errno);
                }
                written += writeSize;
            }
        }
    } catch (...) {
        static_cast<void>(::close(sourceDescriptor));
        static_cast<void>(::close(destinationDescriptor));
        static_cast<void>(::unlink(destinationAccess.data()));
        throw;
    }
    static_cast<void>(::close(sourceDescriptor));
    if (::close(destinationDescriptor) != 0) {
        const auto error = errno;
        static_cast<void>(::unlink(destinationAccess.data()));
        throwSystemError(
            "File could not be copied"_el,
            "The destination file could not be finalized."_el,
            source,
            destination,
            error);
    }
}

void PosixPathBackend::moveEntryOrThrow(const Path &source, const Path &destination) const {
    const auto sourceText = pathTextOrThrow(source);
    const auto destinationText = pathTextOrThrow(destination);
    const auto sourceAccess = text::impl::UnsafeU8StringViewAccess{sourceText};
    const auto destinationAccess = text::impl::UnsafeU8StringViewAccess{destinationText};
    if (::rename(sourceAccess.data(), destinationAccess.data()) != 0) {
        throwSystemError(
            "Path could not be moved"_el,
            "The operating system could not move the path on the same filesystem."_el,
            source,
            destination,
            errno);
    }
}

auto PosixPathBackend::readSymlinkOrThrow(const Path &path) const -> Path {
    const auto pathText = pathTextOrThrow(path);
    const auto pathAccess = text::impl::UnsafeU8StringViewAccess{pathText};
    auto bufferSize = std::size_t{256U};
    while (true) {
        auto buffer = text::impl::UnsafeU8StringBuffer{bufferSize};
        const auto length = ::readlink(pathAccess.data(), buffer.data(), buffer.dataSize());
        if (length < 0) {
            throwSystemError(
                "Symbolic link could not be read"_el,
                "The operating system could not read the symbolic-link target."_el,
                path,
                errno);
        }
        if (static_cast<std::size_t>(length) < buffer.dataSize()) {
            return Path::fromPosixOrThrow(buffer.take(unit::ByteLength::fromSizeT(static_cast<std::size_t>(length))));
        }
        bufferSize *= 2U;
    }
}

void PosixPathBackend::createSymlinkOrThrow(
    const Path &target, const Path &path, [[maybe_unused]] const bool targetIsDirectory) const {
    const auto targetText = pathTextOrThrow(target);
    const auto pathText = pathTextOrThrow(path);
    const auto targetAccess = text::impl::UnsafeU8StringViewAccess{targetText};
    const auto pathAccess = text::impl::UnsafeU8StringViewAccess{pathText};
    if (::symlink(targetAccess.data(), pathAccess.data()) != 0) {
        throwSystemError(
            "Symbolic link could not be created"_el,
            "The operating system could not create the symbolic link."_el,
            target,
            path,
            errno);
    }
}

}
