// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CommonPathBackend.hpp"

#include "../PathAccessInfo.hpp"
#include "../PathAccessProfile.hpp"
#include "../PathAttribute.hpp"
#include "../PathType.hpp"

#include "../../text/impl/UnsafeU16StringBuffer_fwd.hpp"
#include "../../text/impl/UnsafeU16StringEditorAccess_fwd.hpp"
#include "../../text/StringList.hpp"
#include "../../text/u16/U16StringEditor.hpp"

#include <cstdint>

namespace erbsland::path::impl {

/// The Windows path backend.
class WindowsPathBackend : public CommonPathBackend {
    struct SymbolicLinkReparseData {
        std::uint32_t reparseTag;
        std::uint16_t reparseDataLength;
        std::uint16_t reserved;
        std::uint16_t substituteNameOffset;
        std::uint16_t substituteNameLength;
        std::uint16_t printNameOffset;
        std::uint16_t printNameLength;
        std::uint32_t flags;
        wchar_t pathBuffer[1];
    };

    static constexpr std::uint32_t cSymlinkReparseFlagRelative = 1U;

public:
    WindowsPathBackend() = default;

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
    [[nodiscard]] static auto typeFromAttributes(
        unsigned long attributes, unsigned long reparseTag, unsigned long fileType) noexcept -> PathType;
    [[nodiscard]] static auto accessInfoFromAttributes(unsigned long attributes) noexcept -> PathAccessInfo;
    [[nodiscard]] static auto pathAttributesFromWindowsAttributes(unsigned long attributes) noexcept -> PathAttributes;
    [[nodiscard]] static auto windowsAttributesFromPathAttributes(PathAttributes attributes) -> unsigned long;
    [[nodiscard]] static auto sidString(void *sid) -> text::String;
    [[nodiscard]] static auto pathTextOrThrow(const Path &path) -> text::U16String;
    static void createParentDirectoriesOrThrow(const Path &path);
    [[nodiscard]] static auto handleHasContentOrThrow(void *handle, const Path &path) -> bool;
    [[nodiscard]] static auto physicalPathOrThrow(const Path &path) -> Path;
    [[nodiscard]] static auto existingPath(const Path &path) -> bool;
    [[nodiscard]] static auto weakPathOrThrow(const Path &path) -> Path;
    [[nodiscard]] static auto physicalNoFinalSymlinkPathOrThrow(const Path &path) -> Path;
    [[nodiscard]] static auto nonRootElements(const Path &path) -> text::StringList;
    [[noreturn]] static void throwSystemError(
        const text::String &title, const text::String &description, const Path &path, unsigned long errorCode);
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
