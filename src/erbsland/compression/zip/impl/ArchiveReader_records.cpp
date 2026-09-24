// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveReader.hpp"

#include "ZipTools.hpp"

#include "../../../mem/ByteReader.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../stream/StreamPositionStatus.hpp"
#include "../../../stream/StreamReadStatus.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringDecoder.hpp"
#include "../../../unit/ByteIndex.hpp"

namespace erbsland::compression::zip::impl {

using namespace text::literals;

auto ArchiveReader::parseCentralEntry(mem::ByteReader &reader) -> ZipEntryRecord {
    // APPNOTE 4.3.12: parse one complete central header in field order from the bounded directory block.
    if (!reader.canRead(46U) || reader.readUInt32OrThrow() != zipTools::cCentralHeaderSignature) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP central-directory entry is invalid"_el,
            "An expected central file header is missing or truncated."_el);
    }
    const auto versionMadeBy = reader.readUInt16OrThrow();
    auto record = ZipEntryRecord{};
    record.versionNeeded = reader.readUInt16OrThrow();
    record.flags = reader.readUInt16OrThrow();
    if ((record.flags & 0x0001U) != 0U || (record.flags & 0x0040U) != 0U) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "Encrypted ZIP entries are not supported"_el,
            "Traditional and strong encryption flags are rejected."_el);
    }
    if ((record.flags & static_cast<uint16_t>(~(zipTools::cUtf8Flag | zipTools::cDescriptorFlag | 0x0006U))) != 0U) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "ZIP entry uses unsupported flags"_el,
            "Only UTF-8, data-descriptor, and method option flags are accepted."_el);
    }
    record.compressionMethod = zipTools::methodFromRaw(reader.readUInt16OrThrow());
    const auto methodOptionFlags = static_cast<uint16_t>(record.flags & 0x0006U);
    if ((record.compressionMethod == CompressionMethod::Lzma && (methodOptionFlags & 0x0004U) != 0U) ||
        (record.compressionMethod != CompressionMethod::Deflate &&
            record.compressionMethod != CompressionMethod::Lzma && methodOptionFlags != 0U)) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "ZIP entry uses unsupported method flags"_el,
            "Compression-option flags are accepted only for Deflate and the LZMA end-marker flag."_el);
    }
    record.dosTime = reader.readUInt16OrThrow();
    record.dosDate = reader.readUInt16OrThrow();
    record.crc32 = reader.readUInt32OrThrow();
    auto compressed = unit::ByteLength{reader.readUInt32OrThrow()};
    auto uncompressed = unit::ByteLength{reader.readUInt32OrThrow()};
    const auto nameLength = reader.readUInt16OrThrow();
    const auto extraLength = reader.readUInt16OrThrow();
    const auto commentLength = reader.readUInt16OrThrow();
    auto diskStart = static_cast<uint32_t>(reader.readUInt16OrThrow());
    reader.advance(2U); // internal attributes
    const auto externalAttributes = reader.readUInt32OrThrow();
    auto localOffset = unit::ByteIndex{reader.readUInt32OrThrow()};
    const auto variableLength =
        unit::ByteLength{static_cast<uint64_t>(nameLength) + static_cast<uint64_t>(extraLength) + commentLength};
    if (!reader.canRead(variableLength)) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP central-directory entry is truncated"_el,
            "The declared name, extra field, and comment do not fit in the central directory."_el);
    }
    record.name = reader.readBytesOrThrow(unit::ByteLength{nameLength});
    const auto extra = reader.readBytesOrThrow(unit::ByteLength{extraLength});
    record.comment = text::StringDecoder{reader.readBytesOrThrow(unit::ByteLength{commentLength})}.decode(
        text::StringEncoding::Utf8);
    record.directory = !record.name.isEmpty() &&
        record.name.get(record.name.endIndex().retreated(unit::ByteLength::one())).toUInt8() == '/';
    record.path = zipTools::normalizeEntryPath(record.name, record.directory);

    const auto needUncompressed = uncompressed == unit::ByteLength{0xffffffffU};
    const auto needCompressed = compressed == unit::ByteLength{0xffffffffU};
    const auto needOffset = localOffset == unit::ByteIndex{0xffffffffU};
    const auto needDisk = diskStart == 0xffffU;
    if (needUncompressed || needCompressed || needOffset || needDisk) {
        const auto zip64 = parseZip64Extra(
            extra, needUncompressed, needCompressed, needOffset, needDisk, ZipOperationPhase::Directory);
        if (needUncompressed) {
            uncompressed = zip64.uncompressedLength;
        }
        if (needCompressed) {
            compressed = zip64.compressedLength;
        }
        if (needOffset) {
            localOffset = zip64.localHeaderOffset;
        }
        if (needDisk) {
            diskStart = zip64.diskStart;
        }
        record.zip64 = true;
        record.zip64Sizes = needUncompressed || needCompressed;
        _isZip64 = true;
    }
    if (diskStart != 0U) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "Multi-disk ZIP entries are not supported"_el,
            "Every central-directory entry must begin on disk zero."_el);
    }
    record.compressedLength = compressed;
    record.uncompressedLength = uncompressed;
    record.localHeaderOffset = localOffset;
    auto opaqueCentralExtra = mem::ByteWriter{};
    const auto dosModificationTime = zipTools::dateTimeFromDos(record.dosDate, record.dosTime);
    if (!dosModificationTime.isValid()) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP DOS timestamp is invalid"_el,
            "The mandatory date and time fields do not represent a valid UTC timestamp."_el);
    }
    record.modificationTime = zipTools::modificationTimeFromExtra(extra, dosModificationTime, &opaqueCentralExtra);
    record.opaqueCentralExtra = opaqueCentralExtra.toByteBlock();
    // Rust zip uses the ZIP64 feature level for Zstandard entries. Accept that common encoding on input while Core
    // continues to advertise the APPNOTE 6.3 feature level when writing method 93.
    const auto minimumReaderVersion = record.compressionMethod == CompressionMethod::Zstandard
        ? uint16_t{45U}
        : zipTools::minimumVersion(record.compressionMethod, record.zip64);
    if (record.versionNeeded < minimumReaderVersion) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP version-needed field is inconsistent"_el,
            "The entry requires a newer APPNOTE feature level than its central header declares."_el);
    }
    if (record.versionNeeded > 63U) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "ZIP entry requires an unsupported APPNOTE version"_el,
            "This implementation supports feature levels through APPNOTE 6.3."_el);
    }
    validateEntryType(record, versionMadeBy, externalAttributes);
    return record;
}

