// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveWriter.hpp"

#include "ArchiveItem.hpp"
#include "ZipTools.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../path/PathMoveOptions.hpp"
#include "../../../path/PathOperations.hpp"
#include "../../../stream/StreamError.hpp"
#include "../../../stream/StreamWriteStatus.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringEncoder.hpp"

#include <exception>
#include <utility>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

void ArchiveWriter::requireWritable() const {
    if (_finalized) {
        throw err::LogicError{"The ZIP archive writer is already finalized."_el};
    }
    if (_aborted) {
        throw err::LogicError{"The ZIP archive writer was aborted."_el};
    }
}

auto ArchiveWriter::normalizedStoragePath(const path::Path &value, const bool directory) const -> path::Path {
    if (value.isEmpty()) {
        throw err::ParameterError{"The ZIP storage path must not be empty."_el, "value"_el};
    }
    auto text = value.toString();
    if (directory && !text.endsWith("/"_el)) {
        auto editor = text::StringEditor{text};
        editor.append(text::Char{U'/'});
        text = editor;
    }
    try {
        return zipTools::normalizeEntryPath(text::StringEncoder{text}.encode(text::StringEncoding::Utf8), directory);
    } catch (const ZipError &) {
        throw err::ParameterError{"The ZIP storage path must be a safe, portable relative path."_el, "value"_el};
    }
}

void ArchiveWriter::addItem(const zip::ArchiveItem &source) {
    requireWritable();
    const auto *item = dynamic_cast<const impl::ArchiveItem *>(&source);
    if (item == nullptr) {
        throw err::ParameterError{"The ZIP item must originate from an Erbsland ZIP archive reader."_el, "source"_el};
    }
    const auto &record = item->record();
    const auto entryData = item->entryData();
    // APPNOTE 4.3.7 and 4.3.12: copy opaque extras and compressed payload, but regenerate all managed fields.
    addCompressed(
        entryData.payload,
        record.path,
        record.compressionMethod,
        record.uncompressedLength,
        record.crc32,
        record.modificationTime,
        record.comment,
        entryData.opaqueLocalExtra,
        record.opaqueCentralExtra,
        record.directory,
        [&](ZipEntryRecord &written) {
            item->copyPayload([&](mem::ConstByteSpan bytes) {
                auto block = mem::ByteBlockEditor{};
                block.append(bytes);
                write(block);
            });
            written.compressedLength = record.compressedLength;
            written.uncompressedLength = record.uncompressedLength;
            written.crc32 = record.crc32;
        },
        record.compressedLength,
        static_cast<uint16_t>(record.flags & 0x0006U));
}

void ArchiveWriter::writeCentralDirectory() {
    const auto directoryOffset = _offset;
    auto archiveNeedsZip64 = _options.zip64Policy() == Zip64Policy::Always || _entries.size() >= 0xffffU;
    auto directoryLength = unit::ByteLength{};
    for (const auto &entry : _entries) {
        const auto &record = entry.record;
        const auto zip64Entry = _options.zip64Policy() == Zip64Policy::Always || record.zip64 ||
            record.localHeaderOffset >= unit::ByteIndex{0xffffffffU};
        archiveNeedsZip64 = archiveNeedsZip64 || zip64Entry;
        directoryLength = directoryLength.addedOrThrow(unit::ByteLength{46U})
                              .addedOrThrow(entry.name.length())
                              .addedOrThrow(entry.centralExtra.length())
                              .addedOrThrow(entry.comment.length());
    }
    archiveNeedsZip64 = archiveNeedsZip64 || directoryOffset >= unit::ByteLength{0xffffffffU} ||
        directoryLength >= unit::ByteLength{0xffffffffU};
    if (_options.zip64Policy() == Zip64Policy::Never && archiveNeedsZip64) {
        throwWriterError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Finalization,
            "ZIP64 is required but disabled"_el,
            "The central directory cannot be represented using classic ZIP fields."_el);
    }
    auto finalLength = directoryOffset.addedOrThrow(directoryLength);
    if (archiveNeedsZip64) {
        finalLength = finalLength.addedOrThrow(unit::ByteLength{76U});
    }
    // Validate the full archive extent before emitting any central-directory bytes.
    finalLength.addOrThrow(unit::ByteLength{22U}).addOrThrow(_comment.length());

    // Emit one central entry at a time without retaining the serialized directory.
    for (const auto &entry : _entries) {
        writeCentralEntry(entry);
    }
    writeEndRecords(directoryOffset, directoryLength, archiveNeedsZip64);
}

void ArchiveWriter::writeCentralEntry(const WrittenEntry &entry) {
    const auto &record = entry.record;
    const auto zip64Entry = _options.zip64Policy() == Zip64Policy::Always || record.zip64 ||
        record.localHeaderOffset >= unit::ByteIndex{0xffffffffU};
    const auto versionNeeded = zipTools::minimumVersion(record.compressionMethod, zip64Entry);
    auto directory = mem::ByteWriter{};
    directory.reserve(
        unit::ByteLength{46U} + entry.name.length() + entry.centralExtra.length() + entry.comment.length());
    directory.writeUInt32(zipTools::cCentralHeaderSignature)
        .writeUInt16(static_cast<uint16_t>((3U << 8U) | 63U))
        .writeUInt16(static_cast<uint16_t>(versionNeeded))
        .writeUInt16(record.flags)
        .writeUInt16(static_cast<uint16_t>(record.compressionMethod))
        .writeUInt16(record.dosTime)
        .writeUInt16(record.dosDate)
        .writeUInt32(record.crc32)
        .writeUInt32(zip64Entry ? 0xffffffffU : static_cast<uint32_t>(record.compressedLength.toRawValue()))
        .writeUInt32(zip64Entry ? 0xffffffffU : static_cast<uint32_t>(record.uncompressedLength.toRawValue()))
        .writeUInt16(static_cast<uint16_t>(entry.name.length().toRawValue()))
        .writeUInt16(static_cast<uint16_t>(entry.centralExtra.length().toRawValue()))
        .writeUInt16(static_cast<uint16_t>(entry.comment.length().toRawValue()))
        .writeUInt16(0U)
        .writeUInt16(0U);
    const auto unixMode = record.directory ? 0040755U : 0100644U;
    directory.writeUInt32(static_cast<uint32_t>((unixMode << 16U) | (record.directory ? 0x10U : 0U)))
        .writeUInt32(zip64Entry ? 0xffffffffU : static_cast<uint32_t>(record.localHeaderOffset.toRawValue()))
        .writeBytes(entry.name)
        .writeBytes(entry.centralExtra)
        .writeBytes(entry.comment);
    write(directory.toByteBlock());
}

