// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveReader.hpp"

#include "ArchiveItem.hpp"
#include "ZipTools.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/OutOfRangeError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../mem/ByteReader.hpp"
#include "../../../stream/StreamError.hpp"
#include "../../../stream/StreamPositionOrigin.hpp"
#include "../../../stream/StreamPositionStatus.hpp"
#include "../../../stream/StreamReadStatus.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringDecoder.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteOffset.hpp"
#include "../../../unit/ItemIndex.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

ArchiveReader::ArchiveReader(stream::ByteInputStreamPtr source, path::Path sourcePath, ArchiveReaderOptions options) :
    _source{std::move(source)}, _sourcePath{std::move(sourcePath)}, _options{std::move(options)} {
}

ArchiveReader::~ArchiveReader() {
    close();
}

void ArchiveReader::open() {
    if (_source == nullptr) {
        throw err::ParameterError{"The ZIP input stream must not be null."_el, "source"_el};
    }
    try {
        indexArchive();
        parseDirectory();
    } catch (const ZipError &) {
        close();
        throw;
    } catch (const err::OutOfRangeError &) {
        const auto cause = std::current_exception();
        close();
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::MalformedArchive,
                ZipOperationPhase::Directory,
                "ZIP directory record is truncated"_el,
                "A required field extends beyond its bounded ZIP record."_el}
                .setSourcePath(_sourcePath),
            cause};
    } catch (const stream::StreamError &) {
        const auto cause = std::current_exception();
        close();
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Open,
                "ZIP archive could not be opened"_el,
                "The archive stream failed while a bounded record range was read."_el}
                .setSourcePath(_sourcePath),
            cause};
    }
}

auto ArchiveReader::itemCount() const noexcept -> unit::ItemCount {
    return unit::ItemCount::fromSizeT(_items.size());
}

auto ArchiveReader::item(const unit::ItemIndex index) const noexcept -> ArchiveItemPtr {
    if (index.isNoIndex() || index.toRawValue() >= _items.size()) {
        return {};
    }
    return _items[index.toSizeT()];
}

auto ArchiveReader::items() const -> ArchiveItemList {
    auto result = ArchiveItemList{};
    for (const auto &itemValue : _items) {
        result.append(itemValue);
    }
    return result;
}

auto ArchiveReader::comment() const noexcept -> const text::String & {
    return _comment;
}

auto ArchiveReader::isZip64() const noexcept -> bool {
    return _isZip64;
}

void ArchiveReader::close() noexcept {
    const auto lock = std::lock_guard{_sourceMutex};
    if (!_isOpen) {
        return;
    }
    _isOpen = false;
    if (_source != nullptr) {
        try {
            if (!_source->close().isClosed()) {
                _source->abort();
            }
        } catch (const stream::StreamError &) {
            _source->abort();
        }
        _source.reset();
    }
}

auto ArchiveReader::isOpen() const noexcept -> bool {
    return _isOpen;
}

auto ArchiveReader::extract(const ZipEntryRecord &record, unit::ByteLength maximumLength) const -> mem::ByteBlock {
    if (maximumLength > _options.maximumBufferedItemLength()) {
        maximumLength = _options.maximumBufferedItemLength();
    }
    auto output = mem::ByteBlockEditor{};
    extractTo(record, maximumLength, [&](mem::ConstByteSpan bytes) { output.append(bytes); });
    return output;
}

auto ArchiveReader::entryData(const ZipEntryRecord &record, bool readPayload) const -> ZipEntryData {
    if (!_isOpen) {
        throw err::LogicError{"The ZIP archive reader is closed."_el};
    }
    if (readPayload && record.compressedLength > _options.maximumBufferedItemLength()) {
        throwArchiveError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Payload,
            "ZIP compressed item limit exceeded"_el,
            "The selected compressed payload exceeds the configured item bound."_el);
    }
    const auto lock = std::lock_guard{_sourceMutex};
    if (!_isOpen || _source == nullptr) {
        throw err::LogicError{"The ZIP archive reader is closed."_el};
    }
    try {
        return readEntryData(record, readPayload);
    } catch (const ZipError &) {
        throw;
    } catch (const err::OutOfRangeError &) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::LocalHeader,
            "ZIP local record is truncated"_el,
            "A required field extends beyond the selected local record."_el}
                .setSourcePath(_sourcePath)
                .setItemPath(record.path)};
    } catch (const stream::StreamError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Payload,
                "ZIP item could not be read"_el,
                "The archive stream failed while the selected local record was read."_el}
                .setSourcePath(_sourcePath)
                .setItemPath(record.path),
            std::current_exception()};
    }
}

void ArchiveReader::indexArchive() {
    if (!_source->supportsPositioning()) {
        throw err::ParameterError{"A ZIP input stream must support positioning."_el, "source"_el};
    }
    if (_source->movePosition(stream::StreamPositionOrigin::End, unit::ByteOffset{}) !=
        stream::StreamPositionStatus::Success) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::Open,
            "ZIP archive length lookup timed out"_el,
            "The input did not report its end position within the configured timeout."_el);
    }
    const auto endPosition = _source->position();
    if (endPosition.isNoIndex()) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::Open,
            "ZIP archive length is unavailable"_el,
            "The input returned an invalid end position."_el);
    }
    _archiveLength = endPosition.distanceFromZero();
}

