// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveWriter.hpp"

#include "ZipTools.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../path/PathError.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathOperations.hpp"
#include "../../../path/PathType.hpp"
#include "../../../path/PathWalker.hpp"
#include "../../../path/PathWalkOptions.hpp"
#include "../../../path/PathWalkStatus.hpp"
#include "../../../path/SymlinkMode.hpp"
#include "../../../stream/TempByteOutputStream.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringEncoder.hpp"
#include "../../../time/Timestamp.hpp"

#include <exception>
#include <limits>
#include <utility>

namespace erbsland::compression::zip::impl {

using namespace err;
using namespace mem;
using namespace path;
using namespace text;
using namespace unit;
using namespace text::literals;

ArchiveWriter::ArchiveWriter(
    stream::ByteOutputStreamPtr destination,
    stream::TempByteOutputStreamPtr temporary,
    Path destinationPath,
    ArchiveWriterOptions options) :
    _destination{std::move(destination)},
    _temporary{std::move(temporary)},
    _destinationPath{std::move(destinationPath)},
    _options{std::move(options)} {
    if (_destination == nullptr) {
        throw ParameterFailure{"The ZIP output stream must not be null."_el, "destination"_el};
    }
    if (_temporary != nullptr) {
        _temporaryPath = _temporary->path();
    }
}

ArchiveWriter::~ArchiveWriter() {
    if (!_finalized && !_aborted) {
        abort();
    }
}

void ArchiveWriter::setCompressionMethod(const CompressionMethod value) {
    requireWritable();
    _compressionMethod = value;
}

void ArchiveWriter::setCompressionLevel(const compression::CompressionLevel value) {
    requireWritable();
    _compressionLevel = value;
}

void ArchiveWriter::setBaseDirectory(Path value) {
    requireWritable();
    _baseDirectory = std::move(value);
}

void ArchiveWriter::setComment(String value) {
    requireWritable();
    auto comment = StringEncoder{value}.encode(StringEncoding::Utf8);
    if (comment.length().toRawValue() > std::numeric_limits<uint16_t>::max()) {
        throw ParameterFailure{"A ZIP archive comment cannot exceed 65,535 UTF-8 bytes."_el, "value"_el};
    }
    _comment = std::move(comment);
}

void ArchiveWriter::setCleanupEnabled(const bool value) noexcept {
    _cleanupEnabled = value;
    if (_temporary != nullptr) {
        _temporary->setRemoveOnClose(value);
    }
}

void ArchiveWriter::addDirectory(const Path &source, ArchiveDirectoryOptions options) {
    requireWritable();
    const auto sourceInfo = source.info();
    if (!sourceInfo.isDirectory()) {
        throw ParameterFailure{"The ZIP directory source must be an existing directory."_el, "source"_el};
    }
    try {
        auto walkOptions = PathWalkOptions{};
        walkOptions.setSymlinkMode(SymlinkMode::Skip);
        walkOptions.setTypes(PathTypes{PathType::RegularFile, PathType::Directory});
        source.walker().walkOrThrow(
            [&](const Path &current, const PathInfo &info) {
                const auto isRoot = current == source;
                auto relative = isRoot ? Path{} : current.toRelativeOrThrow(source);
                auto storage = relative;
                if (options.includeRootDirectory()) {
                    storage = isRoot ? Path::fromPosix(source.name()) : Path::fromPosix(source.name()) / relative;
                }
                if (!isRoot || options.includeRootDirectory()) {
                    const auto accepted = includeDirectoryPath(storage, options) &&
                        (!options.filter() || options.filter()(current, info));
                    if (accepted) {
                        if (info.isDirectory()) {
                            addCompressed(
                                {},
                                normalizedStoragePath(storage, true),
                                CompressionMethod::Stored,
                                {},
                                0U,
                                info.lastModified(),
                                {},
                                {},
                                {},
                                true);
                        } else if (info.isRegularFile()) {
                            auto entryOptions = options.entryOptions();
                            entryOptions.setModificationTime(info.lastModified());
                            addFile(current, storage, std::move(entryOptions));
                        }
                    }
                }
                if (!options.recursive() && !isRoot && info.isDirectory()) {
                    return PathWalkStatus::Skip;
                }
                return PathWalkStatus::Continue;
            },
            walkOptions);
    } catch (const ZipError &) {
        throw;
    } catch (const PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::DirectoryTraversal,
                "Directory could not be added to the ZIP archive"_el,
                "Traversal accepts only regular files and directories and never follows symbolic links."_el}
                .setSourcePath(source)
                .setDestinationPath(_destinationPath),
            std::current_exception()};
    }
}

