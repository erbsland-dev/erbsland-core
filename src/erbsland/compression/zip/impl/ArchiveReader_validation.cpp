// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveReader.hpp"

#include "ArchiveItem.hpp"
#include "ZipTools.hpp"

#include "../../../text/Literals.hpp"

#include <algorithm>
#include <vector>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

void ArchiveReader::validateDirectoryEntries() const {
    if (_items.empty()) {
        if (!_centralDirectoryOffset.isZero()) {
            throwArchiveError(
                ZipErrorReason::UnsupportedFeature,
                ZipOperationPhase::Directory,
                "Prefixed ZIP archives are not supported"_el,
                "An empty archive must begin with its central-directory end record."_el);
        }
        return;
    }
    auto ordered = std::vector<const ZipEntryRecord *>{};
    ordered.reserve(_items.size());
    for (const auto &itemValue : _items) {
        ordered.push_back(&itemValue->record());
    }
    std::sort(ordered.begin(), ordered.end(), [](const auto *left, const auto *right) {
        return left->localHeaderOffset < right->localHeaderOffset;
    });
    if (!ordered.front()->localHeaderOffset.isZero()) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "Prefixed ZIP archives are not supported"_el,
            "The first central-directory entry must identify a local header at offset zero."_el);
    }
    for (auto index = std::size_t{}; index < ordered.size(); ++index) {
        const auto *record = ordered[index];
        const auto fixedRange = unit::ByteRange{record->localHeaderOffset, unit::ByteLength{30U}};
        const auto payloadRange = unit::ByteRange{fixedRange.endIndex(), record->compressedLength};
        if (!fixedRange.isWithin(_centralDirectoryOffset.distanceFromZero()) ||
            !payloadRange.isWithin(_centralDirectoryOffset.distanceFromZero())) {
            zipTools::throwMalformed("A ZIP local record exceeds the payload area."_el);
        }
        const auto recordMinimumEnd = payloadRange.endIndex();
        if (index + 1U < ordered.size() && recordMinimumEnd > ordered[index + 1U]->localHeaderOffset) {
            throwArchiveError(
                ZipErrorReason::MalformedArchive,
                ZipOperationPhase::Directory,
                "ZIP local record ranges collide"_el,
                "Central-directory offsets and compressed lengths must identify distinct local records."_el);
        }
    }

    std::sort(
        ordered.begin(), ordered.end(), [](const auto *left, const auto *right) { return left->path < right->path; });
    auto ancestors = std::vector<const ZipEntryRecord *>{};
    for (auto index = std::size_t{}; index < ordered.size(); ++index) {
        const auto *record = ordered[index];
        if (index != 0U && ordered[index - 1U]->path == record->path) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::MalformedArchive,
                ZipOperationPhase::Directory,
                "ZIP archive contains duplicate paths"_el,
                "Every normalized central-directory path must be unique."_el}
                    .setSourcePath(_sourcePath)
                    .setItemPath(record->path)};
        }
        while (!ancestors.empty() && !zipTools::isPathPrefix(ancestors.back()->path, record->path)) {
            ancestors.pop_back();
        }
        if (!ancestors.empty() && !ancestors.back()->directory) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::MalformedArchive,
                ZipOperationPhase::Directory,
                "ZIP archive contains colliding paths"_el,
                "Files cannot duplicate another normalized path or act as a parent directory."_el}
                    .setSourcePath(_sourcePath)
                    .setItemPath(record->path)};
        }
        ancestors.push_back(record);
    }
}

void ArchiveReader::validateEntryType(
    const ZipEntryRecord &record, const uint16_t versionMadeBy, const uint32_t externalAttributes) const {
    const auto host = static_cast<uint8_t>(versionMadeBy >> 8U);
    const auto dosDirectory = (externalAttributes & 0x10U) != 0U;
    if (host == 3U) {
        const auto mode = static_cast<uint16_t>(externalAttributes >> 16U);
        const auto type = static_cast<uint16_t>(mode & 0170000U);
        const auto expected = record.directory ? uint16_t{0040000U} : uint16_t{0100000U};
        if (type != expected) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::UnsupportedFeature,
                ZipOperationPhase::Directory,
                "ZIP special filesystem entry is not supported"_el,
                "Only regular files and explicit directories are accepted."_el}
                    .setSourcePath(_sourcePath)
                    .setItemPath(record.path)};
        }
    } else if (host != 0U && host != 10U) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "ZIP creator platform is not supported"_el,
            "Only DOS, Unix, and NTFS external attributes are accepted."_el);
    }
    if (dosDirectory && !record.directory) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP directory metadata is ambiguous"_el,
            "An external directory attribute requires a directory path suffix."_el}
                .setSourcePath(_sourcePath)
                .setItemPath(record.path)};
    }
    if (record.directory &&
        (!record.compressedLength.isZero() || !record.uncompressedLength.isZero() ||
            record.compressionMethod != CompressionMethod::Stored)) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP directory entry contains payload data"_el,
            "Explicit directories must be empty and use the Stored method."_el}
                .setSourcePath(_sourcePath)
                .setItemPath(record.path)};
    }
}

}
