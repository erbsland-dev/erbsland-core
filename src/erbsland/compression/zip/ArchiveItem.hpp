// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveExtractionOptions.hpp"
#include "ArchiveItem_fwd.hpp"
#include "CompressionMethod.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../path/Path.hpp"
#include "../../stream/ByteOutputStream.hpp"
#include "../../text/String.hpp"
#include "../../time/DateTime.hpp"
#include "../../unit/ByteLength.hpp"

#include <cstdint>

namespace erbsland::compression::zip {

/// Immutable metadata and extraction access for one ZIP archive entry.
/// Metadata remains available after its reader closes; payload operations require the reader to remain open.
/// @seedoc{/reference/compression/zip_archives}
/// @tested{ZipArchiveTest}
class ArchiveItem {
protected: // defaults
    ArchiveItem() = default;

public:
    /// Decode into a borrowed stream; output is provisional until integrity validation succeeds.
    virtual void extractToStream(
        stream::ByteOutputStream &destination, ArchiveExtractionOptions options = {}) const = 0;
    // defaults/deletions
    virtual ~ArchiveItem() = default;
    ArchiveItem(const ArchiveItem &) = delete;
    ArchiveItem(ArchiveItem &&) = delete;
    auto operator=(const ArchiveItem &) -> ArchiveItem & = delete;
    auto operator=(ArchiveItem &&) -> ArchiveItem & = delete;

public: // metadata
    /// Get the normalized portable relative path.
    [[nodiscard]] virtual auto path() const noexcept -> const path::Path & = 0;
    /// Test whether this is an explicit directory entry.
    [[nodiscard]] virtual auto isDirectory() const noexcept -> bool = 0;
    /// Get the method used for this entry payload.
    [[nodiscard]] virtual auto compressionMethod() const noexcept -> CompressionMethod = 0;
    /// Get the preferred Extended Timestamp or DOS fallback modification time.
    [[nodiscard]] virtual auto lastModificationTime() const noexcept -> const time::DateTime & = 0;
    /// Get the declared CRC-32 of the uncompressed bytes.
    [[nodiscard]] virtual auto crc32() const noexcept -> uint32_t = 0;
    /// Get the declared compressed payload length.
    [[nodiscard]] virtual auto compressedLength() const noexcept -> unit::ByteLength = 0;
    /// Get the declared uncompressed payload length.
    [[nodiscard]] virtual auto uncompressedLength() const noexcept -> unit::ByteLength = 0;
    /// Get the entry comment decoded as tolerant UTF-8.
    [[nodiscard]] virtual auto comment() const noexcept -> const text::String & = 0;

public: // payload access
    /// Extract and validate this entry into memory.
    /// @throws ZipError If the reader is closed, the limit is exceeded, decoding fails, or CRC/length validation fails.
    [[nodiscard]] virtual auto extract(unit::ByteLength maximumLength) const -> mem::ByteBlock = 0;
    /// Extract a file entry to an explicit destination path.
    virtual void extractToFile(const path::Path &destination, ArchiveExtractionOptions options = {}) const = 0;
    /// Extract this entry below a root while retaining its archive path.
    virtual void extractToDirectory(const path::Path &root, ArchiveExtractionOptions options = {}) const = 0;
};

}
