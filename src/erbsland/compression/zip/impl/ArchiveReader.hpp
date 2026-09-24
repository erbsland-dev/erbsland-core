// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveItem_fwd.hpp"
#include "ZipEntryRecord.hpp"

#include "../ArchiveReader.hpp"
#include "../ZipError.hpp"

#include "../../../mem/ByteReader_fwd.hpp"
#include "../../../stream/ByteInputStream.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../impl/CodecOutput.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace erbsland::compression::zip::impl {

/// Strict APPNOTE 6.3.10 ZIP/ZIP64 reader implementation.
/// @tested{ZipArchiveTest}
class ArchiveReader final : public zip::ArchiveReader, public std::enable_shared_from_this<ArchiveReader> {
public:
    /// Create an unopened reader that owns its source.
    ArchiveReader(stream::ByteInputStreamPtr source, path::Path sourcePath, ArchiveReaderOptions options);
    /// Close the owned source if it is still open.
    ~ArchiveReader() override;

public:
    /// Index the source and validate its end records and central directory.
    void open();

public: // implement zip::ArchiveReader
    /// Implement `zip::ArchiveReader::itemCount()`.
    [[nodiscard]] auto itemCount() const noexcept -> unit::ItemCount override;
    /// Implement `zip::ArchiveReader::item()`.
    [[nodiscard]] auto item(unit::ItemIndex index) const noexcept -> ArchiveItemPtr override;
    /// Implement `zip::ArchiveReader::items()`.
    [[nodiscard]] auto items() const -> ArchiveItemList override;
    /// Implement `zip::ArchiveReader::comment()`.
    [[nodiscard]] auto comment() const noexcept -> const text::String & override;
    /// Implement `zip::ArchiveReader::isZip64()`.
    [[nodiscard]] auto isZip64() const noexcept -> bool override;
    /// Implement `zip::ArchiveReader::extractToDirectory()`.
    void extractToDirectory(
        const path::Path &root, ArchiveExtractionOptions options = {}, Filter filter = {}) const override;
    /// Implement `zip::ArchiveReader::close()`.
    void close() noexcept override;
    /// Implement `zip::ArchiveReader::isOpen()`.
    [[nodiscard]] auto isOpen() const noexcept -> bool override;

public: // item backend
    /// Decode and verify one already validated item.
    [[nodiscard]] auto extract(const ZipEntryRecord &record, unit::ByteLength maximumLength) const -> mem::ByteBlock;
    /// Read and validate one local record and its compressed payload.
    [[nodiscard]] auto entryData(const ZipEntryRecord &record, bool readPayload = true) const -> ZipEntryData;
    /// Stream decoded bytes while validating exact length and CRC.
    void extractTo(
        const ZipEntryRecord &record,
        unit::ByteLength maximum,
        const compression::impl::CodecOutput::Write &output,
        ZipProgressFn progress = {}) const;
    /// Copy validated compressed bytes without decoding.
    void copyPayload(const ZipEntryRecord &record, const compression::impl::CodecOutput::Write &output) const;

private:
    /// Conditional values decoded from an APPNOTE 4.5.3 ZIP64 extra field.
    struct Zip64Values final {
        unit::ByteLength uncompressedLength;
        unit::ByteLength compressedLength;
        unit::ByteIndex localHeaderOffset;
        uint32_t diskStart{};
    };

private:
    /// Require positioning and determine the source length.
    void indexArchive();
    /// Locate and validate the APPNOTE central directory and end records.
    void parseDirectory();
    /// Find the classic end record in a tail containing at least its fixed 22-byte header.
    [[nodiscard]] auto findEndRecord(const mem::ByteBlock &tail) const -> unit::ByteIndex;
    /// Resolve ZIP64 directory fields and return the start of the end records.
    [[nodiscard]] auto parseZip64Directory(
        mem::ByteReader &tailReader,
        unit::ByteIndex endOffsetInTail,
        unit::ByteIndex endOffset,
        uint64_t &entryCount,
        unit::ByteLength &directoryLength,
        unit::ByteIndex &directoryOffset) -> unit::ByteIndex;
    /// Validate a signed or unsigned descriptor at the current payload-end position.
    void validateDataDescriptor(const ZipEntryRecord &record, unit::ByteIndex payloadEnd, bool zip64Descriptor) const;
    /// Validate local size and CRC fields against the immutable directory metadata.
    void validateLocalSizes(
        const ZipEntryRecord &record,
        const mem::ByteBlock &extra,
        uint32_t crc,
        unit::ByteLength compressed,
        unit::ByteLength uncompressed) const;
    /// Parse one bounded APPNOTE central-file-header record.
    [[nodiscard]] auto parseCentralEntry(mem::ByteReader &reader) -> ZipEntryRecord;
    /// Read and validate the selected local record while the source lock is held.
    [[nodiscard]] auto readEntryData(const ZipEntryRecord &record, bool readPayload = true) const -> ZipEntryData;
    /// Resolve the requested classic sentinel replacements from a ZIP64 extra field.
    [[nodiscard]] auto parseZip64Extra(
        const mem::ByteBlock &extra,
        bool needUncompressed,
        bool needCompressed,
        bool needOffset,
        bool needDisk,
        ZipOperationPhase phase) const -> Zip64Values;
    /// Accept only regular files and explicit directories from supported creator platforms.
    void validateEntryType(const ZipEntryRecord &record, uint16_t versionMadeBy, uint32_t externalAttributes) const;
    /// Reject central path conflicts and duplicate local-header offsets.
    void validateDirectoryEntries() const;
    /// Read one source range after overflow and archive-bound checks.
    [[nodiscard]] auto readBlock(unit::ByteRange range, ZipOperationPhase phase) const -> mem::ByteBlock;
    /// Throw a ZIP error carrying the reader source path.
    [[noreturn]] void throwArchiveError(
        ZipErrorReason reason, ZipOperationPhase phase, text::String title, text::String description) const;

private:
    stream::ByteInputStreamPtr _source;
    path::Path _sourcePath;
    ArchiveReaderOptions _options;
    unit::ByteLength _archiveLength;
    std::vector<std::shared_ptr<ArchiveItem>> _items;
    text::String _comment;
    unit::ByteIndex _centralDirectoryOffset;
    bool _isZip64{};
    bool _isOpen{true};
    mutable std::mutex _sourceMutex;
};

}
