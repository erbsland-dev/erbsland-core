// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CommonPathBackend.hpp"

#include "../PathAccessInfo.hpp"
#include "../PathAccessProfile.hpp"
#include "../PathType.hpp"

#include "../../text/StringEditor.hpp"
#include "../../text/StringList.hpp"

#include <sys/stat.h>
#include <sys/types.h>

namespace erbsland::path::impl {

/// The POSIX path backend.
class PosixPathBackend : public CommonPathBackend {
public:
    PosixPathBackend() = default;

public: // implement PathBackend
    [[nodiscard]] auto currentDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto systemTempDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto resolveOrThrow(const Path &path, PathResolveOptions options) const -> Path override;
    [[nodiscard]] auto loadInfoOrThrow(const Path &path, PathInfoParts parts) const -> PathInfoData override;
    [[nodiscard]] auto directoryEntriesOrThrow(const Path &path) const -> std::vector<Path> override;
    void createDirectoryEntryOrThrow(const Path &path, PathAccessProfile profile) const override;
    void removeEntryOrThrow(const Path &path) const override;
    void copyFileEntryOrThrow(const Path &source, const Path &destination) const override;
    void moveEntryOrThrow(const Path &source, const Path &destination) const override;
    [[nodiscard]] auto readSymlinkOrThrow(const Path &path) const -> Path override;
    void createSymlinkOrThrow(const Path &target, const Path &path, bool targetIsDirectory) const override;
    [[nodiscard]] auto openByteInputStreamOrThrow(const Path &path, PathReadDataOptions options) const
        -> stream::ByteInputStreamPtr override;
    void setAccessProfileOrThrow(const Path &path, PathAccessProfile profile, PathChangeOptions options) const override;
    void addAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const override;
    void clearAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const override;

private:
    [[nodiscard]] static auto pathTextOrThrow(const Path &path) -> text::String;
    static void createParentDirectoriesOrThrow(const Path &path);
    [[nodiscard]] static auto fileDescriptorHasContentOrThrow(int fileDescriptor, const Path &path) -> bool;
    [[nodiscard]] static auto typeFromMode(mode_t mode) noexcept -> PathType;
    [[nodiscard]] static auto accessInfoFromStatus(const struct stat &info, PathType type) -> PathAccessInfo;
    [[nodiscard]] static auto rightsFromMode(mode_t mode, unsigned int shift) noexcept -> PathAccessRights;
    [[nodiscard]] static auto profileMode(PathAccessProfile profile, PathType type) noexcept -> mode_t;
    static void applyAccessProfileToDescriptorOrThrow(
        int fileDescriptor, const Path &path, PathAccessProfile profile, PathType type);
    static void applyAttributesOrThrow(const Path &path, PathAttributes attributes, bool set);
    [[nodiscard]] static auto physicalPathOrThrow(const Path &path) -> Path;
    [[nodiscard]] static auto existingPath(const Path &path) -> bool;
    [[nodiscard]] static auto weakPathOrThrow(const Path &path) -> Path;
    [[nodiscard]] static auto physicalNoFinalSymlinkPathOrThrow(const Path &path) -> Path;
    [[noreturn]] static void throwSystemError(
        const text::String &title, const text::String &description, const Path &path, int errorCode);
    [[noreturn]] static void throwSystemError(
        const text::String &title,
        const text::String &description,
        const Path &source,
        const Path &destination,
        int errorCode);

protected:
    [[nodiscard]] auto openByteOutputStreamWithExistingContentOrThrow(
        const Path &path, PathWriteDataOptions options) const -> PathByteOutputStreamOpenResult override;
};

}
