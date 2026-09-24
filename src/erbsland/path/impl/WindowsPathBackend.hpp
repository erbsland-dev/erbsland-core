// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CommonPathBackend.hpp"
#include "WindowsSymbolicLinkReparseData.hpp"

#include "../PathAccessInfo.hpp"
#include "../PathAccessProfile.hpp"
#include "../PathAttribute.hpp"
#include "../PathType.hpp"

#include "../../text/impl/UnsafeU16StringBuffer_fwd.hpp"
#include "../../text/StringList.hpp"
#include "../../text/u16/U16StringEditor.hpp"

#include <cstdint>

namespace erbsland::path::impl {

/// The Windows path backend.
class WindowsPathBackend : public CommonPathBackend {
    /// Mark a symbolic-link reparse target as relative.
    static constexpr std::uint32_t cSymlinkReparseFlagRelative = 1U;

public:
    /// Create the Windows path backend.
    WindowsPathBackend() = default;

public: // implement PathBackend
    [[nodiscard]] auto currentDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto userHomeDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto systemTempDirectoryOrThrow() const -> Path override;
    [[nodiscard]] auto resolveOrThrow(const Path &path, PathResolveOptions options) const -> Path override;
    [[nodiscard]] auto loadInfoOrThrow(const Path &path, PathInfoParts parts) const -> PathInfoData override;
    [[nodiscard]] auto loadResolvedInfoOrThrow(const Path &path, const Path &resolvedPath, PathInfoParts parts) const
        -> PathInfoData override;
    [[nodiscard]] auto directoryEntriesOrThrow(const Path &path, const Path &resolvedPath) const
        -> std::vector<Path> override;
    void createDirectoryEntryOrThrow(const Path &path, PathAccessProfile profile) const override;
    void removeEntryOrThrow(const Path &path) const override;
    void copyFileEntryOrThrow(const Path &source, const Path &destination) const override;
    void moveEntryOrThrow(const Path &source, const Path &destination) const override;
    [[nodiscard]] auto readSymlinkOrThrow(const Path &path) const -> Path override;
    void createSymlinkOrThrow(const Path &target, const Path &path, bool targetIsDirectory) const override;
    [[nodiscard]] auto openByteInputStreamOrThrow(const Path &path, PathReadDataOptions options) const
        -> stream::ByteInputStreamPtr override;
    void setAccessProfileOrThrow(const Path &path, PathAccessProfile profile, PathChangeOptions options) const override;
    void setLastModifiedOrThrow(
        const Path &path, const time::DateTime &value, PathChangeOptions options) const override;
    void addAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const override;
    void clearAttributesOrThrow(const Path &path, PathAttributes attributes, PathChangeOptions options) const override;

private:
    /// Map native attributes to a library path type.
    [[nodiscard]] static auto typeFromAttributes(
        unsigned long attributes, unsigned long reparseTag, unsigned long fileType) noexcept -> PathType;
    /// Map native attributes to path-access information.
    [[nodiscard]] static auto accessInfoFromAttributes(unsigned long attributes) noexcept -> PathAccessInfo;
    /// Map native attributes to library attributes.
    [[nodiscard]] static auto pathAttributesFromWindowsAttributes(unsigned long attributes) noexcept -> PathAttributes;
    /// Map library attributes to native attributes.
    [[nodiscard]] static auto windowsAttributesFromPathAttributes(PathAttributes attributes) -> unsigned long;
    /// Convert a Windows SID to text.
    [[nodiscard]] static auto sidString(void *sid) -> text::String;
    /// Convert a path to Windows UTF-16 text.
    [[nodiscard]] static auto pathTextOrThrow(const Path &path) -> text::U16String;
    /// Create the missing parent directories for `path`.
    static void createParentDirectoriesOrThrow(const Path &path);
    /// Test whether a native handle has content.
    [[nodiscard]] static auto handleHasContentOrThrow(void *handle, const Path &path) -> bool;
    /// Open an input handle using the configured symbolic-link policy.
    [[nodiscard]] static auto openInputHandleOrThrow(const Path &path, SymlinkMode symlinkMode) -> void *;
    /// Resolve a path through all symbolic links.
    [[nodiscard]] static auto physicalPathOrThrow(const Path &path) -> Path;
    /// Test whether `path` exists.
    [[nodiscard]] static auto existingPath(const Path &path) -> bool;
    /// Resolve the existing prefix of a path.
    [[nodiscard]] static auto weakPathOrThrow(const Path &path) -> Path;
    /// Resolve a path except for its final symbolic link.
    [[nodiscard]] static auto physicalNoFinalSymlinkPathOrThrow(const Path &path) -> Path;
    /// Get the path elements excluding its root.
    [[nodiscard]] static auto nonRootElements(const Path &path) -> text::StringList;
    /// Throw a system error for one path.
    [[noreturn]] static void throwSystemError(
        const text::String &title, const text::String &description, const Path &path, unsigned long errorCode);
    /// Throw a system error involving source and destination paths.
    [[noreturn]] static void throwSystemError(
        const text::String &title,
        const text::String &description,
        const Path &source,
        const Path &destination,
        unsigned long errorCode);

protected:
    [[nodiscard]] auto openByteOutputStreamWithExistingContentOrThrow(
        const Path &path, PathWriteDataOptions options) const -> PathByteOutputStreamOpenResult override;
};

}