auto ArchiveReader::readEntryData(const ZipEntryRecord &record, bool readPayload) const -> ZipEntryData {
    const auto fixedRange = unit::ByteRange{record.localHeaderOffset, unit::ByteLength{30U}};
    if (!fixedRange.isWithin(_centralDirectoryOffset.distanceFromZero())) {
        zipTools::throwMalformed("A ZIP local header exceeds the payload area."_el);
    }
    // APPNOTE 4.3.7: seek once, then consume the selected local record in its physical order.
    if (_source->setPosition(record.localHeaderOffset) != stream::StreamPositionStatus::Success) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::LocalHeader,
            "ZIP local-header seek timed out"_el,
            "The input could not reach the selected local record."_el);
    }
    const auto fixedResult = _source->readExact(unit::ByteLength{30U});
    if (fixedResult != stream::StreamReadStatus::Data) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::LocalHeader,
            "ZIP local header could not be read"_el,
            "The input ended inside the selected local file header."_el);
    }
    auto fixed = mem::ByteReader{fixedResult.data()};
    if (fixed.readUInt32OrThrow() != zipTools::cLocalHeaderSignature) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP local header is invalid"_el,
            "The central-directory offset does not identify a local file header."_el);
    }
    const auto localVersion = fixed.readUInt16OrThrow();
    const auto localFlags = fixed.readUInt16OrThrow();
    const auto localMethod = fixed.readUInt16OrThrow();
    const auto localTime = fixed.readUInt16OrThrow();
    const auto localDate = fixed.readUInt16OrThrow();
    const auto localCrc = fixed.readUInt32OrThrow();
    auto localCompressed = unit::ByteLength{fixed.readUInt32OrThrow()};
    auto localUncompressed = unit::ByteLength{fixed.readUInt32OrThrow()};
    const auto localNameLength = fixed.readUInt16OrThrow();
    const auto localExtraLength = fixed.readUInt16OrThrow();
    if (localVersion != record.versionNeeded || localFlags != record.flags ||
        localMethod != static_cast<uint16_t>(record.compressionMethod) || localTime != record.dosTime ||
        localDate != record.dosDate) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP local and central headers disagree"_el,
            "Version, flags, compression method, and timestamp fields must match."_el);
    }

    const auto variableLength = unit::ByteLength{static_cast<uint64_t>(localNameLength) + localExtraLength};
    const auto variableRange = unit::ByteRange{fixedRange.endIndex(), variableLength};
    const auto payloadRange = unit::ByteRange{variableRange.endIndex(), record.compressedLength};
    if (!variableRange.isWithin(_centralDirectoryOffset.distanceFromZero()) ||
        !payloadRange.isWithin(_centralDirectoryOffset.distanceFromZero())) {
        zipTools::throwMalformed("ZIP local metadata or payload exceeds the payload area."_el);
    }
    const auto payloadOffset = payloadRange.index();
    const auto payloadEnd = payloadRange.endIndex();
    const auto variableResult = _source->readExact(variableLength);
    if (variableResult != stream::StreamReadStatus::Data) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::LocalHeader,
            "ZIP local metadata could not be read"_el,
            "The local name or extra field is truncated."_el);
    }
    auto variable = mem::ByteReader{variableResult.data()};
    const auto localName = variable.readBytesOrThrow(unit::ByteLength{localNameLength});
    const auto localExtra = variable.readBytesOrThrow(unit::ByteLength{localExtraLength});
    if (localName != record.name) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP local and central names disagree"_el,
            "The local and central file names must match exactly."_el);
    }
    auto opaqueLocalExtra = mem::ByteWriter{};
    const auto localModificationTime =
        zipTools::modificationTimeFromExtra(localExtra, record.modificationTime, &opaqueLocalExtra);
    if (localModificationTime != record.modificationTime) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP local and central timestamps disagree"_el,
            "The local Extended Timestamp must match the immutable central-directory timestamp."_el);
    }

    validateLocalSizes(record, localExtra, localCrc, localCompressed, localUncompressed);

    auto result = ZipEntryData{{}, opaqueLocalExtra.toByteBlock(), payloadOffset};
    if (readPayload) {
        const auto payloadResult = _source->readExact(record.compressedLength);
        if (payloadResult != stream::StreamReadStatus::Data) {
            throwArchiveError(
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Payload,
                "ZIP payload read failed"_el,
                "Truncated or timed out input."_el);
        }
        result.payload = payloadResult.data();
    } else if (_source->setPosition(payloadEnd) != stream::StreamPositionStatus::Success) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::Payload,
            "ZIP payload seek failed"_el,
            "Positioning timed out."_el);
    }
    if ((record.flags & zipTools::cDescriptorFlag) == 0U) {
        return result;
    }

    // APPNOTE 4.3.9: parse the optional descriptor immediately following the selected payload.
    const auto zip64Descriptor = record.zip64Sizes || localCompressed == unit::ByteLength{0xffffffffU} ||
        localUncompressed == unit::ByteLength{0xffffffffU} || record.compressedLength.toRawValue() >= 0xffffffffU ||
        record.uncompressedLength.toRawValue() >= 0xffffffffU;
    validateDataDescriptor(record, payloadEnd, zip64Descriptor);
    return result;
}

