// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveItem.hpp"

#include "ZipTools.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../path/PathContent.hpp"
#include "../../../path/PathCreateDirectoryOptions.hpp"
#include "../../../path/PathCreateMode.hpp"
#include "../../../path/PathError.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathMoveOptions.hpp"
#include "../../../path/PathOperations.hpp"
#include "../../../path/PathTempFileOptions.hpp"
#include "../../../path/PathWriteDataOptions.hpp"
#include "../../../stream/StreamError.hpp"
#include "../../../stream/StreamWriteStatus.hpp"
#include "../../../stream/TempByteOutputStream.hpp"
#include "../../../text/Literals.hpp"

#include <utility>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

ArchiveItem::ArchiveItem(std::weak_ptr<ArchiveReader> reader, ZipEntryRecord record) :
    _reader{std::move(reader)}, _record{std::move(record)} {
}

auto ArchiveItem::path() const noexcept -> const path::Path & {
    return _record.path;
}

auto ArchiveItem::isDirectory() const noexcept -> bool {
    return _record.directory;
}

auto ArchiveItem::compressionMethod() const noexcept -> CompressionMethod {
    return _record.compressionMethod;
}

auto ArchiveItem::lastModificationTime() const noexcept -> const time::DateTime & {
    return _record.modificationTime;
}

auto ArchiveItem::crc32() const noexcept -> uint32_t {
    return _record.crc32;
}

auto ArchiveItem::compressedLength() const noexcept -> unit::ByteLength {
    return _record.compressedLength;
}

auto ArchiveItem::uncompressedLength() const noexcept -> unit::ByteLength {
    return _record.uncompressedLength;
}

auto ArchiveItem::comment() const noexcept -> const text::String & {
    return _record.comment;
}

auto ArchiveItem::extract(const unit::ByteLength maximumLength) const -> mem::ByteBlock {
    return requireReader()->extract(_record, maximumLength);
}

void ArchiveItem::extractToStream(stream::ByteOutputStream &destination, ArchiveExtractionOptions options) const {
    requireReader()->extractTo(
        _record,
        options.maximumLength(),
        [&](mem::ConstByteSpan bytes) {
            const auto maximum = destination.outputSettings().backBufferLimit().toSizeTOrThrow();
            while (!bytes.empty()) {
                const auto part = bytes.first(std::min(bytes.size(), maximum));
                if (destination.write(part) != stream::StreamWriteStatus::Success) {
                    throw ZipError{ZipErrorContext{
                        ZipErrorReason::StreamFailure,
                        ZipOperationPhase::Extraction,
                        "ZIP extraction output timed out"_el}
                            .setItemPath(_record.path)};
                }
                bytes = bytes.subspan(part.size());
            }
        },
        options.progress());
}

void ArchiveItem::copyPayload(const compression::impl::CodecOutput::Write &output) const {
    requireReader()->copyPayload(_record, output);
}

void ArchiveItem::extractToFile(const path::Path &destination, ArchiveExtractionOptions options) const {
    if (_record.directory) {
        throw err::ParameterError{"A ZIP directory entry cannot be extracted to a file."_el, "destination"_el};
    }
    writeExtracted(destination, options, destination.toAbsoluteOrThrow().parent());
}

void ArchiveItem::extractToDirectory(const path::Path &root, ArchiveExtractionOptions options) const {
    if (root.isEmpty()) {
        throw err::ParameterError{"The ZIP extraction root must not be empty."_el, "root"_el};
    }
    if (_record.directory) {
        if (prepareDirectory(root, options)) {
            restoreDirectoryModificationTime(root, options);
        }
        return;
    }
    writeExtracted(root / _record.path, options, root);
}

auto ArchiveItem::entryData() const -> ZipEntryData {
    return requireReader()->entryData(_record, false);
}

auto ArchiveItem::prepareDirectory(const path::Path &root, const ArchiveExtractionOptions &options) const -> bool {
    const auto destination = root / _record.path;
    verifySafeParent(destination, root);
    const auto existing = destination.info();
    if (existing.exists()) {
        if (existing.isSymlink() || existing.isReparsePoint()) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::UnsafePath,
                ZipOperationPhase::Extraction,
                "ZIP directory destination is a link"_el,
                "Symbolic links and reparse points are never used as extraction directories."_el}
                    .setDestinationPath(destination)
                    .setItemPath(_record.path)};
        }
        if (options.collisionMode() == path::PathCollisionMode::Skip) {
            return false;
        }
        if (options.collisionMode() == path::PathCollisionMode::Stop || !existing.isDirectory()) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Extraction,
                "ZIP directory collides with an existing path"_el}
                    .setDestinationPath(destination)
                    .setItemPath(_record.path)};
        }
        return true;
    }
    try {
        destination.operations().createDirectoryOrThrow(
            path::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(path::PathCreateMode::CreateNew));
        verifySafeParent(destination, root);
    } catch (const path::PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure, ZipOperationPhase::Extraction, "ZIP directory could not be created"_el}
                .setDestinationPath(destination)
                .setItemPath(_record.path),
            std::current_exception()};
    }
    return true;
}

void ArchiveItem::restoreDirectoryModificationTime(
    const path::Path &root, const ArchiveExtractionOptions &options) const {
    restoreModificationTime(root / _record.path, options);
}

auto ArchiveItem::requireReader() const -> std::shared_ptr<ArchiveReader> {
    const auto reader = _reader.lock();
    if (reader == nullptr || !reader->isOpen()) {
        throw err::LogicError{"The ZIP item payload is unavailable because its reader is closed."_el};
    }
    return reader;
}

