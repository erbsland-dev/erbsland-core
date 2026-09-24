// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipEntryRecord.hpp"

#include "../ArchiveWriter.hpp"
#include "../ZipError.hpp"

#include "../../../stream/ByteOutputStream.hpp"
#include "../../../stream/TempByteOutputStream_fwd.hpp"

#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace erbsland::compression::zip::impl {

/// APPNOTE 6.3.10 ZIP/ZIP64 writer implementation.
/// @tested{ZipArchiveTest}
class ArchiveWriter final : public zip::ArchiveWriter {
public:
    /// Create a writer that owns its destination and optional temporary stream.
    ArchiveWriter(
        stream::ByteOutputStreamPtr destination,
        stream::TempByteOutputStreamPtr temporary,
        path::Path destinationPath,
        ArchiveWriterOptions options);
    /// Abort an unfinished archive according to the cleanup policy.
    ~ArchiveWriter() override;

public: // implement zip::ArchiveWriter
    /// Implement `zip::ArchiveWriter::setCompressionMethod()`.
    void setCompressionMethod(CompressionMethod value) override;
    /// Implement `zip::ArchiveWriter::setCompressionLevel()`.
    void setCompressionLevel(compression::CompressionLevel value) override;
    /// Implement `zip::ArchiveWriter::setBaseDirectory()`.
    void setBaseDirectory(path::Path value) override;
    /// Implement `zip::ArchiveWriter::setComment()`.
    void setComment(text::String value) override;
    /// Implement `zip::ArchiveWriter::setCleanupEnabled()`.
    void setCleanupEnabled(bool value) noexcept override;
    /// Implement `zip::ArchiveWriter::addFile()`.
    void addFile(
        const path::Path &source, const path::Path &storagePath = {}, ArchiveEntryOptions options = {}) override;
    /// Implement `zip::ArchiveWriter::addData()`.
    void addData(const mem::ByteBlock &data, const path::Path &storagePath, ArchiveEntryOptions options = {}) override;
    /// Implement `zip::ArchiveWriter::addStream()`.
    void addStream(
        stream::ByteInputStreamPtr source,
        const path::Path &storagePath,
        ArchiveEntryOptions options = {},
        unit::ByteLength length = unit::ByteLength::infinite()) override;
    /// Implement `zip::ArchiveWriter::addDirectory()`.
    void addDirectory(const path::Path &source, ArchiveDirectoryOptions options = {}) override;
    /// Implement `zip::ArchiveWriter::addItem()`.
    void addItem(const zip::ArchiveItem &source) override;
    /// Implement `zip::ArchiveWriter::finalize()`.
    void finalize() override;
    /// Implement `zip::ArchiveWriter::abort()`.
    void abort() noexcept override;
    /// Implement `zip::ArchiveWriter::isFinalized()`.
    [[nodiscard]] auto isFinalized() const noexcept -> bool override;
    /// Implement `zip::ArchiveWriter::temporaryPath()`.
    [[nodiscard]] auto temporaryPath() const noexcept -> const path::Path & override;

private:
    /// One emitted local record retained until the central directory is written.
    struct WrittenEntry final {
        ZipEntryRecord record;
        mem::ByteBlock name;
        mem::ByteBlock centralExtra;
        mem::ByteBlock comment;
    };

private:
    /// Require that no successful finalization or abort occurred.
    void requireWritable() const;
    /// Normalize a caller-supplied storage path and reject unsafe components.
    [[nodiscard]] auto normalizedStoragePath(const path::Path &value, bool directory = false) const -> path::Path;
    /// Validate and emit a complete known-size local entry record.
    void addCompressed(
        const mem::ByteBlock &payload,
        const path::Path &storagePath,
        CompressionMethod method,
        unit::ByteLength uncompressedLength,
        uint32_t crc32,
        time::DateTime modificationTime,
        text::String comment,
        mem::ByteBlock opaqueLocalExtra = {},
        mem::ByteBlock opaqueCentralExtra = {},
        bool directory = false,
        std::function<void(ZipEntryRecord &)> producer = {},
        std::optional<unit::ByteLength> compressedBound = {},
        std::optional<uint16_t> methodFlags = {});

    /// Emit central-directory and classic or ZIP64 end records.
    void writeCentralDirectory();
    /// Emit one central-directory entry.
    void writeCentralEntry(const WrittenEntry &entry);
    /// Emit classic and optional ZIP64 end records.
    void writeEndRecords(unit::ByteLength directoryOffset, unit::ByteLength directoryLength, bool archiveNeedsZip64);
    /// Write one complete record block and advance the checked output offset.
    void write(const mem::ByteBlock &data);
    /// Atomically move a completed sibling temporary archive into place.
    void commitPathDestination();
    /// Apply include then exclude globs to one normalized directory path.
    [[nodiscard]] auto includeDirectoryPath(
        const text::String &storagePath, const ArchiveDirectoryOptions &options) const -> bool;
    /// Throw a ZIP error carrying the writer destination path.
    [[noreturn]] void throwWriterError(
        ZipErrorReason reason, ZipOperationPhase phase, text::String title, text::String description) const;

private:
    stream::ByteOutputStreamPtr _destination;
    stream::TempByteOutputStreamPtr _temporary;
    path::Path _destinationPath;
    path::Path _temporaryPath;
    ArchiveWriterOptions _options;
    CompressionMethod _compressionMethod{CompressionMethod::Deflate};
    compression::CompressionLevel _compressionLevel{compression::CompressionLevel::Default};
    path::Path _baseDirectory;
    mem::ByteBlock _comment;
    std::vector<WrittenEntry> _entries;
    std::map<path::Path, bool> _paths;
    unit::ByteLength _offset;
    bool _cleanupEnabled{true};
    bool _finalized{};
    bool _aborted{};
};

}
