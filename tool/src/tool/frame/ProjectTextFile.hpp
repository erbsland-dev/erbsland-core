// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteCount.hpp>
#include <erbsland/FileIdentity.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/Timestamp.hpp>

namespace tool::frame {

/// Immutable text and file metadata captured during a confined read.
class ProjectTextFile final {
    friend class ProjectFiles;

private:
    /// Capture one confined path before the file service reads its contents.
    explicit ProjectTextFile(el::Path path);

public:
    /// Create an empty snapshot that never matches a filesystem object.
    ProjectTextFile() = default;

public: // accessors
    /// Get the original absolute file path, retaining its metadata cache.
    [[nodiscard]] auto path() const noexcept -> const el::Path & { return _path; }
    /// Get the text read from the file.
    [[nodiscard]] auto text() const noexcept -> const el::String & { return _text; }

private:
    /// Reload metadata to detect replacement, edits, or a redirected path.
    [[nodiscard]] auto isCurrent() const -> bool;

private:
    el::Path _path;                 ///< Confined absolute path.
    el::String _text;               ///< Original text.
    el::FileIdentity _identity;     ///< Original file identity.
    el::ByteCount _size;            ///< Original byte length.
    el::Timestamp _modified;        ///< Original modification time.
    el::Timestamp _metadataChanged; ///< Original metadata-change time.
};

}