void ArchiveReader::validateLocalSizes(
    const ZipEntryRecord &record,
    const mem::ByteBlock &extra,
    const uint32_t crc,
    unit::ByteLength compressed,
    unit::ByteLength uncompressed) const {
    const auto localHasZip64Compressed = compressed == unit::ByteLength{0xffffffffU};
    const auto localHasZip64Uncompressed = uncompressed == unit::ByteLength{0xffffffffU};
    auto localZip64 = Zip64Values{};
    if (localHasZip64Compressed || localHasZip64Uncompressed) {
        localZip64 = parseZip64Extra(
            extra, localHasZip64Uncompressed, localHasZip64Compressed, false, false, ZipOperationPhase::LocalHeader);
    }
    if ((record.flags & zipTools::cDescriptorFlag) == 0U) {
        if (localHasZip64Uncompressed) {
            uncompressed = localZip64.uncompressedLength;
        }
        if (localHasZip64Compressed) {
            compressed = localZip64.compressedLength;
        }
        if (crc != record.crc32 || compressed != record.compressedLength || uncompressed != record.uncompressedLength) {
            throwArchiveError(
                ZipErrorReason::MalformedArchive,
                ZipOperationPhase::LocalHeader,
                "ZIP local payload metadata is inconsistent"_el,
                "CRC-32 and sizes must match the central directory when no descriptor is used."_el);
        }
    } else if (
        (crc != 0U && crc != record.crc32) ||
        (localHasZip64Compressed && !localZip64.compressedLength.isZero() &&
            localZip64.compressedLength != record.compressedLength) ||
        (localHasZip64Uncompressed && !localZip64.uncompressedLength.isZero() &&
            localZip64.uncompressedLength != record.uncompressedLength) ||
        (compressed != unit::ByteLength{} && compressed != unit::ByteLength{0xffffffffU} &&
            compressed != record.compressedLength) ||
        (uncompressed != unit::ByteLength{} && uncompressed != unit::ByteLength{0xffffffffU} &&
            uncompressed != record.uncompressedLength)) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP descriptor header metadata is inconsistent"_el,
            "Non-zero local CRC and size fields must agree with the central directory."_el);
    }
}