void ArchiveWriter::finalize() {
    if (_finalized) {
        return;
    }
    requireWritable();
    try {
        writeCentralDirectory();
        if (_temporary != nullptr) {
            commitPathDestination();
        } else if (!_destination->close().isClosed()) {
            throwWriterError(
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Finalization,
                "ZIP archive output could not be closed"_el,
                "The owned output stream did not close within its configured timeout."_el);
        }
        _finalized = true;
    } catch (const RuntimeError &) {
        abort();
        throw;
    }
}

void ArchiveWriter::abort() noexcept {
    if (_finalized || _aborted) {
        return;
    }
    _aborted = true;
    if (_temporary != nullptr && !_cleanupEnabled) {
        _temporaryPath = _temporary->release();
    }
    if (_destination != nullptr) {
        _destination->abort();
    }
    if (_cleanupEnabled && !_temporaryPath.isEmpty() && (_temporary == nullptr || _temporary->path().isEmpty())) {
        try {
            _temporaryPath.operations().removeOrThrow();
        } catch (const PathError &) {}
        _temporaryPath = {};
    }
    _destination.reset();
    _temporary.reset();
}

auto ArchiveWriter::isFinalized() const noexcept -> bool {
    return _finalized;
}

auto ArchiveWriter::temporaryPath() const noexcept -> const Path & {
    return _temporaryPath;
}