void ArchiveReader::parseDirectory() {
    constexpr auto cEndLength = unit::ByteLength{22U};
    constexpr auto cMaximumTailLength = unit::ByteLength{65557U};
    if (_archiveLength < cEndLength) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP end record is missing"_el,
            "The archive is shorter than an End of Central Directory record."_el);
    }

    // APPNOTE 4.3.16: inspect one bounded tail containing the EOCD and its maximum comment.
    const auto tailLength = std::min(_archiveLength, cMaximumTailLength);
    const auto tailOffset = unit::ByteIndex::end(_archiveLength - tailLength);
    const auto tail = readBlock(unit::ByteRange{tailOffset, tailLength}, ZipOperationPhase::Directory);
    auto tailReader = mem::ByteReader{tail};
    const auto endOffsetInTail = findEndRecord(tail);

    tailReader.setPosition(endOffsetInTail);
    tailReader.advance(4U); // signature validated by the preceding search
    const auto diskNumber = tailReader.readUInt16OrThrow();
    const auto directoryDisk = tailReader.readUInt16OrThrow();
    const auto diskEntryCount = tailReader.readUInt16OrThrow();
    auto entryCount = static_cast<uint64_t>(tailReader.readUInt16OrThrow());
    auto directoryLength = unit::ByteLength{tailReader.readUInt32OrThrow()};
    auto directoryOffset = unit::ByteIndex{tailReader.readUInt32OrThrow()};
    const auto commentLength = tailReader.readUInt16OrThrow();
    _comment = text::StringDecoder{tailReader.readBytesOrThrow(unit::ByteLength{commentLength})}.decode(
        text::StringEncoding::Utf8);
    if (diskNumber != 0U || directoryDisk != 0U) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "Multi-disk ZIP archives are not supported"_el,
            "All EOCD disk fields must identify the single disk zero."_el);
    }

    const auto endOffset = tailOffset.advancedOrThrow(endOffsetInTail.distanceFromZero());
    const auto hasSentinel = entryCount == 0xffffU || directoryLength == unit::ByteLength{0xffffffffU} ||
        directoryOffset == unit::ByteIndex{0xffffffffU} || diskEntryCount == 0xffffU;
    auto endRecordsOffset = endOffset;
    if (hasSentinel) {
        endRecordsOffset =
            parseZip64Directory(tailReader, endOffsetInTail, endOffset, entryCount, directoryLength, directoryOffset);
    } else if (diskEntryCount != entryCount) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP entry counts are inconsistent"_el,
            "The per-disk and total entry counts differ in a single-disk archive."_el);
    }

    if (entryCount > _options.maximumItemCount().toRawValue()) {
        throwArchiveError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Directory,
            "ZIP item count limit exceeded"_el,
            "The central directory declares more entries than the configured maximum."_el);
    }
    if (directoryLength > _options.maximumCentralDirectoryLength()) {
        throwArchiveError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Directory,
            "ZIP central-directory limit exceeded"_el,
            "The central directory exceeds the configured byte limit."_el);
    }
    const auto directoryRange = unit::ByteRange{directoryOffset, directoryLength};
    if (!directoryRange.isWithin(_archiveLength) || directoryRange.endIndex() != endRecordsOffset) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP central-directory boundary is inconsistent"_el,
            "The central directory does not end at the archive end records."_el);
    }

    _centralDirectoryOffset = directoryOffset;
    const auto directory = readBlock(directoryRange, ZipOperationPhase::Directory);
    auto directoryReader = mem::ByteReader{directory};
    for (auto index = uint64_t{}; index < entryCount; ++index) {
        auto record = parseCentralEntry(directoryReader);
        _items.emplace_back(std::make_shared<ArchiveItem>(weak_from_this(), std::move(record)));
    }
    if (!directoryReader.isAtEnd()) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP central-directory length is inconsistent"_el,
            "The declared entries do not consume the complete central directory."_el);
    }
    validateDirectoryEntries();
}

auto ArchiveReader::findEndRecord(const mem::ByteBlock &tail) const -> unit::ByteIndex {
    auto reader = mem::ByteReader{tail};
    for (auto candidate = tail.endIndex().retreatedOrThrow(unit::ByteLength{22U});; --candidate) {
        reader.setPosition(candidate);
        const auto signature = reader.readUInt32OrThrow();
        reader.advance(16U);
        const auto commentLength = unit::ByteLength{reader.readUInt16OrThrow()};
        if (signature == zipTools::cEndSignature && reader.position().advanced(commentLength) == tail.endIndex()) {
            return candidate;
        }
        if (candidate.isZero()) {
            break;
        }
    }
    throwArchiveError(
        ZipErrorReason::MalformedArchive,
        ZipOperationPhase::Directory,
        "ZIP end record is missing"_el,
        "No structurally valid End of Central Directory record was found."_el);
}