void ArchiveReader::validateDataDescriptor(
    const ZipEntryRecord &record, const unit::ByteIndex payloadEnd, const bool zip64Descriptor) const {
    const auto unsignedLength = unit::ByteLength{zip64Descriptor ? 20U : 12U};
    const auto descriptorRange = unit::ByteRange{payloadEnd, unsignedLength};
    if (!descriptorRange.isWithin(_centralDirectoryOffset.distanceFromZero())) {
        zipTools::throwMalformed("A ZIP data descriptor exceeds the payload area."_el);
    }
    const auto descriptorEnd = descriptorRange.endIndex();
    const auto descriptorResult = _source->readExact(unsignedLength);
    if (descriptorResult != stream::StreamReadStatus::Data) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::LocalHeader,
            "ZIP data descriptor could not be read"_el,
            "The input ended inside the selected data descriptor."_el);
    }
    auto descriptor = descriptorResult.data();
    auto descriptorReader = mem::ByteReader{descriptor};
    const auto firstValue = descriptorReader.readUInt32OrThrow();
    const auto unsignedCompressed =
        zip64Descriptor ? descriptorReader.readUInt64OrThrow() : descriptorReader.readUInt32OrThrow();
    const auto unsignedUncompressed =
        zip64Descriptor ? descriptorReader.readUInt64OrThrow() : descriptorReader.readUInt32OrThrow();
    const auto unsignedMatches = firstValue == record.crc32 &&
        unsignedCompressed == record.compressedLength.toRawValue() &&
        unsignedUncompressed == record.uncompressedLength.toRawValue();

    auto signedMatches = false;
    if (firstValue == zipTools::cDataDescriptorSignature &&
        unit::ByteRange{descriptorEnd, unit::ByteLength{4U}}.isWithin(_centralDirectoryOffset.distanceFromZero())) {
        const auto suffixResult = _source->readExact(unit::ByteLength{4U});
        if (suffixResult != stream::StreamReadStatus::Data) {
            throwArchiveError(
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::LocalHeader,
                "ZIP signed data descriptor could not be read"_el,
                "The input ended inside the selected signed data descriptor."_el);
        }
        auto completeDescriptor = mem::ByteBlockEditor{descriptor};
        completeDescriptor.append(suffixResult.data());
        descriptorReader = mem::ByteReader{completeDescriptor};
        descriptorReader.advance(4U);
        const auto signedCrc = descriptorReader.readUInt32OrThrow();
        const auto signedCompressed =
            zip64Descriptor ? descriptorReader.readUInt64OrThrow() : descriptorReader.readUInt32OrThrow();
        const auto signedUncompressed =
            zip64Descriptor ? descriptorReader.readUInt64OrThrow() : descriptorReader.readUInt32OrThrow();
        signedMatches = signedCrc == record.crc32 && signedCompressed == record.compressedLength.toRawValue() &&
            signedUncompressed == record.uncompressedLength.toRawValue();
    }
    if (unsignedMatches == signedMatches) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP data descriptor is invalid or ambiguous"_el,
            "Exactly one signed or unsigned descriptor representation must match the central directory."_el);
    }
}

