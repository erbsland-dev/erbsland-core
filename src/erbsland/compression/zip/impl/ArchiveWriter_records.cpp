// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveWriter.hpp"

#include "ArchiveItem.hpp"
#include "ZipTools.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../path/PathCommitOptions.hpp"
#include "../../../path/PathError.hpp"
#include "../../../path/PathGlobPattern.hpp"
#include "../../../path/PathOperations.hpp"
#include "../../../stream/StreamError.hpp"
#include "../../../stream/StreamWriteStatus.hpp"
#include "../../../stream/TempByteOutputStream.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringEncoder.hpp"

#include <exception>
#include <utility>

namespace erbsland::compression::zip::impl {

using namespace err;
using namespace mem;
using namespace path;
using namespace text;
using namespace unit;
using namespace text::literals;

void ArchiveWriter::requireWritable() const {
    if (_finalized) {
        throw LogicFailure{"The ZIP archive writer is already finalized."_el};
    }
    if (_aborted) {
        throw LogicFailure{"The ZIP archive writer was aborted."_el};
    }
}

auto ArchiveWriter::normalizedStoragePath(const Path &value, const bool directory) const -> Path {
    if (value.isEmpty()) {
        throw ParameterFailure{"The ZIP storage path must not be empty."_el, "value"_el};
    }
    auto text = value.toString();
    if (directory && !text.endsWith("/"_el)) {
        auto editor = StringEditor{text};
        editor.append(Char{U'/'});
        text = editor;
    }
    try {
        return zipTools::normalizeEntryPath(StringEncoder{text}.encode(StringEncoding::Utf8), directory);
    } catch (const ZipError &) {
        throw ParameterFailure{"The ZIP storage path must be a safe, portable relative path."_el, "value"_el};
    }
}

void ArchiveWriter::addItem(const zip::ArchiveItem &source) {
    requireWritable();
    const auto *item = dynamic_cast<const impl::ArchiveItem *>(&source);
    if (item == nullptr) {
        throw ParameterFailure{"The ZIP item must originate from an Erbsland ZIP archive reader."_el, "source"_el};
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
            item->copyPayload([&](ConstByteSpan bytes) {
                auto block = ByteBlockEditor{};
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
    auto directoryLength = ByteCount{};
    for (const auto &entry : _entries) {
        const auto &record = entry.record;
        const auto zip64Entry = _options.zip64Policy() == Zip64Policy::Always || record.zip64 ||
            record.localHeaderOffset >= ByteIndex{0xffffffffU};
        archiveNeedsZip64 = archiveNeedsZip64 || zip64Entry;
        directoryLength = directoryLength.addedOrThrow(ByteCount{46U})
                              .addedOrThrow(entry.name.length())
                              .addedOrThrow(entry.centralExtra.length())
                              .addedOrThrow(entry.comment.length());
    }
    archiveNeedsZip64 =
        archiveNeedsZip64 || directoryOffset >= ByteCount{0xffffffffU} || directoryLength >= ByteCount{0xffffffffU};
    if (_options.zip64Policy() == Zip64Policy::Never && archiveNeedsZip64) {
        throwWriterError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Finalization,
            "ZIP64 is required but disabled"_el,
            "The central directory cannot be represented using classic ZIP fields."_el);
    }
    auto finalLength = directoryOffset.addedOrThrow(directoryLength);
    if (archiveNeedsZip64) {
        finalLength = finalLength.addedOrThrow(ByteCount{76U});
    }
    // Validate the full archive extent before emitting any central-directory bytes.
    finalLength.addOrThrow(ByteCount{22U}).addOrThrow(_comment.length());

    // Emit one central entry at a time without retaining the serialized directory.
    for (const auto &entry : _entries) {
        writeCentralEntry(entry);
    }
    writeEndRecords(directoryOffset, directoryLength, archiveNeedsZip64);
}

void ArchiveWriter::writeCentralEntry(const WrittenEntry &entry) {
    const auto &record = entry.record;
    const auto zip64Entry = _options.zip64Policy() == Zip64Policy::Always || record.zip64 ||
        record.localHeaderOffset >= ByteIndex{0xffffffffU};
    const auto versionNeeded = zipTools::minimumVersion(record.compressionMethod, zip64Entry);
    auto directory = ByteWriter{};
    directory.reserve(ByteCount{46U} + entry.name.length() + entry.centralExtra.length() + entry.comment.length());
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
    const ByteCount directoryOffset, const ByteCount directoryLength, const bool archiveNeedsZip64) {
    if (archiveNeedsZip64) {
        // APPNOTE 4.3.14-4.3.15: append the ZIP64 end record and its single-disk locator.
        const auto zip64Offset = _offset;
        auto zip64 = ByteWriter{};
        zip64.reserve(ByteCount{76U});
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
    auto end = ByteWriter{};
    end.reserve(ByteCount{22U}.addedOrThrow(_comment.length()));
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

void ArchiveWriter::write(const ByteBlock &data) {
    if (data.isEmpty()) {
        return;
    }
    try {
        auto bytes = data.span();
        const auto maximum =
            std::min(std::size_t{65536U}, _destination->outputSettings().backBufferLimit().toSizeTOrThrow());
        while (!bytes.empty()) {
            const auto part = bytes.first(std::min(bytes.size(), maximum));
            const auto nextOffset = _offset.addedOrThrow(ByteCount::fromSizeT(part.size()));
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
    try {
        _temporary->commitToOrThrow(
            _destinationPath,
            PathCommitOptions{}.setCreateParents(true).setCollisionMode(PathCollisionMode::Overwrite));
    } catch (const PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Finalization,
                "ZIP archive could not be committed"_el,
                "Finalizing or replacing the path-backed archive failed."_el}
                .setDestinationPath(_destinationPath),
            std::current_exception()};
    }
    _temporaryPath = {};
    _temporary.reset();
}

auto ArchiveWriter::includeDirectoryPath(const Path &storagePath, const ArchiveDirectoryOptions &options) const
    -> bool {
    if (options.includeGlobs() != nullptr && !options.includeGlobs()->fullMatch(storagePath)) {
        return false;
    }
    return options.excludeGlobs() == nullptr || !options.excludeGlobs()->fullMatch(storagePath);
}

void ArchiveWriter::throwWriterError(
    const ZipErrorReason reason, const ZipOperationPhase phase, String title, String description) const {
    throw ZipError{
        ZipErrorContext{reason, phase, std::move(title), std::move(description)}.setDestinationPath(_destinationPath)};
}

}
