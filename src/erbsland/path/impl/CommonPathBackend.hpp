// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathBackend.hpp"

#include "../Path_fwd.hpp"

#include "../../text/StringList.hpp"

namespace erbsland::path::impl {

/// A common implementation of the path backend with functionality that is shared between platforms.
class CommonPathBackend : public PathBackend {
protected:
    /// The result of opening a byte output stream.
    struct PathByteOutputStreamOpenResult {
        stream::ByteOutputStreamPtr stream; ///< The opened stream.
        bool hasExistingContent{false};     ///< If the opened target already had content after opening.
    };

public:
    ~CommonPathBackend() override = default;

public: // implement PathBackend
    [[nodiscard]] auto toAbsoluteOrThrow(const Path &path, std::optional<Path> base) const -> Path override;
    [[nodiscard]] auto toRelativeOrThrow(const Path &path, std::optional<Path> base) const -> Path override;
    [[nodiscard]] auto isRelativeTo(const Path &path, std::optional<Path> base) const noexcept -> bool override;
    [[nodiscard]] auto commonAncestor(const Path &path, std::optional<Path> base) const noexcept -> Path override;
    [[nodiscard]] auto openTextInputStreamOrThrow(const Path &path, PathReadTextOptions options) const
        -> stream::TextInputStreamPtr override;
    [[nodiscard]] auto openByteOutputStreamOrThrow(const Path &path, PathWriteDataOptions options) const
        -> stream::ByteOutputStreamPtr override;
    [[nodiscard]] auto openTextOutputStreamOrThrow(const Path &path, PathWriteTextOptions options) const
        -> stream::TextOutputStreamPtr override;

protected:
    /// Convert a path to an absolute path and lexically normalize it.
    [[nodiscard]] auto absoluteLexicalPathOrThrow(const Path &path) const -> Path;
    /// Lexically normalize a path without touching the file system.
    [[nodiscard]] static auto lexicalPath(const Path &path) -> Path;
    /// Join two paths and lexically normalize the result.
    [[nodiscard]] static auto joinedLexicalPath(const Path &base, const Path &suffix) -> Path;
    /// Test if two absolute paths have the same root.
    [[nodiscard]] static auto haveSameRoot(const Path &left, const Path &right) noexcept -> bool;
    /// Assemble a path from a root and normalized non-root elements.
    [[nodiscard]] static auto assemblePath(const text::String &root, const text::StringList &elements) -> Path;
    /// Open a byte output stream and report if the opened target already has content.
    [[nodiscard]] virtual auto openByteOutputStreamWithExistingContentOrThrow(
        const Path &path, PathWriteDataOptions options) const -> PathByteOutputStreamOpenResult = 0;
};

}