void ArchiveWriter::writeEndRecords(
    const unit::ByteLength directoryOffset, const unit::ByteLength directoryLength, const bool archiveNeedsZip64) {
    if (archiveNeedsZip64) {
        // APPNOTE 4.3.14-4.3.15: append the ZIP64 end record and its single-disk locator.
        const auto zip64Offset = _offset;
        auto zip64 = mem::ByteWriter{};
        zip64.reserve(unit::ByteLength{76U});
        zip64.writeUInt32(zipTools::cZip64EndSignature)
            .writeUInt64(44U)
            .writeUInt16(static_cast<uint16_t>((3U << 8U) | 63U))
            .writeUInt16(45U)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt64(_entries.size())
            .writeUInt64(_entries.size())
            .writeUInt64(directoryLength.toRawValue())
            .writeUInt64(directoryOffset.toRawValue())
            .writeUInt32(zipTools::cZip64LocatorSignature)
            .writeUInt32(0U)
            .writeUInt64(zip64Offset.toRawValue())
            .writeUInt32(1U);
        write(zip64.toByteBlock());
    }
    // APPNOTE 4.3.16: EOCD terminates the archive and owns the archive comment.
    auto end = mem::ByteWriter{};
    end.reserve(unit::ByteLength{22U}.addedOrThrow(_comment.length()));
    end.writeUInt32(zipTools::cEndSignature)
        .writeUInt16(0U)
        .writeUInt16(0U)
        .writeUInt16(archiveNeedsZip64 ? 0xffffU : static_cast<uint16_t>(_entries.size()))
        .writeUInt16(archiveNeedsZip64 ? 0xffffU : static_cast<uint16_t>(_entries.size()))
        .writeUInt32(archiveNeedsZip64 ? 0xffffffffU : static_cast<uint32_t>(directoryLength.toRawValue()))
        .writeUInt32(archiveNeedsZip64 ? 0xffffffffU : static_cast<uint32_t>(directoryOffset.toRawValue()))
        .writeUInt16(static_cast<uint16_t>(_comment.length().toRawValue()))
        .writeBytes(_comment);
    write(end.toByteBlock());
}

void ArchiveWriter::write(const mem::ByteBlock &data) {
    if (data.isEmpty()) {
        return;
    }
    try {
        auto bytes = data.span();
        const auto maximum =
            std::min(std::size_t{65536U}, _destination->outputSettings().backBufferLimit().toSizeTOrThrow());
        while (!bytes.empty()) {
            const auto part = bytes.first(std::min(bytes.size(), maximum));
            const auto nextOffset = _offset.addedOrThrow(unit::ByteLength::fromSizeT(part.size()));
            if (_destination->write(part) != stream::StreamWriteStatus::Success) {
                throwWriterError(
                    ZipErrorReason::StreamFailure,
                    ZipOperationPhase::Finalization,
                    "ZIP archive write timed out"_el,
                    "The output stream accepted no bytes before its timeout."_el);
            }
            _offset = nextOffset;
            bytes = bytes.subspan(part.size());
        }
    } catch (const ZipError &) {
        abort();
        throw;
    } catch (const stream::StreamError &) {
        const auto cause = std::current_exception();
        abort();
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Finalization,
                "ZIP archive output failed"_el,
                "The owned output stream failed while writing an archive record."_el}
                .setDestinationPath(_destinationPath),
            cause};
    }
}

void ArchiveWriter::commitPathDestination() {
    if (_temporary == nullptr) {
        return;
    }
    _temporaryPath.operations().moveToOrThrow(
        _destinationPath,
        path::PathMoveOptions{}.setCreateParents(true).setCollisionMode(path::PathCollisionMode::Overwrite));
    _temporaryPath = {};
    _temporary.reset();
}

auto ArchiveWriter::includeDirectoryPath(const text::String &storagePath, const ArchiveDirectoryOptions &options) const
    -> bool {
    auto included = options.includeGlobs().isEmpty();
    for (const auto &pattern : options.includeGlobs()) {
        if (zipTools::globMatches(pattern, storagePath)) {
            included = true;
            break;
        }
    }
    if (!included) {
        return false;
    }
    for (const auto &pattern : options.excludeGlobs()) {
        if (zipTools::globMatches(pattern, storagePath)) {
            return false;
        }
    }
    return true;
}

void ArchiveWriter::throwWriterError(
    const ZipErrorReason reason, const ZipOperationPhase phase, text::String title, text::String description) const {
    throw ZipError{
        ZipErrorContext{reason, phase, std::move(title), std::move(description)}.setDestinationPath(_destinationPath)};
}

}
