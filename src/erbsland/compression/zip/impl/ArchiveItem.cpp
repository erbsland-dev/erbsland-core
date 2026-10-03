// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveItem.hpp"

#include "ZipTools.hpp"

#include "../../../err/ParameterFailure.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../path/PathCommitOptions.hpp"
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

using namespace err;
using namespace path;
using namespace text::literals;

ArchiveItem::ArchiveItem(std::weak_ptr<ArchiveReader> reader, ZipEntryRecord record) :
    _reader{std::move(reader)}, _record{std::move(record)} {
}

auto ArchiveItem::path() const noexcept -> const Path & {
    return _record.path;
}

auto ArchiveItem::isDirectory() const noexcept -> bool {
    return _record.directory;
}

auto ArchiveItem::compressionMethod() const noexcept -> CompressionMethod {
    return _record.compressionMethod;
}

auto ArchiveItem::lastModificationTime() const noexcept -> const time::Timestamp & {
    return _record.modificationTime;
}

auto ArchiveItem::crc32() const noexcept -> uint32_t {
    return _record.crc32;
}

auto ArchiveItem::compressedLength() const noexcept -> unit::ByteCount {
    return _record.compressedLength;
}

auto ArchiveItem::uncompressedLength() const noexcept -> unit::ByteCount {
    return _record.uncompressedLength;
}

auto ArchiveItem::comment() const noexcept -> const text::String & {
    return _record.comment;
}

auto ArchiveItem::extract(const unit::ByteCount maximumLength) const -> mem::ByteBlock {
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

void ArchiveItem::extractToFile(const Path &destination, ArchiveExtractionOptions options) const {
    if (_record.directory) {
        throw ParameterFailure{"A ZIP directory entry cannot be extracted to a file."_el, "destination"_el};
    }
    writeExtracted(destination, options, destination.toAbsoluteOrThrow().parent());
}

void ArchiveItem::extractToDirectory(const Path &root, ArchiveExtractionOptions options) const {
    if (root.isEmpty()) {
        throw ParameterFailure{"The ZIP extraction root must not be empty."_el, "root"_el};
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

auto ArchiveItem::prepareDirectory(const Path &root, const ArchiveExtractionOptions &options) const -> bool {
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
        if (options.collisionMode() == PathCollisionMode::Skip) {
            return false;
        }
        if (options.collisionMode() == PathCollisionMode::Stop || !existing.isDirectory()) {
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
            PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateNew));
        verifySafeParent(destination, root);
    } catch (const PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure, ZipOperationPhase::Extraction, "ZIP directory could not be created"_el}
                .setDestinationPath(destination)
                .setItemPath(_record.path),
            std::current_exception()};
    }
    return true;
}

void ArchiveItem::restoreDirectoryModificationTime(const Path &root, const ArchiveExtractionOptions &options) const {
    restoreModificationTime(root / _record.path, options);
}

auto ArchiveItem::requireReader() const -> std::shared_ptr<ArchiveReader> {
    const auto reader = _reader.lock();
    if (reader == nullptr || !reader->isOpen()) {
        throw LogicFailure{"The ZIP item payload is unavailable because its reader is closed."_el};
    }
    return reader;
}

void ArchiveItem::verifySafeParent(const Path &destination, const Path &trustedRoot) const {
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

void ArchiveItem::restoreModificationTime(const Path &destination, const ArchiveExtractionOptions &options) const {
    if (!options.preserveModificationTime() || !_record.modificationTime.isValid()) {
        return;
    }
    try {
        destination.operations().setLastModifiedOrThrow(_record.modificationTime);
    } catch (const PathError &) {
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
    const Path &destination, ArchiveExtractionOptions options, const Path &trustedRoot) const {
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
        if (options.collisionMode() == PathCollisionMode::Skip) {
            return;
        }
        if (options.collisionMode() == PathCollisionMode::Stop) {
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
            PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
        verifySafeParent(destination, trustedRoot);
        if (options.useAtomicFiles()) {
            auto temporary = destination.parent().operations().openTempByteOutputStreamOrThrow(
                PathTempFileOptions{}
                    .setStreamSettings(options.outputStreamSettings())
                    .setPrefix(".zip-extract-"_el)
                    .setSuffix(".tmp"_el));
            try {
                extractToStream(*temporary, options);
            } catch (const RuntimeError &) {
                // Complete normal cleanup within the owned stream timeout; abort remains the fallback.
                try {
                    if (!temporary->close().isClosed()) {
                        temporary->abort();
                    }
                } catch (const RuntimeError &) {
                    temporary->abort();
                }
                throw;
            }
            auto commitOptions = PathCommitOptions{}.setCreateParents(true);
            if (options.collisionMode() == PathCollisionMode::Overwrite) {
                commitOptions.setCollisionMode(PathCollisionMode::Overwrite);
            } else {
                commitOptions.setCollisionMode(PathCollisionMode::Stop);
            }
            commitOptions.setBeforeCommit([this, &trustedRoot](const Path &, const Path &target) -> void {
                verifySafeParent(target, trustedRoot);
            });
            temporary->commitToOrThrow(destination, std::move(commitOptions));
        } else {
            auto output = destination.content().openByteOutputStream(
                PathWriteDataOptions{}
                    .setStreamSettings(options.outputStreamSettings())
                    .setCreateParents(true)
                    .setCreationMode(
                        options.collisionMode() == PathCollisionMode::Overwrite ? PathCreateMode::CreateOrOverwrite
                                                                                : PathCreateMode::CreateNew));
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
    } catch (const PathError &) {
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