void ArchiveItem::verifySafeParent(const path::Path &destination, const path::Path &trustedRoot) const {
    const auto absoluteDestination = destination.toAbsoluteOrThrow();
    const auto absoluteRoot = trustedRoot.toAbsoluteOrThrow();
    auto reachedRoot = false;
    for (const auto &parent : absoluteDestination.parents()) {
        const auto info = parent.info();
        if (info.exists() && (info.isSymlink() || info.isReparsePoint())) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::UnsafePath,
                ZipOperationPhase::Extraction,
                "ZIP extraction path crosses a link"_el,
                "Existing symbolic links and reparse points are never followed below the output root."_el}
                    .setDestinationPath(destination)
                    .setItemPath(_record.path)};
        }
        if (parent == absoluteRoot) {
            reachedRoot = true;
            break;
        }
    }
    if (!reachedRoot) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::UnsafePath,
            ZipOperationPhase::Extraction,
            "ZIP extraction destination escapes its root"_el,
            "The normalized destination must remain below the trusted extraction root."_el}
                .setDestinationPath(destination)
                .setItemPath(_record.path)};
    }
}

void ArchiveItem::restoreModificationTime(
    const path::Path &destination, const ArchiveExtractionOptions &options) const {
    if (!options.preserveModificationTime() || !_record.modificationTime.isValid()) {
        return;
    }
    try {
        destination.operations().setLastModifiedOrThrow(_record.modificationTime);
    } catch (const path::PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Extraction,
                "ZIP item modification time could not be restored"_el}
                .setDestinationPath(destination)
                .setItemPath(_record.path),
            std::current_exception()};
    }
}

void ArchiveItem::writeExtracted(
    const path::Path &destination, ArchiveExtractionOptions options, const path::Path &trustedRoot) const {
    const auto existing = destination.info();
    if (existing.exists()) {
        if (existing.isSymlink() || existing.isReparsePoint()) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::UnsafePath,
                ZipOperationPhase::Extraction,
                "ZIP extraction destination is a link"_el,
                "Symbolic links and reparse points are never followed or overwritten during extraction."_el}
                    .setDestinationPath(destination)
                    .setItemPath(_record.path)};
        }
        if (options.collisionMode() == path::PathCollisionMode::Skip) {
            return;
        }
        if (options.collisionMode() == path::PathCollisionMode::Stop) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Extraction,
                "ZIP extraction destination already exists"_el}
                    .setDestinationPath(destination)
                    .setItemPath(_record.path)};
        }
    }
    try {
        // Security boundary: inspect every existing component through the caller's trusted extraction root.
        verifySafeParent(destination, trustedRoot);
        destination.parent().operations().createDirectoryOrThrow(
            path::PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(
                path::PathCreateMode::CreateOrOverwrite));
        verifySafeParent(destination, trustedRoot);
        if (options.useAtomicFiles()) {
            auto temporary = destination.parent().operations().openTempByteOutputStreamOrThrow(
                path::PathTempFileOptions{}
                    .setStreamSettings(options.outputStreamSettings())
                    .setPrefix(".zip-extract-"_el)
                    .setSuffix(".tmp"_el));
            try {
                extractToStream(*temporary, options);
            } catch (...) {
                // Complete normal cleanup within the owned stream timeout; abort remains the fallback.
                try {
                    if (!temporary->close().isClosed()) {
                        temporary->abort();
                    }
                } catch (...) {
                    temporary->abort();
                }
                throw;
            }
            const auto temporaryPath = temporary->path();
            temporary->setRemoveOnClose(false);
            try {
                if (!temporary->close().isClosed()) {
                    temporary->setRemoveOnClose(true);
                    throw ZipError{ZipErrorContext{
                        ZipErrorReason::StreamFailure,
                        ZipOperationPhase::Extraction,
                        "ZIP extracted file could not be closed"_el}
                            .setDestinationPath(destination)
                            .setItemPath(_record.path)};
                }
                temporaryPath.operations().moveToOrThrow(
                    destination,
                    path::PathMoveOptions{}.setCreateParents(true).setCollisionMode(
                        options.collisionMode() == path::PathCollisionMode::Overwrite
                            ? path::PathCollisionMode::Overwrite
                            : path::PathCollisionMode::Stop));
                temporary->setRemoveOnClose(false);
            } catch (...) {
                temporaryPath.operations().removeOrThrow();
                throw;
            }
        } else {
            auto output = destination.content().openByteOutputStream(
                path::PathWriteDataOptions{}
                    .setStreamSettings(options.outputStreamSettings())
                    .setCreateParents(true)
                    .setCreationMode(
                        options.collisionMode() == path::PathCollisionMode::Overwrite
                            ? path::PathCreateMode::CreateOrOverwrite
                            : path::PathCreateMode::CreateNew));
            extractToStream(*output, options);
            if (!output->close().isClosed()) {
                throw ZipError{ZipErrorContext{
                    ZipErrorReason::StreamFailure, ZipOperationPhase::Extraction, "ZIP extraction close timed out"_el}};
            }
        }
        restoreModificationTime(destination, options);
    } catch (const ZipError &) {
        throw;
    } catch (const stream::StreamError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Extraction,
                "ZIP item output stream failed"_el,
                "The temporary extraction stream failed while committing the item."_el}
                .setDestinationPath(destination)
                .setItemPath(_record.path),
            std::current_exception()};
    } catch (const path::PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Extraction,
                "ZIP item could not be committed to the destination"_el}
                .setDestinationPath(destination)
                .setItemPath(_record.path),
            std::current_exception()};
    }
}

}
