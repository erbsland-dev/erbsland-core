// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveDirectoryOptions.hpp"
#include "ArchiveItem.hpp"
#include "ArchiveWriter_fwd.hpp"
#include "ArchiveWriterOptions.hpp"

#include "../CompressionLevel.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../path/Path.hpp"
#include "../../stream/ByteInputStream_fwd.hpp"
#include "../../stream/ByteOutputStream_fwd.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::compression::zip {

/// A synchronous, append-and-finalize interface for creating one ZIP archive.
/// Record emission follows APPNOTE 6.3.10 sections 4.3.6 through 4.3.16 and 4.5.3.
/// @seedoc{/reference/compression/zip_archives}
/// Archive operations are unsuitable for sensitive data; codec storage is not securely erased.
/// @tested{ZipArchiveTest}
class ArchiveWriter {
protected: // defaults
    ArchiveWriter() = default;

public: // defaults/deletions
    virtual ~ArchiveWriter() = default;
    ArchiveWriter(const ArchiveWriter &) = delete;
    ArchiveWriter(ArchiveWriter &&) = delete;
    auto operator=(const ArchiveWriter &) -> ArchiveWriter & = delete;
    auto operator=(ArchiveWriter &&) -> ArchiveWriter & = delete;

public: // factories
    /// Create a path-backed writer using a sibling temporary archive.
    [[nodiscard]] static auto create(const path::Path &destination, ArchiveWriterOptions options = {})
        -> ArchiveWriterPtr;
    /// Create a writer that owns the supplied output stream.
    [[nodiscard]] static auto create(stream::ByteOutputStreamPtr destination, ArchiveWriterOptions options = {})
        -> ArchiveWriterPtr;

public: // persistent defaults
    /// Set the compression method used by subsequent entries without an override.
    virtual void setCompressionMethod(CompressionMethod value) = 0;
    /// Set the compression effort used by subsequent entries without an override.
    virtual void setCompressionLevel(compression::CompressionLevel value) = 0;
    /// Set the directory removed from implicit `addFile()` storage paths.
    virtual void setBaseDirectory(path::Path value) = 0;
    /// Set the archive comment encoded into the final EOCD record.
    virtual void setComment(text::String value) = 0;
    /// Set whether an unfinished path-backed temporary archive is removed.
    virtual void setCleanupEnabled(bool value) noexcept = 0;

public: // entries
    /// Read, compress, and add one regular file.
    virtual void addFile(
        const path::Path &source, const path::Path &storagePath = {}, ArchiveEntryOptions options = {}) = 0;
    /// Compress and add one in-memory item.
    virtual void addData(
        const mem::ByteBlock &data, const path::Path &storagePath, ArchiveEntryOptions options = {}) = 0;
    /// Own and consume all or an exact prefix of an input stream, then close it.
    virtual void addStream(
        stream::ByteInputStreamPtr source,
        const path::Path &storagePath,
        ArchiveEntryOptions options = {},
        unit::ByteLength length = unit::ByteLength::infinite()) = 0;
    /// Traverse and add a directory according to the supplied filters.
    virtual void addDirectory(const path::Path &source, ArchiveDirectoryOptions options = {}) = 0;
    /// Copy one compressed payload and its opaque extra fields without decoding it.
    virtual void addItem(const ArchiveItem &source) = 0;

public: // lifecycle
    /// Write directory/end records, close the stream, and atomically commit a path destination.
    virtual void finalize() = 0;
    /// Immediately abandon the writer and apply its cleanup policy.
    virtual void abort() noexcept = 0;
    /// Test whether finalization completed successfully.
    [[nodiscard]] virtual auto isFinalized() const noexcept -> bool = 0;
    /// Get a retained incomplete path, or an empty path when none exists.
    [[nodiscard]] virtual auto temporaryPath() const noexcept -> const path::Path & = 0;
};

}