auto ArchiveReader::parseZip64Extra(
    const mem::ByteBlock &extra,
    const bool needUncompressed,
    const bool needCompressed,
    const bool needOffset,
    const bool needDisk,
    const ZipOperationPhase phase) const -> Zip64Values {
    auto reader = mem::ByteReader{extra};
    while (!reader.isAtEnd()) {
        if (!reader.canRead(4U)) {
            throwArchiveError(
                ZipErrorReason::MalformedArchive,
                phase,
                "ZIP extra field is truncated"_el,
                "Every extra field needs a complete identifier and length."_el);
        }
        const auto id = reader.readUInt16OrThrow();
        const auto length = reader.readUInt16OrThrow();
        if (!reader.canRead(unit::ByteLength{length})) {
            throwArchiveError(
                ZipErrorReason::MalformedArchive,
                phase,
                "ZIP extra field is truncated"_el,
                "An extra-field payload exceeds its containing record."_el);
        }
        const auto field = reader.readBytesOrThrow(unit::ByteLength{length});
        if (id != zipTools::cZip64ExtraId) {
            continue;
        }
        // APPNOTE 4.5.3: substitute sentinel fields in their fixed conditional order.
        auto fieldReader = mem::ByteReader{field};
        auto result = Zip64Values{};
        if (needUncompressed) {
            result.uncompressedLength = unit::ByteLength{fieldReader.readUInt64OrThrow()};
        }
        if (needCompressed) {
            result.compressedLength = unit::ByteLength{fieldReader.readUInt64OrThrow()};
        }
        if (needOffset) {
            result.localHeaderOffset = unit::ByteIndex{fieldReader.readUInt64OrThrow()};
        }
        if (needDisk) {
            result.diskStart = fieldReader.readUInt32OrThrow();
        }
        if (!fieldReader.isAtEnd()) {
            throwArchiveError(
                ZipErrorReason::MalformedArchive,
                phase,
                "ZIP64 extra field has unexpected data"_el,
                "Only values corresponding to sentinel fields are accepted."_el);
        }
        return result;
    }
    throwArchiveError(
        ZipErrorReason::MalformedArchive,
        phase,
        "ZIP64 extra field is missing"_el,
        "A classic sentinel field has no matching ZIP64 replacement."_el);
}

}