auto ArchiveReader::parseZip64Directory(
    mem::ByteReader &tailReader,
    const unit::ByteIndex endOffsetInTail,
    const unit::ByteIndex endOffset,
    uint64_t &entryCount,
    unit::ByteLength &directoryLength,
    unit::ByteIndex &directoryOffset) -> unit::ByteIndex {
    // APPNOTE 4.3.15: the adjacent ZIP64 locator is already present in the bounded tail.
    if (endOffsetInTail < unit::ByteIndex{20U}) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP64 locator is missing"_el,
            "Classic sentinel fields require an adjacent ZIP64 locator."_el);
    }
    const auto locatorOffset = endOffset.retreatedOrThrow(unit::ByteLength{20U});
    tailReader.setPosition(endOffsetInTail.retreatedOrThrow(unit::ByteLength{20U}));
    if (tailReader.readUInt32OrThrow() != zipTools::cZip64LocatorSignature || tailReader.readUInt32OrThrow() != 0U) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP64 locator is invalid"_el,
            "The locator must identify the ZIP64 end record on disk zero."_el);
    }
    const auto zip64Offset = unit::ByteIndex{tailReader.readUInt64OrThrow()};
    if (tailReader.readUInt32OrThrow() != 1U ||
        !unit::ByteRange{zip64Offset, unit::ByteLength{56U}}.isWithin(locatorOffset.distanceFromZero())) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "ZIP64 locator is invalid"_el,
            "Only one complete single-disk ZIP64 end record is supported."_el);
    }

    // APPNOTE 4.3.14: only the fixed ZIP64 fields are required; the declared size bounds extensions.
    auto zip64Reader =
        mem::ByteReader{readBlock(unit::ByteRange{zip64Offset, unit::ByteLength{56U}}, ZipOperationPhase::Directory)};
    if (zip64Reader.readUInt32OrThrow() != zipTools::cZip64EndSignature) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP64 end record is invalid"_el,
            "The locator does not point to a ZIP64 End of Central Directory record."_el);
    }
    const auto zip64Size = unit::ByteLength{zip64Reader.readUInt64OrThrow()};
    const auto zip64Body = unit::ByteRange{zip64Offset.advancedOrThrow(unit::ByteLength{12U}), zip64Size};
    if (zip64Size < unit::ByteLength{44U} || !zip64Body.isWithin(_archiveLength) ||
        zip64Body.endIndex() != locatorOffset) {
        throwArchiveError(
            ZipErrorReason::MalformedArchive,
            ZipOperationPhase::Directory,
            "ZIP64 end record boundary is invalid"_el,
            "The ZIP64 record must end immediately before its locator."_el);
    }
    zip64Reader.advance(2U); // version made by
    const auto zip64VersionNeeded = zip64Reader.readUInt16OrThrow();
    const auto zip64Disk = zip64Reader.readUInt32OrThrow();
    const auto zip64DirectoryDisk = zip64Reader.readUInt32OrThrow();
    const auto diskEntries64 = zip64Reader.readUInt64OrThrow();
    entryCount = zip64Reader.readUInt64OrThrow();
    directoryLength = unit::ByteLength{zip64Reader.readUInt64OrThrow()};
    directoryOffset = unit::ByteIndex{zip64Reader.readUInt64OrThrow()};
    if (zip64Disk != 0U || zip64DirectoryDisk != 0U || diskEntries64 != entryCount) {
        throwArchiveError(
            ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "Multi-disk ZIP64 archives are not supported"_el,
            "ZIP64 disk identifiers and entry counts must describe one disk."_el);
    }
    if (zip64VersionNeeded < 45U || zip64VersionNeeded > 63U) {
        throwArchiveError(
            zip64VersionNeeded < 45U ? ZipErrorReason::MalformedArchive : ZipErrorReason::UnsupportedFeature,
            ZipOperationPhase::Directory,
            "ZIP64 end record has an invalid feature version"_el,
            "The ZIP64 record must require an APPNOTE feature level from 4.5 through 6.3."_el);
    }
    _isZip64 = true;
    return zip64Offset;
}

auto ArchiveReader::readBlock(const unit::ByteRange range, const ZipOperationPhase phase) const -> mem::ByteBlock {
    if (!range.isWithin(_archiveLength)) {
        zipTools::throwMalformed("A ZIP record range exceeds the archive boundary."_el);
    }
    const auto lock = std::lock_guard{_sourceMutex};
    if (!_isOpen || _source == nullptr) {
        throw err::LogicError{"The ZIP archive reader is closed."_el};
    }
    if (_source->setPosition(range.index()) != stream::StreamPositionStatus::Success) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            phase,
            "ZIP archive seek timed out"_el,
            "The input could not reach a validated record range."_el);
    }
    const auto result = _source->readExact(range.length());
    if (result != stream::StreamReadStatus::Data) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            phase,
            "ZIP archive range could not be read"_el,
            "The input timed out or ended inside a validated record range."_el);
    }
    return result.data();
}

void ArchiveReader::throwArchiveError(
    const ZipErrorReason reason, const ZipOperationPhase phase, text::String title, text::String description) const {
    throw ZipError{ZipErrorContext{reason, phase, std::move(title), std::move(description)}.setSourcePath(_sourcePath)};
}

}