void ArchiveWriter::addCompressed(
    const ByteBlock &payload,
    const Path &storagePath,
    const CompressionMethod method,
    const ByteCount uncompressedLength,
    const uint32_t crcValue,
    time::Timestamp modificationTime,
    String comment,
    ByteBlock opaqueLocalExtra,
    ByteBlock opaqueCentralExtra,
    const bool directory,
    std::function<void(ZipEntryRecord &)> producer,
    std::optional<ByteCount> compressedBound,
    std::optional<uint16_t> methodFlags) {
    requireWritable();
    if (payload.length() > _options.maximumBufferedItemLength()) {
        throwWriterError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Compression,
            "ZIP compressed item buffer limit exceeded"_el,
            "The compressed payload exceeds the configured maximum buffered length."_el);
    }
    if (_entries.size() >= _options.maximumItemCount().toSizeT()) {
        throwWriterError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Compression,
            "ZIP item count limit exceeded"_el,
            "No more entries can be added within the configured maximum."_el);
    }
    const auto normalized = normalizedStoragePath(storagePath, directory);
    auto pathPosition = _paths.lower_bound(normalized);
    auto hasPathConflict = pathPosition != _paths.end() && pathPosition->first == normalized;
    if (!hasPathConflict && !directory && pathPosition != _paths.end()) {
        hasPathConflict = zipTools::isPathPrefix(normalized, pathPosition->first);
    }
    if (!hasPathConflict) {
        for (const auto &parent : normalized.parents()) {
            const auto parentPosition = _paths.find(parent);
            if (parentPosition != _paths.end() && !parentPosition->second) {
                hasPathConflict = true;
                break;
            }
        }
    }
    if (hasPathConflict) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::UnsafePath,
            ZipOperationPhase::Compression,
            "ZIP archive paths collide"_el,
            "Files cannot duplicate another normalized path or act as a parent directory."_el}
                .setDestinationPath(_destinationPath)
                .setItemPath(normalized)};
    }
    auto nameText = normalized.toPosix();
    if (directory) {
        auto editor = StringEditor{nameText};
        editor.append(Char{U'/'});
        nameText = editor;
    }
    const auto name = StringEncoder{nameText}.encode(StringEncoding::Utf8);
    const auto commentBytes = StringEncoder{comment}.encode(StringEncoding::Utf8);
    if (name.length().toRawValue() > std::numeric_limits<uint16_t>::max() ||
        commentBytes.length().toRawValue() > std::numeric_limits<uint16_t>::max()) {
        throw ParameterFailure{"A ZIP entry path or comment cannot exceed 65,535 UTF-8 bytes."_el, "storagePath"_el};
    }
    auto record = ZipEntryRecord{};
    record.path = normalized;
    record.modificationTime = modificationTime.isValid() ? modificationTime : time::Timestamp::now();
    record.compressionMethod = directory ? CompressionMethod::Stored : method;
    record.flags = static_cast<uint16_t>(
        zipTools::cUtf8Flag | (producer ? zipTools::cDescriptorFlag : 0U) |
        methodFlags.value_or(method == CompressionMethod::Lzma ? 2U : 0U));
    record.crc32 = directory ? 0U : crcValue;
    record.compressedLength = directory ? ByteCount{} : payload.length();
    record.uncompressedLength = directory ? ByteCount{} : uncompressedLength;
    record.localHeaderOffset = ByteIndex::end(_offset);
    record.directory = directory;
    zipTools::dateTimeToDos(record.modificationTime, record.dosDate, record.dosTime);
    const auto needsZip64 = _options.zip64Policy() == Zip64Policy::Always ||
        record.compressedLength.toRawValue() >= 0xffffffffU || record.uncompressedLength.toRawValue() >= 0xffffffffU ||
        (producer && (!compressedBound || compressedBound->toRawValue() >= 0xffffffffU));
    if (_options.zip64Policy() == Zip64Policy::Never &&
        (needsZip64 || record.localHeaderOffset >= ByteIndex{0xffffffffU})) {
        throwWriterError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Compression,
            "ZIP64 is required but disabled"_el,
            "The next local entry cannot be represented using classic ZIP fields."_el);
    }
    record.zip64 = needsZip64 || record.localHeaderOffset >= ByteIndex{0xffffffffU};
    record.zip64Sizes = needsZip64;
    record.versionNeeded = zipTools::minimumVersion(record.compressionMethod, record.zip64);
    auto localExtraWriter = ByteWriter{};
    if (needsZip64) {
        // APPNOTE 4.5.3: local ZIP64 fields contain uncompressed then compressed size.
        localExtraWriter.writeUInt16(zipTools::cZip64ExtraId)
            .writeUInt16(16U)
            .writeUInt64(producer ? 0U : record.uncompressedLength.toRawValue())
            .writeUInt64(producer ? 0U : record.compressedLength.toRawValue());
    }
    zipTools::appendExtendedTimestamp(localExtraWriter, record.modificationTime);
    localExtraWriter.writeBytes(opaqueLocalExtra);
    const auto localExtra = localExtraWriter.toByteBlock();

    if (localExtra.length().toRawValue() > 65535U || opaqueCentralExtra.length().toRawValue() + 64U > 65535U) {
        throw ParameterFailure{"ZIP extra fields exceed the record limit."_el, "extra"_el};
    }
    // APPNOTE 4.3.7: streamed sizes are completed by the signed data descriptor.
    auto local = ByteWriter{};
    local.reserve(ByteCount{30U}.addedOrThrow(name.length()).addedOrThrow(localExtra.length()));
    local.writeUInt32(zipTools::cLocalHeaderSignature)
        .writeUInt16(record.versionNeeded)
        .writeUInt16(record.flags)
        .writeUInt16(static_cast<uint16_t>(record.compressionMethod))
        .writeUInt16(record.dosTime)
        .writeUInt16(record.dosDate)
        .writeUInt32(producer ? 0U : record.crc32)
        .writeUInt32(
            needsZip64 ? 0xffffffffU : static_cast<uint32_t>(producer ? 0U : record.compressedLength.toRawValue()))
        .writeUInt32(
            needsZip64 ? 0xffffffffU : static_cast<uint32_t>(producer ? 0U : record.uncompressedLength.toRawValue()))
        .writeUInt16(static_cast<uint16_t>(name.length().toRawValue()))
        .writeUInt16(static_cast<uint16_t>(localExtra.length().toRawValue()))
        .writeBytes(name)
        .writeBytes(localExtra);
    try {
        write(local.toByteBlock());
        if (producer) {
            producer(record);
            auto descriptor = ByteWriter{};
            descriptor.writeUInt32(zipTools::cDataDescriptorSignature).writeUInt32(record.crc32);
            if (needsZip64) {
                descriptor.writeUInt64(record.compressedLength.toRawValue())
                    .writeUInt64(record.uncompressedLength.toRawValue());
            } else {
                descriptor.writeUInt32(static_cast<uint32_t>(record.compressedLength.toRawValue()))
                    .writeUInt32(static_cast<uint32_t>(record.uncompressedLength.toRawValue()));
            }
            write(descriptor.toByteBlock());
        } else {
            write(directory ? ByteBlock{} : payload);
        }
        auto centralExtraWriter = ByteWriter{};
        if (record.zip64) {
            // APPNOTE 4.5.3: central ZIP64 fields also include the local-header offset.
            centralExtraWriter.writeUInt16(zipTools::cZip64ExtraId)
                .writeUInt16(24U)
                .writeUInt64(record.uncompressedLength.toRawValue())
                .writeUInt64(record.compressedLength.toRawValue())
                .writeUInt64(record.localHeaderOffset.toRawValue());
        }
        zipTools::appendExtendedTimestamp(centralExtraWriter, record.modificationTime);
        centralExtraWriter.writeBytes(opaqueCentralExtra);
        const auto centralExtra = centralExtraWriter.toByteBlock();
        if (localExtra.length().toRawValue() > std::numeric_limits<uint16_t>::max() ||
            centralExtra.length().toRawValue() > std::numeric_limits<uint16_t>::max()) {
            throwWriterError(
                ZipErrorReason::ResourceLimit,
                ZipOperationPhase::Compression,
                "ZIP entry extra fields are too large"_el,
                "Managed and opaque extra fields exceed the 16-bit record limit."_el);
        }

        _paths.emplace(normalized, directory);
        _entries.push_back(WrittenEntry{std::move(record), name, centralExtra, commentBytes});
    } catch (...) {
        abort();
        throw;
    }
}

}
