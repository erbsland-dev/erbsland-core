// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveItem_fwd.hpp"
#include "ArchiveReader.hpp"
#include "ZipEntryRecord.hpp"

#include "../ArchiveItem.hpp"

#include "../../../stream/ByteOutputStream.hpp"

namespace erbsland::compression::zip::impl {

/// Concrete immutable ZIP entry bound weakly to its owning reader.
/// @tested{ZipArchiveTest}
class ArchiveItem final : public zip::ArchiveItem {
public:
    /// Decode into a borrowed stream; output is provisional until integrity validation succeeds.
    void extractToStream(stream::ByteOutputStream &destination, ArchiveExtractionOptions options = {}) const override;

    /// Create an immutable item bound weakly to its reader.
    ArchiveItem(std::weak_ptr<ArchiveReader> reader, ZipEntryRecord record);

public: // implement zip::ArchiveItem
    /// Implement `zip::ArchiveItem::path()`.
    [[nodiscard]] auto path() const noexcept -> const path::Path & override;
    /// Implement `zip::ArchiveItem::isDirectory()`.
    [[nodiscard]] auto isDirectory() const noexcept -> bool override;
    /// Implement `zip::ArchiveItem::compressionMethod()`.
    [[nodiscard]] auto compressionMethod() const noexcept -> CompressionMethod override;
    /// Implement `zip::ArchiveItem::lastModificationTime()`.
    [[nodiscard]] auto lastModificationTime() const noexcept -> const time::DateTime & override;
    /// Implement `zip::ArchiveItem::crc32()`.
    [[nodiscard]] auto crc32() const noexcept -> uint32_t override;
    /// Implement `zip::ArchiveItem::compressedLength()`.
    [[nodiscard]] auto compressedLength() const noexcept -> unit::ByteLength override;
    /// Implement `zip::ArchiveItem::uncompressedLength()`.
    [[nodiscard]] auto uncompressedLength() const noexcept -> unit::ByteLength override;
    /// Implement `zip::ArchiveItem::comment()`.
    [[nodiscard]] auto comment() const noexcept -> const text::String & override;
    /// Implement `zip::ArchiveItem::extract()`.
    [[nodiscard]] auto extract(unit::ByteLength maximumLength) const -> mem::ByteBlock override;
    /// Implement `zip::ArchiveItem::extractToFile()`.
    void extractToFile(const path::Path &destination, ArchiveExtractionOptions options = {}) const override;
    /// Implement `zip::ArchiveItem::extractToDirectory()`.
    void extractToDirectory(const path::Path &root, ArchiveExtractionOptions options = {}) const override;

public: // writer backend
    /// Access the complete validated record metadata.
    [[nodiscard]] auto record() const noexcept -> const ZipEntryRecord & { return _record; }
    /// Read the validated local metadata and compressed payload for raw archive reassembly.
    [[nodiscard]] auto entryData() const -> ZipEntryData;
    /// Copy the compressed payload in bounded chunks.
    void copyPayload(const compression::impl::CodecOutput::Write &output) const;

public: // bulk extraction backend
    /// Create or accept this explicit directory and report whether its timestamp may be restored.
    [[nodiscard]] auto prepareDirectory(const path::Path &root, const ArchiveExtractionOptions &options) const -> bool;
    /// Restore this explicit directory timestamp after all selected children were extracted.
    void restoreDirectoryModificationTime(const path::Path &root, const ArchiveExtractionOptions &options) const;

private:
    /// Lock and validate the weak reader association.
    [[nodiscard]] auto requireReader() const -> std::shared_ptr<ArchiveReader>;
    /// Reject existing symbolic links or reparse points from the destination through the trusted extraction root.
    void verifySafeParent(const path::Path &destination, const path::Path &trustedRoot) const;
    /// Restore the validated modification time when requested.
    void restoreModificationTime(const path::Path &destination, const ArchiveExtractionOptions &options) const;
    /// Decode, verify, and atomically commit one file entry.
    void writeExtracted(
        const path::Path &destination, ArchiveExtractionOptions options, const path::Path &trustedRoot) const;

private:
    std::weak_ptr<ArchiveReader> _reader;
    ZipEntryRecord _record;
};

}
